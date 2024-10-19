#include <cstdint>
#include <iostream>
#include <fstream>
#include "chip8.h"

int main()
{
    // First of all, we print the program instructions in order to debug better
    chip8::decompile();
    
    // Now, we begin the emulation
    chip8::load_rom();

    while (true)
    {
        chip8::fetch_decode_execute();
        chip8::dump_memory();
        chip8::dump_display();
        chip8::display_registers();
    }

    return 0;
}
