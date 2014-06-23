#ifndef RAPIDAPP_EASY_ACTION_H_
#define RAPIDAPP_EASY_ACTION_H_

namespace rapidapp {

class IActionable {
    public:
        IActionable(){}
        virtual ~IActionable(){}

    public:
        virtual int OnActionPerform() {return 0;}
        virtual int OnActionDone() {return 0;}
};

class EasyAction {
    public:
        EasyAction();
        ~EasyAction();

    public:
        // 目前需要上层触发
        int Perform();
        int Done();

    public:
        int AddSubAction(EasyAction* sub_action);

    private:
        static int action_id_seed_;        // action id种子生成器

    private:
        EasyAction* sub_action_;       // sub action，每个action只支持1个sub action

        IActionable* action_cb_func_;  // 每个action的具体动作内容
        int action_id_;                // 每个action的唯一标识
};

}

#endif
