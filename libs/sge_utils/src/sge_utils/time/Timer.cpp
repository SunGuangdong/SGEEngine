#include "timer.h"

namespace sge {

const Timer::time_point Timer::application_start_time = Timer::clock::now();

}
