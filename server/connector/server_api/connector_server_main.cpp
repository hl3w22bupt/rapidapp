#include "rapidapp.h"
#include "connector_server_demo.h"

int main(int argc, char** argv)
{
    AppLauncher launcher;
    ServerDemoApp myapp;

    launcher.Run(&myapp, argc, argv);

    return 0;
}
