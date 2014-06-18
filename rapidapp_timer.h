#ifndef RAPIDAPP_TIMER_H_
#define RAPIDAPP_TIMER_H_

#include <time.h>

namespace rapidapp {

class TimerMgr;
class EasyTimer {
    public:
        EasyTimer();
        ~EasyTimer();

    public:
        int Init(size_t time, int timer_id);
        void CleanUp();

    public:
        int Start();
        int Terminate();

    public:
        // 定时器描点
        inline void Step() {
            next_active_timestamp_ += time_step_;
        }

    public:
        inline time_t timestamp() const {
            return next_active_timestamp_;
        }

        inline int timer_id() const {
            return timer_id_;
        }

    private:
        time_t next_active_timestamp_;  // 下一次活动时间
        size_t time_step_;              // 定时器步长
        int timer_id_;                  // 计时器id标识

    private:
        int list_idx_;                  // 链表中索引位置，加入到链表后分配
        friend class TimerMgr;
};

}

#endif
