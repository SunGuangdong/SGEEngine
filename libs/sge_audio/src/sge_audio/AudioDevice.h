#pragma once

#include "miniaudio.h"
#include "sge_utils/sge_utils.h"
#include <memory>
#include <set>
#include <vector>

namespace sge {

struct AudioDecoder;
struct AudioData;
typedef std::shared_ptr<AudioData> AudioDataPtr;

struct AudioDevice {
	AudioDevice() = default;
	~AudioDevice() { clear(); }

	void createAudioDevice();
	void startAudioDevice();
	void clear();

	ma_mutex& getDataLockMutex() { return mutexDataLock; }

	void play(AudioDecoder* decoder, bool ifAlreadyPlayingSeekToBegining);
	void stop(AudioDecoder* decoder);

  private:
	void stop_noMutexLock(AudioDecoder* decoder);

	void dataCallback(void* pOutput, const void* pInput, ma_uint32 frameCount);

	/// @brief Returns true if the playing has done.
	bool readFramesFromDecoder(AudioDecoder& decoder, float* output, uint32 frameCount);

	static void miniaudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

	ma_device device;
	ma_mutex mutexDataLock;

	std::set<AudioDecoder*> m_playingDecoders;
	std::vector<float> decodingTemp;
	std::vector<AudioDecoder*> decodersToStopTemp;
};

struct AudioData {
	AudioData() = default;

	void createFromFile(const char* filename);

	const std::vector<char>& getData() const { return fileData; }
	bool isEmpty() { return fileData.empty(); }


  private:
	std::vector<char> fileData;
};

struct AudioDecoder {
	friend AudioDevice;

	struct State {
		bool isLooping = false;
		float volume = 1.f;
	};

  public:
	AudioDecoder() = default;
	~AudioDecoder() { clear(); }

	void clear();
	void createDecoder(AudioDataPtr& audioData);

	void seekToBegining();
	void seekToBegining_noLock();
	bool isPlaying() const { return playingDevice != nullptr; }


  public:
	AudioDataPtr audioData;
	ma_decoder decoder;
	ma_uint64 numFramesInDecoder = 0;

	State state;

  private:
	AudioDevice* playingDevice = nullptr;

	/// Sync state managed by the @AudioDevice when playing. Do not modify it. Modify @state instead.
	State m_syncState;
};

} // namespace sge
