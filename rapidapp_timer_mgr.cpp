#include "rapidapp_timer_mgr.h"
#include <algorithm>
#include <glog/logging.h>

namespace rapidapp {
bool timer_cmp_func(const EasyTimer* a, const EasyTimer* b)
{
    if (NULL == a || NULL == b)
    {
        return false;
    }

    if (a->timestamp() < b->timestamp())
        return true;
    else
        return false;
}

TimerMgr::TimerMgr()
{}

TimerMgr::~TimerMgr()
{}

int TimerMgr::Init(size_t timer_max)
{
    max_timer_num_ = timer_max;
}

void TimerMgr::CleanUp()
{}

EasyTimer* TimerMgr::AddTimer(size_t time_gap, int timer_id)
{
    if (0 == time_gap)
    {
        LOG(ERROR)<<"time gap must be bigger than 0, but equal to 0";
        return NULL;
    }

    if (timer_list_.size() >= max_timer_num_)
    {
        LOG(ERROR)<<"timer has reach upper limit "<<max_timer_num_;
        return NULL;
    }

    EasyTimer* timer = new(std::nothrow) EasyTimer();
    if (NULL == timer)
    {
        PLOG(ERROR)<<"new timer failed";
        return NULL;
    }

    time_t now;
    time(&now);
    if (timer->Init(now + time_gap, timer_id) != 0)
    {
        LOG(ERROR)<<"Init timer failed";
        delete timer;
        return NULL;
    }

    TimerList::iterator it = upper_bound(timer_list_.begin(),
                                         timer_list_.end(),
                                         timer, timer_cmp_func);
    timer_list_.insert(it, timer);

    return timer;
}

int TimerMgr::RemoveTimer(const EasyTimer* timer)
{
    if (NULL == timer)
    {
        LOG(ERROR)<<"null timer to remove";
        return -1;
    }

    // TODO remove from vector

    delete timer;
    return 0;
}

}
