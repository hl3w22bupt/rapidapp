#ifndef MC_COROUTINE_H_
#define MC_COROUTINE_H_

#include <ucontext.h>
#include <tr1/unordered_map>

namespace magic_cube {

enum CoroutineState {
    COROUTINE_READY   = 0,
    COROUTINE_RUNNING = 1,
    COROUTINE_SUSPEND = 2,
    COROUTINE_DONE    = 3,
};

typedef int (*coroutine_func)(void*);

typedef struct CoroutineCtx {
    ucontext_t uctx;      // 当前协程上下文
    int idx;              // 当前协程idx
    int state;            // 当前协程状态
    int yield;            // 传递给其它ctx的参数
    coroutine_func func;  // 协程回调函数
    void *arg;            // 协程回调函数参数
    char stack[1];
} COROUTINECTX;

typedef std::tr1::unordered_map<int, COROUTINECTX*> CoroutineTable;

class CoroutineScheduler {
    public:
        CoroutineScheduler();
        CoroutineScheduler(int cocurrent_num, int stack_size);
        ~CoroutineScheduler();

    public:
        /// @brief  创建协程实例
        ///
        /// @return
        int CreateCoroutine(coroutine_func func, void* arg);

        /// @brief  销毁协程实例
        void DestroyCoroutine(int uct_id);

        /// @brief  协程让出CPU占用权
        ///
        /// @return
        int YieldCoroutine();

        /// @brief  协程抢占CPU占用权
        ///
        /// @param  uct_id
        ///
        /// @return
        int ResumeCoroutine(int uct_id);

        /// @brief  协程是否存活
        ///
        /// @return
        bool CoroutineBeenAlive(int uct_id);

    private:
        static void ScheduleFunction(void* arg);

        // noncopyable
    private:
        CoroutineScheduler(const CoroutineScheduler&);
        CoroutineScheduler& operator=(const CoroutineScheduler&);

    private:
        int running_cid_;                       // 当前运行coroutine id
        ucontext_t main_ctx_;                   // 主coroutine

        size_t stack_size_;                     // coroutine stack大小
        int coroutine_max_;                     // 最大同时支持协程数
        CoroutineTable coroutine_table_;        // 协程表
};

}

#endif
