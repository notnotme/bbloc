#include "ApplicationWindow.h"


int main(int argc, char *argv[]) {
    ApplicationWindow window;
    window.create("bbloc", 1280, 720);
    window.mainLoop();
    window.destroy();
    return EXIT_SUCCESS;
}