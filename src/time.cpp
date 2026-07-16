#include <protocol/utils/time.hpp>
#include <chrono>

namespace protocol::utils
{
    uint32_t getCurrentTimestamp() 
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
}