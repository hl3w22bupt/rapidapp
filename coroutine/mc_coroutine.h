#ifndef MC_COROUTINE_H_
#define MC_COROUTINE_H_

#include <ucontext.h>
#include <tr1/unordered_map>

namespace MagicCube {

typedef int (*coroutine_func)(void*);

typedef struct CoroutineCtx {
    ucontext_t ctx;       // 当前协程上下文
    int state;            // 当前协程状态
    int yield;            // 传递给其它ctx的参数
    coroutine_func func;  // 协程回调函数
    void *arg;            // 协程回调函数参数
} COROUTINECTX;

typedef std::tr1::unordered_map<int, COROUTINECTX*> CoroutineTable;

class CoroutineScheduler {
    public:

        /// @brief  创建协程实例
        ///
        /// @return
        int CreateCoroutine();

        /// @brief  销毁协程实例
        void DestroyCoroutine();

        /// @brief  协程让出CPU占用权
        ///
        /// @param  uct_id
        ///
        /// @return
        int YieldCoroutine(int uct_id);

        /// @brief  协程抢占CPU占用权
        ///
        /// @param  uct_id
        ///
        /// @return
        int ResumeCoroutine(int uct_id);

    private:
        int running_cid_;
        ucontext_t main_ctx_;

        size_t stack_size_;
        int coroutine_max_;
        struct CoroutineCtx* coroutine_list_;
        CoroutineTable coroutine_table_;
};

}

#endif
