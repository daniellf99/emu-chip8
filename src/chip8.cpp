#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include "chip8.h"

namespace chip8 {
    std::string LIB_NAME = "Emu-Chip8";
    std::string LIB_VERSION = "0.1.0";

    constexpr int MEMORY_SIZE_BYTES = 4096;

    /*
        The first CHIP-8 interpreter (on the COSMAC VIP computer) was also located in RAM, from address 000 to 1FF. 
        It would expect a CHIP-8 program to be loaded into memory after it, starting at address 0x200
    */
    constexpr uint16_t PROGRAM_START_ADDRESS = 0x200;

    std::uint16_t program_counter = 0;
    std::uint16_t i_register = 0;
    std::uint8_t delay_timer = 0;
    std::uint8_t sound_timer = 0;

    std::array<std::uint8_t, MEMORY_SIZE_BYTES> memory {};
    std::array<std::array<std::uint8_t, SCREEN_WIDTH>, SCREEN_HEIGHT> display {}; // 64w X 32h Display

    std::array<std::uint8_t, 16> registers{};
    std::vector<int> stack(0); // TODO size?

    //  For some reason, it’s become popular to put fonts at 050–09F. We will follow this "convention". TODO

    const char *get_lib_name() {return LIB_NAME.c_str();};
    const char *get_lib_version() {return LIB_VERSION.c_str();};

    // Decompiler
    bool check_instruction(std::uint16_t inst, std::uint16_t target, std::uint16_t mask) 
    {
        return ((inst & mask) ^ target) == 0;
    }


    void decompile() 
    {
        // Given a rom file, disassemble instructions and print them to the console
        std::cout << "-- BEGIN DECOMPILATION --\n";
        char instruction[2] = {0};
        uint16_t current_address = 0;
        std::ifstream input("roms/IBM Logo.ch8", std::ios::binary);

        while (input.read(instruction, 2)) {
            std::uint16_t first_byte = static_cast<std::uint16_t>(static_cast<unsigned char>(instruction[0])) << 8;
            std::uint16_t second_byte = static_cast<std::uint16_t>(static_cast<unsigned char>(instruction[1]));

            std::uint16_t current_instruction = first_byte | second_byte;

            printf("0x%04x  0x%04x ", current_address, current_instruction); // Print instruction address and value as hex
            current_address += 2; // Advance to next address

            if (current_instruction == 0x00E0) {
                // CLS - Clear screen
                std::cout << "CLS\n";

            } else if (check_instruction(current_instruction, 0xA000, 0xF000)) {
                // LD I, nnn - Load from address into register I
                std::uint16_t address = current_instruction & 0x0FFF;
                std::cout << "LD I, ";
                printf("0x%04x\n", address);

            } else if (check_instruction(current_instruction, 0x1000, 0xF000)) {
                // JP - Jump to address
                std::uint16_t address = current_instruction & 0x0FFF;
                printf("JP 0x%04x\n", address);

            } else if (current_instruction == 0x00EE) {
                // RET - Return from subroutine
                std::cout << "RET\n";

            } else if (check_instruction(current_instruction, 0x6000, 0xF000)) {
                // LD - Load literal
                std::uint8_t reg = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
                std::uint8_t value = static_cast<std::uint8_t>(current_instruction & 0x00FF);
                std::cout << "LD V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

            } else if (check_instruction(current_instruction, 0xD000, 0xF000)) {
                // DRW - Draw
                std::uint8_t reg_x = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
                std::uint8_t reg_y = static_cast<std::uint8_t>((current_instruction & 0x00F0) >> 4);
                std::uint8_t nibble = static_cast<std::uint8_t>(current_instruction & 0x000F);
                std::cout << "DRW V" << unsigned(reg_x) << ", V" << unsigned(reg_y) << ", ";
                printf("0x%01x\n", nibble);

            } else if (check_instruction(current_instruction, 0x7000, 0xF000)) {
                // ADD - Add value
                std::uint8_t reg = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
                std::uint8_t value = static_cast<std::uint8_t>(current_instruction & 0x00FF);
                std::cout << "ADD V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

            } else if (check_instruction(current_instruction, 0x3000, 0xF000)) {
                // SE - Skip next instruction on condition
                std::uint8_t reg = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
                std::uint8_t value = static_cast<std::uint8_t>(current_instruction & 0x00FF);
                std::cout << "SE V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

            } else if (check_instruction(current_instruction, 0x0, 0xF000)) {
                // SYS - Jump to a machine code routine
                std::uint16_t address = current_instruction & 0x0FFF;
                printf("SYS 0x%04x\n", address);

            } else if (check_instruction(current_instruction, 0xB000, 0xF000)) {
                // JP - Jump to location nnn + V0
                std::uint16_t address = current_instruction & 0x0FFF;
                printf("JP V0, 0x%04x\n", address);

            } else if (check_instruction(current_instruction, 0xC000, 0xF000)) {
                // RND - Set random value
                std::uint8_t reg = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
                std::uint8_t value = static_cast<std::uint8_t>(current_instruction & 0x00FF);
                std::cout << "RND V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

            } else if (check_instruction(current_instruction, 0x4000, 0xF000)) {
                // SNE - Skip next instruction on condition
                std::uint8_t reg = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
                std::uint8_t value = static_cast<std::uint8_t>(current_instruction & 0x00FF);
                std::cout << "SNE V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

            } else if (check_instruction(current_instruction, 0x8000, 0xF00F)) {
                // LD - Load from register to register
                std::uint8_t reg_x = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
                std::uint8_t reg_y = static_cast<std::uint8_t>((current_instruction & 0x00F0) >> 4);
                std::cout << "LD V" << unsigned(reg_x) << ", V" << unsigned(reg_y) << "\n";

            }
            else {
                std::cout << "NOOP?\n";
            }
        }

        std::cout << "-- END DECOMPILATION --\n";
    }


    void dump_memory() 
    {
        std::ofstream output("out/memory-dump.hex", std::ios::binary | std::ios::out);

        for (uint16_t address = 0; address < MEMORY_SIZE_BYTES; address++) {
            output.put(static_cast<char>(static_cast<unsigned char>(memory.at(address))));
        }

        output.close();
    }


    void dump_display()
    {
        std::ofstream output("out/display-dump.txt", std::ios::out);

        for (unsigned i=0; i < SCREEN_HEIGHT; i++) {
            for (unsigned j=0; j < SCREEN_WIDTH; j++) {
                output << static_cast<unsigned int>(display.at(i).at(j));
            }
            output << "\n";
        }

        output.close();
    }

    void display_registers()
    {
        std::cout << "V0=" << unsigned(registers.at(0)) << " ";
        std::cout << "V1=" << unsigned(registers.at(1)) << " ";
        std::cout << "I=" << unsigned(i_register) << " ";
        std::cout << "PC=" << unsigned(program_counter) << " ";
        std::cout << "\n";
    }


    void fetch_decode_execute() 
    {
        // Fetch instruction that PC is pointing to
        std::array<std::uint8_t, 2> raw_instruction;
        raw_instruction.at(0) = memory.at(program_counter);
        raw_instruction.at(1) = memory.at(program_counter+1);
        program_counter += 2;

        // Decode & Execute
        std::uint16_t instruction = (raw_instruction.at(0) << 8) | raw_instruction.at(1);

        std::uint8_t second_nibble = static_cast<std::uint8_t>((instruction & 0x0F00) >> 8);
        std::uint8_t third_nibble = static_cast<std::uint8_t>((instruction & 0x00F0) >> 4);
        std::uint8_t fourth_nibble = static_cast<std::uint8_t>((instruction & 0x000F));
        std::uint8_t second_byte = static_cast<std::uint8_t>(instruction & 0x00FF);
        std::uint16_t address_param = instruction & 0x0FFF;
        
        if (instruction == 0x00E0) {
            // CLS - Clear screen
            std::cout << "CLS\n";
            for (unsigned i=0; i < SCREEN_HEIGHT; i++) {
                for (unsigned j=0; j < SCREEN_WIDTH; j++) {
                    display.at(i).at(j) = 0;
                }
            }

        } else if (check_instruction(instruction, 0xA000, 0xF000)) {
            // LD - Load from address into register I
            i_register = address_param;
            std::cout << "LD\n";

        } else if (check_instruction(instruction, 0x1000, 0xF000)) {
            // JP - Jump to address
            program_counter = address_param;
            std::cout << "JP\n";

        } else if (instruction == 0x00EE) {
            // RET - Return from subroutine
            std::cout << "RET\n";

        } else if (check_instruction(instruction, 0x6000, 0xF000)) {
            // LD - Load literal/Set register Vx
            registers.at(second_nibble) = second_byte;
            std::cout << "LD literal\n";

        } else if (check_instruction(instruction, 0xD000, 0xF000)) {
            // DRW - Draw
            auto x = registers.at(second_nibble);
            auto y = registers.at(third_nibble);
            auto bytes_to_read = fourth_nibble;
            
            // 0,0 coords are at the top left of the screen
            for (unsigned int i=0; i<bytes_to_read; i++) {
                auto sprite = memory.at(i_register+i);

                for (unsigned int j=0; j<8; j++) {
                    display.at(y+i).at(x+j) = display.at(y+i).at(x+j) ^ ((sprite >> (7-j)) & 0x1);
                }
            }

            std::cout << "DRW V" << unsigned(second_nibble) << ", V" << unsigned(third_nibble) << ", ";
            printf("0x%01x\n", fourth_nibble);

        } else if (check_instruction(instruction, 0x7000, 0xF000)) {
            // ADD - Add value
            registers.at(second_nibble) += second_byte;
            std::cout << "ADD\n";

        } else if (check_instruction(instruction, 0x3000, 0xF000)) {
            // SE - Skip next instruction on condition
            std::uint8_t reg = static_cast<std::uint8_t>((instruction & 0x0F00) >> 8);
            std::uint8_t value = static_cast<std::uint8_t>(instruction & 0x00FF);
            std::cout << "SE V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

        } else if (check_instruction(instruction, 0x0, 0xF000)) {
            // SYS - Jump to a machine code routine
            std::uint16_t address = instruction & 0x0FFF;
            printf("SYS 0x%04x\n", address);

        } else if (check_instruction(instruction, 0xB000, 0xF000)) {
            // JP - Jump to location nnn + V0
            std::uint16_t address = instruction & 0x0FFF;
            printf("JP V0, 0x%04x\n", address);

        } else if (check_instruction(instruction, 0xC000, 0xF000)) {
            // RND - Set random value
            std::uint8_t reg = static_cast<std::uint8_t>((instruction & 0x0F00) >> 8);
            std::uint8_t value = static_cast<std::uint8_t>(instruction & 0x00FF);
            std::cout << "RND V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

        } else if (check_instruction(instruction, 0x4000, 0xF000)) {
            // SNE - Skip next instruction on condition
            std::uint8_t reg = static_cast<std::uint8_t>((instruction & 0x0F00) >> 8);
            std::uint8_t value = static_cast<std::uint8_t>(instruction & 0x00FF);
            std::cout << "SNE V" << unsigned(reg) << ", #" << unsigned(value) << "\n";

        } else if (check_instruction(instruction, 0x8000, 0xF00F)) {
            // LD - Load from register to register
            std::uint8_t reg_x = static_cast<std::uint8_t>((instruction & 0x0F00) >> 8);
            std::uint8_t reg_y = static_cast<std::uint8_t>((instruction & 0x00F0) >> 4);
            std::cout << "LD V" << unsigned(reg_x) << ", V" << unsigned(reg_y) << "\n";

        }
        else {
            std::cout << "NOOP?\n";
        }
    }

    void load_rom(const uint8_t *data, size_t size)
    {
        // Copy program into memory, starting at the default start address
        std::uint16_t address = PROGRAM_START_ADDRESS;
        
        // TODO Check for size violation
        std::memcpy(&memory.at(address), data, size);

        program_counter = PROGRAM_START_ADDRESS;
    }

    void unload_rom()
    {
        reset();
    }

    void reset()
    {
        program_counter = 0;
        i_register = 0;
        delay_timer = 0;
        sound_timer = 0;
        registers.fill(0);
        memory.fill(0);
    }
}