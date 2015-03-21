#include "rapidapp_timer_mgr.h"
#include <algorithm>
#include <glog/logging.h>
#include <cassert>

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

TimerMgr::TimerMgr() : max_timer_num_(DEFAULT_MAX_TIMER_NUM)
{}

TimerMgr::~TimerMgr()
{}

int TimerMgr::Init(size_t timer_max)
{
    max_timer_num_ = timer_max;

    return 0;
}

void TimerMgr::CleanUp()
{
    for (TimerList::iterator it = timer_list_.begin();
         it != timer_list_.end(); ++it)
    {
        if (*it != NULL)
        {
            delete *it;
            *it = NULL;
        }
    }

    timer_list_.clear();
}

EasyTimer* TimerMgr::GetNextActiveTimer()
{
    if (timer_list_.size() <= 0)
    {
        return NULL;
    }

    TimerList::iterator curr = timer_list_.begin();
    EasyTimer* curr_timer = *curr;
    assert(curr_timer != NULL);
    curr_timer->Step();
    timer_list_.erase(curr);

    TimerList::iterator it = upper_bound(timer_list_.begin(),
                                         timer_list_.end(),
                                         curr_timer, timer_cmp_func);
    timer_list_.insert(it, curr_timer);

    return curr_timer;
}

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

    if (timer->Init(time_gap, timer_id) != 0)
    {
        LOG(ERROR)<<"Init timer failed";
        delete timer;
        return NULL;
    }

    timer->Start();

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

    // remove from vector
    // 由于目前定时器一般不会太多，临时采用遍历的方式
    for (TimerList::iterator it = timer_list_.begin();
         it != timer_list_.end(); ++it)
    {
        if (*it != NULL)
        {
            if (*it == timer && timer->timer_id() == (*it)->timer_id())
            {
                delete timer;
                timer_list_.erase(it);
            }
        }
    }

    return 0;
}

}
