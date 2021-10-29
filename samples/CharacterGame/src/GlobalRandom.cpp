#include "GlobalRandom.h"

namespace sge {
Random g_rnd = Random();

AudioDecoder bgMusic;
AudioDecoder sndPickUp;
AudioDecoder sndHurt;
extern bool isMobileEmscripten = false;
}
