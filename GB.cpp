/*
GB class function definitions
*/

// Include libraries
//#include <SFML/Graphics.hpp>
#include <fstream>
// Include local header files
#include "GB.h"
#include "CPU.h"
//#include "PPU.h"

// Create GB object
GB::GB() {
  // Initialize memory map
  mem_map = new uint8_t[65536];
  mem_map[0] = 0;

  // Reading Boot ROM into memory
  FILE *rom_ptr = 0;
  // Open the file (remember to open as bytes "rb")
  rom_ptr = fopen("C:/Users/BellwetherWealth-Enq/Games/ROMs/GB/[BIOS] Nintendo Game Boy Boot ROM (World) (Rev 1).gb", "rb");
  // File ptr, offset, where offset added (SEEK_SET, SEEK_CUR, SEEK_END)
  fseek(rom_ptr, 0, SEEK_SET); // Set place in file to read from
  // Memory ptr, size of each element (in bytes), number of elements, file ptr
  fread(mem_map, 1, 256, rom_ptr); // Reads Boot ROM into start of mem_map
  fclose(rom_ptr); // Close to prevent issues
  
  //  Reading cartridge into memory (first 32kb minus first 256b that Boot ROM
  // is in, will overwrite after boot process)
  rom_ptr = 0;
  // Open the file (remember to open as bytes "rb")
  rom_ptr = fopen("C:/Users/BellwetherWealth-Enq/Games/ROMs/GB/Dr. Mario (World).gb", "rb");
  // File ptr, offset, where offset added (SEEK_SET, SEEK_CUR, SEEK_END)
  fseek(rom_ptr, 0x100, SEEK_SET); // Set place in file to read from
  // Memory ptr, size of each element (in bytes), number of elements, file ptr
  fread(mem_map + 0x100, 1, 32512, rom_ptr); // Reads ROM into after Boot ROM
  fclose(rom_ptr); // Close to prevent issues

  // Create CPU
  cpu = new CPU;
}

// Emulator loop
void GB::emuLoop() {
  // Run CPU for 1000000 cycles (testing)
  cpu->cpuLoop(mem_map, 1000000);
}

// Get input from keyboard
void GB::getInput() {};

// Render screen
void GB::renderScreen() {};

// Delete all GB related objects
GB::~GB() {};