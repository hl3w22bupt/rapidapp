#include "rapidapp_ctrl_cmd_keyword.h"

namespace rapidapp {

class HelpCommand : public ICommandEventListener {
    public:
        HelpCommand(){};
        virtual ~HelpCommand(){};

    public:
        const char** OnCommand(int argc, char** argv) {
            static const char* out[] = {
                "{getrundata: \"get runtime statistic data\"}",
            };

            return out;
        }

        const char* GetCmdName() {
            return "help";
        }
};

void AppControlDispatcher::AddDefaultSupportedCommand()
{
    static HelpCommand help;
    AddSupportedCommand(&help);
}

}
