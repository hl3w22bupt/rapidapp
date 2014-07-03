#ifndef MC_COROUTINE_H_
#define MC_COROUTINE_H_

namespace MagicCube {

class Coroutine {
    public:
        Coroutine();
        ~Coroutine();

        int Yield();
        int Resume();
};

class CoroutineSchedulerImp;
class CoroutineScheduler {
    public:
        int CreateCoroutine();
        void DestroyCoroutine();

    private:
        CoroutineSchedulerImp* pimpl_;
};

}

#endif
