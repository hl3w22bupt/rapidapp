#include "rapidapp.h"
#include "server_demo_app.h"

int main(int argc, char** argv)
{
    AppLauncher launcher;
    ServerDemoApp myapp;

    launcher.Run(&myapp, argc, argv);

    return 0;
}
