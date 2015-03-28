#include "rapidapp.h"
#include "client_demo_app.h"

int main(int argc, char** argv)
{
    AppLauncher launcher;
    ClientDemoApp myapp;

    launcher.Run(&myapp, argc, argv);

    return 0;
}
