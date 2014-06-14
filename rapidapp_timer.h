#ifndef RAPIDAPP_TIMER_H_
#define RAPIDAPP_TIMER_H_

namespace rapidapp {

class EasyTimer {
    public:
        EasyTimer();
        ~EasyTimer();

    public:
        int Init();
        void CleanUp();

    public:
        int Start();
        int Terminate();

    private:
        time_t last_active_timestamp;   // 上一次活动事件
        int timer_id;                   // 计时器id标识
};

}

#endif
