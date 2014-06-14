#ifndef RAPIDAPP_TIMER_MGR_H_
#define RAPIDAPP_TIMER_MGR_H_

#include "rapidapp_timer.h"
#include <vector>

namespace rapidapp {

class AppFrameWork;
class TimerMgr {
    public:
        TimerMgr();
        ~TimerMgr();

    public:
        int AddTimer();
        int RemoveTimer();
};

}

#endif
