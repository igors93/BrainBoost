#include "app/Application.h"

int main(int, char**) {
    Application app;
    if (!app.init()) {
        app.shutdown();
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
