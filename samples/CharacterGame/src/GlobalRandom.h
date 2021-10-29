#pragma once

#include "sge_audio/AudioDevice.h"
#include "sge_utils/math/Random.h"


namespace sge {
extern Random g_rnd;

extern AudioDecoder bgMusic;
extern AudioDecoder sndPickUp;
extern AudioDecoder sndHurt;
extern bool isMobileEmscripten;
} // namespace sge
