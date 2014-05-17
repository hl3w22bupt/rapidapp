#include "rapidapp.h"
#include "rapidapp_imp.h"

int main()
{
    AppLauncher launcher;
    MyApp myapp;

    launcher.Run(&myapp);

    return 0;
}
