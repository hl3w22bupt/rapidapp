#include "rapidapp_timer.h"

namespace rapidapp {

EasyTimer::EasyTimer()
{}

EasyTimer::~EasyTimer()
{}

int EasyTimer::Init(time_t next_timestamp, int timer_id)
{
    next_active_timestamp_ = next_timestamp;
    timer_id_ = timer_id;

    return 0;
}

void EasyTimer::CleanUp()
{}

int EasyTimer::Start()
{
    return 0;
}

int EasyTimer::Terminate()
{
    return 0;
}

}
