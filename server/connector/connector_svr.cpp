#include "rapidapp.h"
#include "connector_app.h"

int main(int argc, char** argv)
{
    AppLauncher launcher;
    ConnectorApp myapp;

    launcher.Run(&myapp, argc, argv);

    return 0;
}
