#ifndef RAPIDAPP_TIMER_MGR_H_
#define RAPIDAPP_TIMER_MGR_H_

#include "rapidapp_timer.h"
#include <vector>

namespace rapidapp {

/*
 * STL 默认alloc在资源不够时，是会抛出异常的，后续需要加强对异常的处理
 *
 * */
typedef EasyTimer* LPEASYTIMER;
typedef std::vector<EasyTimer*> TimerList;

const int DEFAULT_MAX_TIMER_NUM = 1024 * 1024;

class AppFrameWork;
class TimerMgr {
    public:
        TimerMgr();
        ~TimerMgr();

    public:
        int Init(size_t timer_max);
        void CleanUp();

    public:
        EasyTimer* AddTimer(size_t time, int timer_id);
        int RemoveTimer(const EasyTimer* timer);

    private:
        size_t max_timer_num_;
        TimerList timer_list_;
};

}

#endif
