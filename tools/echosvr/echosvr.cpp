#include "rapidapp.h"
#include "rapidapp_imp.h"

int main(int argc, char** argv)
{
    AppLauncher launcher;
    MyApp myapp;

    launcher.Run(&myapp, argc, argv);

    return 0;
}
