#include "gui/gui.h"
#include <iostream>
#include <drm_fourcc.h>
#include <cstring>
#include <cstdio>

int main(void) {
    gui::DisplayManager::the();

    drm::ScreenBitmap screen;
    screen.fill(style::Colour::white());

    drm::Bitmap bmp {100, 100};
    bmp.fill(style::Colour::blue(0x7F));

    drm::Bitmap bmp2 {500, 50};
    bmp2.fill(style::Colour::red(0x7F));

    bmp2.render(bmp, 10, -20000);
    std::cerr << "HERE" << std::endl;
    bmp.render(screen, 200, 200);

    bmp2.fill(style::Colour::red(0x7F));
    bmp2.render(screen, 0, 0);

    screen.render();

    while(1);
}

