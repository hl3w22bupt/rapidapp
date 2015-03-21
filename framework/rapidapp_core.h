#ifndef RAPIDAPP_CORE_H_
#define RAPIDAPP_CORE_H_

#include "rapidapp_framework_imp.h"

namespace rapidapp {

class AppLauncher {
    public:
        AppLauncher();
        ~AppLauncher();

    public:
        int Run(RapidApp* app, int argc, char** argv);

        class AppFrameWork easy_framework_;
};

}

#endif
