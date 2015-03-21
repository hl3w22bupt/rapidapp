#ifndef RAPIDAPP_CORE_H_
#define RAPIDAPP_CORE_H_

#include <glog/logging.h>

namespace rapidapp {

class AppFrameWork;
class RapidApp;

class AppLauncher {
    public:
        AppLauncher();
        ~AppLauncher();

    public:
        int Run(RapidApp* app, int argc, char** argv);

        AppFrameWork* easy_framework_;
};

}

#endif
