#include "rapidapp_ctrl_cmd_keyword.h"

namespace rapidapp {

class HelpCommand : public ICommandEventListener {
    public:
        HelpCommand(){};
        virtual ~HelpCommand(){};

    public:
        void OnCommand(int argc, char** argv) {
            // TODO
        }
};

void AppControlDispatcher::AddDefaultSupportedCommand()
{}

}
