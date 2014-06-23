#include "rapidapp_easy_timer.h"
#include <time.h>

namespace rapidapp {

EasyTimer::EasyTimer() : next_active_timestamp_(0), time_step_(0), timer_id_(0)
{}

EasyTimer::~EasyTimer()
{}

int EasyTimer::Init(size_t time, int timer_id)
{
    time_step_ = time;
    timer_id_ = timer_id;

    return 0;
}

void EasyTimer::CleanUp()
{}

int EasyTimer::Start()
{
    time_t now = time(NULL);
    next_active_timestamp_ = now + time_step_;

    return 0;
}

int EasyTimer::Terminate()
{
    return 0;
}

}
