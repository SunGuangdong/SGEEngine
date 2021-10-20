#include "AudioDevice.h"
#include "sge_utils/utils/FileStream.h"

#define SAMPLE_FORMAT ma_format_f32
#define CHANNEL_COUNT 2
#define SAMPLE_RATE ma_standard_sample_rate_44100

namespace sge {

struct ma_mutex_raii {
	[[nodiscard]] ma_mutex_raii(ma_mutex& mtx)
	    : mtx(mtx) {
		ma_mutex_lock(&mtx);
	}

	~ma_mutex_raii() { ma_mutex_unlock(&mtx); }

	ma_mutex& mtx;
};

//------------------------------------------------------------------
// AudioDevice
//------------------------------------------------------------------
void AudioDevice::createAudioDevice() {
	ma_device_config deviceConfig;

	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = SAMPLE_FORMAT;
	deviceConfig.playback.channels = CHANNEL_COUNT;
	deviceConfig.sampleRate = SAMPLE_RATE;
	deviceConfig.dataCallback = miniaudioDataCallback;
	deviceConfig.pUserData = this;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
		sgeAssertFalse("Failed to initialize AudioDevice");
		ma_device_uninit(&device);
	}

	ma_mutex_init(&mutexDataLock);
}

void AudioDevice::startAudioDevice() {
	if (ma_device_start(&device) != MA_SUCCESS) {
		sgeAssertFalse("Failed to initialize AudioDevice");
		ma_device_uninit(&device);
	}
}

void AudioDevice::clear() {
	ma_device_uninit(&device);
	ma_mutex_uninit(&mutexDataLock);
}

void AudioDevice::play(AudioDecoder* decoder) {
	if (decoder) {
		if (decoder->playingDevice != nullptr) {
			sgeAssert(false && "AudioDecoder could be played only on one device!");
			return;
		}

		if (decoder->playingDevice == this) {
			// The sound is already playing.
			return;
		}

		const ma_mutex_raii lock(mutexDataLock);
		decoder->playingDevice = this;
		m_playingDecoders.insert(decoder);
	}
}

void AudioDevice::stop(AudioDecoder* decoder) {
	const ma_mutex_raii lock(mutexDataLock);
	stop_noMutexLock(decoder);
}

void AudioDevice::stop_noMutexLock(AudioDecoder* decoder) {
	if (decoder) {
		if (decoder->playingDevice != this) {
			sgeAssert(false && "AudioDecoder could be played only on one device!");
			return;
		}

		for (auto itr = m_playingDecoders.begin(); itr != m_playingDecoders.end(); itr++) {
			if (*itr == decoder) {
				m_playingDecoders.erase(itr);
				decoder->playingDevice = nullptr;
				break;
			}
		}
	}
}

bool AudioDevice::readFramesFromDecoder(AudioDecoder& decoder, float* output, uint32 frameCount) {
	// Caution:
	// The fn assumes that @mutexDataLock is already locked!

	bool isPlaybackDone = false;
	const ma_uint32 tempCapInFrames = ma_uint32(decodingTemp.size()) / CHANNEL_COUNT;

	decoder.m_syncState = decoder.state;

	ma_uint64 totalFramesRead = 0;
	while (totalFramesRead < frameCount) {
		const ma_uint64 totalFramesRemaining = frameCount - totalFramesRead;
		ma_uint64 framesToReadThisIteration = tempCapInFrames;
		if (framesToReadThisIteration > totalFramesRemaining) {
			framesToReadThisIteration = totalFramesRemaining;
		}

		ma_uint64 framesReadThisIteration = 0;
		if (decoder.m_syncState.isLooping) {
			framesReadThisIteration = ma_decoder_read_pcm_frames(&decoder.decoder, decodingTemp.data(), framesToReadThisIteration);
		} else {
			ma_data_source_read_pcm_frames(&decoder.decoder, decodingTemp.data(), framesToReadThisIteration, &framesReadThisIteration,
			                               ma_bool32(true));
		}

		if (framesReadThisIteration == 0) {
			break;
		}

		// Mix the frames together.
		for (ma_uint32 iSample = 0; iSample < framesReadThisIteration * CHANNEL_COUNT; ++iSample) {
			output[totalFramesRead * CHANNEL_COUNT + iSample] += decodingTemp[iSample];
		}

		totalFramesRead += framesReadThisIteration;

		if (framesReadThisIteration < framesToReadThisIteration) {
			// Reached the end of the stream.
			// Looping decoders never end.
			isPlaybackDone = true;
			break;
		}
	}

	return isPlaybackDone;
}

void AudioDevice::dataCallback(void* pOutput, const void* UNUSED(pInput), ma_uint32 frameCount) {
	const ma_mutex_raii lock(mutexDataLock);

	if (decodingTemp.size() < frameCount) {
		decodingTemp.resize(frameCount);
	}

	decodersToStopTemp.clear();

	for (AudioDecoder* decoder : m_playingDecoders) {
		if (decoder) {
			bool isDone = readFramesFromDecoder(*decoder, (float*)pOutput, frameCount);
			if (isDone) {
				decodersToStopTemp.push_back(decoder);
			}
		}
	}

	for (AudioDecoder* decoder : decodersToStopTemp) {
		stop_noMutexLock(decoder);
	}
}

void AudioDevice::miniaudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	if (pDevice == nullptr) {
		return;
	}

	AudioDevice* audioDevice = reinterpret_cast<AudioDevice*>(pDevice->pUserData);
	audioDevice->dataCallback(pOutput, pInput, frameCount);
}

//------------------------------------------------------------------
// AudioData
//------------------------------------------------------------------
void AudioData::createFromFile(const char* filename) {
	fileData = std::vector<char>();
	[[maybe_unused]] bool succeeded = FileReadStream::readFile(filename, fileData);
	sgeAssert(succeeded);
}

//------------------------------------------------------------------
// AudioDecoder
//------------------------------------------------------------------
void AudioDecoder::clear() {
	if (playingDevice) {
		playingDevice->stop(this);
	}

	audioData = nullptr;
	ma_decoder_uninit(&decoder);
	numFramesInDecoder = 0;
	playingDevice = nullptr;

	state = State();
}

void AudioDecoder::createDecoder(AudioDataPtr& audioData) {
	clear();

	this->audioData = audioData;

	if (audioData->isEmpty() == false) {
		ma_decoder_config decoderConfig = ma_decoder_config_init(SAMPLE_FORMAT, CHANNEL_COUNT, SAMPLE_RATE);
		if (ma_decoder_init_memory(audioData->getData().data(), audioData->getData().size(), &decoderConfig, &decoder) != MA_SUCCESS) {
			sgeAssertFalse("Failed to initialize AudioDecoder");
			ma_decoder_uninit(&decoder);
		}
	}

	ma_decoder_get_available_frames(&decoder, &numFramesInDecoder);
}

void AudioDecoder::seekToBegining() {
	if (playingDevice) {
		ma_mutex_lock(&playingDevice->getDataLockMutex());
	}

	ma_decoder_seek_to_pcm_frame(&decoder, 0);

	if (playingDevice) {
		playingDevice->stop(this);
		ma_mutex_unlock(&playingDevice->getDataLockMutex());
	}
}

} // namespace sge
