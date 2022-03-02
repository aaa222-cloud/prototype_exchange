#include <chrono>
#include "utils.hpp"

namespace utils
{
int get_epoch_time()
{
    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    );
    return static_cast<int>(ms.count());
}

} // namespace utils