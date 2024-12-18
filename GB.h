/*
GB class function signatures
*/

#ifndef GB_H
#define GB_H

// Include libraries
#include <SFML/Graphics.hpp>
// Include local header files
#include "CPU.h"
//#include "PPU.h"

// GB class
class GB {
  private:
    //sf::RenderWindow* win;
    //sf::Image* scrn_img;
    //sf::Texture* scrn_tex;
    //sf::Sprite* scrn_spr;
    CPU* cpu;
    //PPU* ppu;
    uint8_t* mem_map;
  public:
    // Create GB object
    GB();
    // Emulator loop
    void emuLoop();
    // Get input from keyboard
    void getInput();
    // Render screen
    void renderScreen();
    // Delete all GB related objects
    ~GB();
};

#endif