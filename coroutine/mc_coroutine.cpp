#include "mc_coroutine.h"
#include <cstdlib>
#include <cassert>

namespace magic_cube {

const int kDefaultCoCurrentNum = 10 * 1024;
const int kDefaultStackSize = 1024;

CoroutineScheduler::CoroutineScheduler() :
    running_cid_(-1), stack_size_(kDefaultStackSize),
    coroutine_max_(kDefaultCoCurrentNum), coroutine_table_()
{}

CoroutineScheduler::CoroutineScheduler(int cocurrent_num, int stack_size) :
    running_cid_(-1), stack_size_(stack_size),
    coroutine_max_(cocurrent_num), coroutine_table_()
{}

CoroutineScheduler::~CoroutineScheduler()
{
    CoroutineTable::iterator it = coroutine_table_.begin();
    for (; it != coroutine_table_.end(); ++it)
    {
        if (it->second != NULL)
        {
            free(it->second);
            it->second = NULL;
        }
    }

    coroutine_table_.clear();
}

int CoroutineScheduler::CreateCoroutine(coroutine_func func, void* arg)
{
    if (NULL == func)
    {
        return -1;
    }

    if (coroutine_table_.size() >= coroutine_max_)
    {
        return -1;
    }

    static int cid_seed = 0;

    COROUTINECTX* ctx = static_cast<COROUTINECTX*>(calloc(1, sizeof(COROUTINECTX) + stack_size_));
    if (NULL == ctx)
    {
        return -1;
    }

    ctx->state = COROUTINE_READY;
    ctx->idx = cid_seed;
    ctx->func = func;
    ctx->arg = arg;

    coroutine_table_[cid_seed] = ctx;

    return cid_seed++;
}

void CoroutineScheduler::DestroyCoroutine(int uct_id)
{
    CoroutineTable::iterator it = coroutine_table_.find(uct_id);
    if (it == coroutine_table_.end())
    {
        return;
    }

    if (it->second != NULL)
    {
        free(it->second);
    }

    coroutine_table_.erase(it);
}

int CoroutineScheduler::YieldCoroutine()
{
    if (running_cid_ < 0)
    {
        return 0;
    }

    CoroutineTable::iterator it = coroutine_table_.find(running_cid_);
    if (it == coroutine_table_.end())
    {
        return -1;
    }

    COROUTINECTX* ctx = it->second;
    if (NULL == ctx)
    {
        return -1;
    }

    running_cid_ = -1;
    ctx->state = COROUTINE_SUSPEND;
    swapcontext(&ctx->uctx, &main_ctx_);

    return 0;
}

void CoroutineScheduler::ScheduleFunction(void* arg)
{
    if (NULL == arg)
    {
        return;
    }

    CoroutineScheduler* the_handler = static_cast<CoroutineScheduler*>(arg);
    int curr_cid = the_handler->running_cid_;
    assert(curr_cid >= 0);

    COROUTINECTX* ctx = the_handler->coroutine_table_[curr_cid];
    assert(ctx != NULL);

    ctx->func(ctx->arg);
    ctx->state = COROUTINE_DONE;
    the_handler->running_cid_ = -1;
}

int CoroutineScheduler::ResumeCoroutine(int uct_id)
{
    CoroutineTable::iterator it = coroutine_table_.find(uct_id);
    if (it == coroutine_table_.end())
    {
        return -1;
    }

    COROUTINECTX* ctx = it->second;
    if (NULL == ctx)
    {
        return -1;
    }

    switch(ctx->state)
    {
        case COROUTINE_READY:
            {
                getcontext(&ctx->uctx);
                ctx->uctx.uc_stack.ss_sp = ctx->stack;
                ctx->uctx.uc_stack.ss_size = stack_size_;
                // succssor context
                ctx->uctx.uc_link = &main_ctx_;
                ctx->state = COROUTINE_RUNNING;
                running_cid_ = ctx->idx;

                makecontext(&ctx->uctx, (void(*)())ScheduleFunction, 1, this);
                swapcontext(&main_ctx_, &ctx->uctx);
                break;
            }
        case COROUTINE_SUSPEND:
            {
                running_cid_ = ctx->idx;
                ctx->state = COROUTINE_RUNNING;
                swapcontext(&main_ctx_, &ctx->uctx);
                break;
            }
        case COROUTINE_RUNNING:
            {
                break;
            }
        case COROUTINE_DONE:
            {
                DestroyCoroutine(uct_id);
                break;
            }
    }

    return 0;
}

bool CoroutineScheduler::CoroutineBeenAlive(int uct_id)
{
    CoroutineTable::iterator it = coroutine_table_.find(uct_id);
    if (it == coroutine_table_.end())
    {
        return false;
    }

    COROUTINECTX* ctx = it->second;
    if (NULL == ctx)
    {
        return false;
    }

    return (ctx->state != COROUTINE_DONE) ? true: false;
}

}
