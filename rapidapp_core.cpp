#include "rapidapp_core.h"
#include <glog/logging.h>

namespace rapidapp {

AppLauncher::AppLauncher() : easy_framework_()
{
}

AppLauncher::~AppLauncher()
{
}

int AppLauncher::Run(RapidApp* app, int argc, char** argv)
{
    if (NULL == app)
    {
        fprintf(stderr, "app is NULL\n");
        return -1;
    }

    if (argc <=0 || NULL == argv)
    {
        fprintf(stderr, "argc:%d, argv:%p\n", argc, argv);
        return -1;
    }

    // 1. Init
    int ret = easy_framework_.Init(app, argc, argv);
    if (ret != 0)
    {
        PLOG(ERROR)<<"Init failed, return"<<ret;
        return -1;
    }

    // 2. app init
    ret = app->OnInit();
    if (ret != 0)
    {
        PLOG(ERROR)<<"app OnInit failed return:"<<ret;
        return -1;
    }

    LOG(INFO)<<"start success...";

    // 3. mainloop
    easy_framework_.MainLoop();

    // 4. app fini
    ret = app->OnFini();
    if (ret != 0)
    {
        PLOG(ERROR)<<"app OnFini failed return:"<<ret;
    }

    // 5. CleanUp
    ret = easy_framework_.CleanUp();
    if (ret != 0)
    {
        PLOG(ERROR)<<"CleanUp failed return:"<<ret;
    }

    return 0;
}

}
