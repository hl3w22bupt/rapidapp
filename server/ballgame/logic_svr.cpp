#include "rapidapp.h"
#include "logic_app.h"

int main(int argc, char** argv)
{
    AppLauncher launcher;
    LogicApp myapp;

    launcher.Run(&myapp, argc, argv);

    return 0;
}
