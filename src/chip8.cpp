#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <sstream>
#include <sys/types.h>
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
    std::uint32_t global_cycle_number = 0;

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


    void fetch_decode_execute(unsigned int cycles)
    {
        for (unsigned int curr_cycle = 1; curr_cycle <= cycles; curr_cycle++)
        {
            global_cycle_number++;

            if (global_cycle_number % 12 == 0) {
                if (delay_timer > 0) delay_timer--;
                if (sound_timer > 0) sound_timer--;
            }

            // Fetch instruction that PC is pointing to
            std::array<std::uint8_t, 2> raw_instruction;
            raw_instruction.at(0) = memory.at(program_counter);
            raw_instruction.at(1) = memory.at(program_counter+1);

            // Decode & Execute
            std::uint16_t instruction = (raw_instruction.at(0) << 8) | raw_instruction.at(1);

            std::uint8_t x = static_cast<std::uint8_t>((instruction & 0x0F00) >> 8);
            std::uint8_t y = static_cast<std::uint8_t>((instruction & 0x00F0) >> 4);
            std::uint8_t nibble = static_cast<std::uint8_t>((instruction & 0x000F));
            std::uint8_t kk = static_cast<std::uint8_t>(instruction & 0x00FF);
            std::uint16_t address_param = instruction & 0x0FFF;

            printf("0x%04x 0x%04x ", program_counter, instruction);

            program_counter += 2;
            
            if (instruction == 0x00E0) {
                // CLS - Clear screen
                std::cout << "CLS\n";
                for (unsigned i=0; i < SCREEN_HEIGHT; i++) {
                    display.at(i).fill(0);
                }

            } else if (instruction == 0x00EE) {
                // 00EE - RET
                // Return from a subroutine.
                program_counter = stack.back();
                stack.pop_back();
                std::cout << "RET\n";

            } else if (check_instruction(instruction, 0x0000, 0xF000)) {
                // 0nnn - SYS addr
                // Jump to a machine code routine at nnn.
                // This instruction is only used on the old computers on which Chip-8 was originally implemented. It is ignored by modern interpreters.
                printf("SYS 0x%04x (NOOP)\n", address_param);

            } else if (check_instruction(instruction, 0x1000, 0xF000)) {
                // 1nnn - JP addr
                // Jump to location nnn.
                program_counter = address_param;
                printf("JP 0x%04x\n", address_param);
            
            } else if (check_instruction(instruction, 0x2000, 0xF000)) {
                // 2nnn - CALL addr
                // Call subroutine at nnn.
                stack.push_back(program_counter);
                program_counter = address_param;
                printf("CALL 0x%04x\n", address_param);

            } else if (check_instruction(instruction, 0x3000, 0xF000)) {
                // 3xkk - SE Vx, byte
                // Skip next instruction if Vx = kk.
                if (registers.at(x) == kk) 
                {
                    program_counter += 2;
                }
                std::cout << "SE V" << unsigned(x) << ", #" << unsigned(kk) << "\n";
            
            } else if (check_instruction(instruction, 0x4000, 0xF000)) {
                // 4xkk - SNE Vx, byte
                // Skip next instruction if Vx != kk.
                if (registers.at(x) != kk)
                {
                    program_counter += 2;
                }
                std::cout << "SNE V" << unsigned(x) << ", #" << unsigned(kk) << "\n";


            } else if (check_instruction(instruction, 0x5000, 0xF00F)) {
                // 5xy0 - SE Vx, Vy
                // Skip next instruction if Vx = Vy.
                if (registers.at(x) == registers.at(y))
                {
                    program_counter += 2;
                }

            } else if (check_instruction(instruction, 0x6000, 0xF000)) {
                // 6xkk - LD Vx, byte
                // Set Vx = kk.
                registers.at(x) = kk;
                printf("LD V%u, 0x%02x\n", x, kk);

            } else if (check_instruction(instruction, 0x7000, 0xF000)) {
                // 7xkk - ADD Vx, byte
                // Set Vx = Vx + kk.
                registers.at(x) += kk;
                printf("ADD V%u, 0x%02x\n", x, kk);

            } else if (check_instruction(instruction, 0x8000, 0xF00F)) {
                // LD - Load from register to register
                std::uint8_t reg_x = static_cast<std::uint8_t>((instruction & 0x0F00) >> 8);
                std::uint8_t reg_y = static_cast<std::uint8_t>((instruction & 0x00F0) >> 4);
                std::cout << "LD V" << unsigned(reg_x) << ", V" << unsigned(reg_y) << "\n";

            } else if (check_instruction(instruction, 0x8001, 0xF00F)) {
                // 8xy1 - OR Vx, Vy
                // Set Vx = Vx OR Vy.
                registers.at(x) |= registers.at(y);
                std::cout << "OR V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x8002, 0xF00F)) {
                // 8xy2 - AND Vx, Vy
                // Set Vx = Vx AND Vy.
                registers.at(x) &= registers.at(y);
                std::cout << "AND V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x8003, 0xF00F)) {
                // 8xy3 - XOR Vx, Vy
                // Set Vx = Vx XOR Vy.
                registers.at(x) ^= registers.at(y);
                std::cout << "XOR V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x8004, 0xF00F)) {
                // 8xy4 - ADD Vx, Vy
                // Set Vx = Vx + Vy, set VF = carry.
                uint16_t result = static_cast<uint16_t>(registers.at(x)) + static_cast<uint16_t>(registers.at(y));
                registers.at(x) = static_cast<uint8_t>(result);
                if ((result & 0xF00) > 0)
                {
                    registers.at(0xF) = 1;
                }   
                std::cout << "ADD V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x8005, 0xF00F)) {
                // 8xy5 - SUB Vx, Vy
                // Set Vx = Vx - Vy, set VF = NOT borrow.
                if (registers.at(x) > registers.at(y))
                {
                    registers.at(0xF) = 1;
                } else {
                    registers.at(0xF) = 0;
                }
                registers.at(x) -= registers.at(y);   
                std::cout << "SUB V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x8006, 0xF00F)) {
                // 8xy6 - SHR Vx {, Vy}
                // Set Vx = Vx SHR 1.
                if ((registers.at(x) & 0x1) == 0x1)
                {
                    registers.at(0xF) = 1;
                } else {
                    registers.at(0xF) = 0;
                }
                registers.at(x) /= 2;   
                std::cout << "SHR V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x8007, 0xF00F)) {
                // 8xy7 - SUBN Vx, Vy
                // Set Vx = Vy - Vx, set VF = NOT borrow.
                if (registers.at(y) > registers.at(x))
                {
                    registers.at(0xF) = 1;
                } else {
                    registers.at(0xF) = 0;
                }
                registers.at(x) = registers.at(y) - registers.at(x);   
                std::cout << "SUBN V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x800E, 0xF00F)) {
                // 8xyE - SHL Vx {, Vy}
                // Set Vx = Vx SHL 1.
                if ((registers.at(x) & 0x80) == 0x80)
                {
                    registers.at(0xF) = 1;
                } else {
                    registers.at(0xF) = 0;
                }
                registers.at(x) *= 2;   
                std::cout << "SHL V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0x9000, 0xF00F)) {
                // 9xy0 - SNE Vx, Vy
                // Skip next instruction if Vx != Vy.
                if (registers.at(x) != registers.at(y))
                {
                    program_counter += 2;
                }   
                std::cout << "SNE V" << unsigned(x) << ", V" << unsigned(y) << "\n";

            } else if (check_instruction(instruction, 0xA000, 0xF000)) {
                // Annn - LD I, addr
                // Set I = nnn.
                i_register = address_param;
                printf("LD I, 0x%04x\n", address_param);

            } else if (check_instruction(instruction, 0xB000, 0xF000)) {
                // Bnnn - JP V0, addr
                // Jump to location nnn + V0.
                program_counter = address_param + static_cast<uint16_t>(registers.at(0));
                printf("JP V0, 0x%04x\n", address_param);

            } else if (check_instruction(instruction, 0xC000, 0xF000)) {
                // Cxkk - RND Vx, byte
                // Set Vx = random byte AND kk.
                uint8_t random = static_cast<uint8_t>(rand() % 256);
                registers.at(x) = (random & kk);
                std::cout << "RND V" << unsigned(x) << ", #" << unsigned(kk) << "\n";

            } else if (check_instruction(instruction, 0xD000, 0xF000)) {
                // Dxyn - DRW Vx, Vy, nibble
                // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
                std::uint8_t x_val = registers.at(x);
                std::uint8_t y_val = registers.at(y);
                std::uint8_t bytes_to_read = nibble;
                
                // 0,0 coords are at the top left of the screen
                for (unsigned int i=0; i<bytes_to_read; i++) {
                    auto row = (y_val + i) % SCREEN_HEIGHT;
                    std::cout << "ROW" << row << std::endl;
                    std::cout << i_register+i << std::endl;
                    auto sprite = memory.at(i_register+i);

                    for (unsigned int j=0; j<8; j++) {
                        auto column = (x_val + j) % SCREEN_WIDTH;
                        std::cout << "COL" << row << std::endl;
                        bool was_set;
                        if (display.at(row).at(column) == 1) {
                            was_set = true;
                        } else {
                            was_set = false;
                        }

                        display.at(row).at(column) = display.at(row).at(column) ^ ((sprite >> (7-j)) & 0x1);

                        if (display.at(row).at(column) == 0 && was_set) {
                            // Bit was erased, so we set VF
                            registers.at(0xF) = 1;
                        }
                    }
                }

                std::cout << "DRW V" << unsigned(x) << ", V" << unsigned(nibble) << ", ";
                printf("0x%01x\n", nibble);

            } else if (check_instruction(instruction, 0xF007, 0xF0FF)) {
                // Fx07 - LD Vx, DT
                // Set Vx = delay timer value.
                registers.at(x) = delay_timer;
                std::cout << "LD V" << unsigned(x) << ", DT\n";

            } else if (check_instruction(instruction, 0xF015, 0xF0FF)) {
                // Fx15 - LD DT, Vx
                // Set delay timer = Vx.
                delay_timer = registers.at(x);
                std::cout << "LD DT, V" << unsigned(x) << "\n";

            } else if (check_instruction(instruction, 0xF018, 0xF0FF)) {
                // Fx18 - LD ST, Vx
                // Set sound timer = Vx.
                sound_timer = registers.at(x);
                std::cout << "LD ST, V" << unsigned(x) << "\n";

            } else if (check_instruction(instruction, 0xF01E, 0xF0FF)) {
                // Fx1E - ADD I, Vx
                // Set I = I + Vx.
                i_register += registers.at(x);
                std::cout << "ADD I, V" << unsigned(x) << "\n";

            } else if (check_instruction(instruction, 0xF029, 0xF0FF)) {
                // Fx29 - LD F, Vx
                // Set I = location of sprite for digit Vx.
                auto font = registers.at(x);
                i_register = (font * 5) + 0x50;
                std::cout << "LD F, V" << unsigned(x) << "\n";

            } else if (check_instruction(instruction, 0xF033, 0xF0FF)) {
                // Fx33 - LD B, Vx
                // Store BCD representation of Vx in memory locations I, I+1, and I+2.
                auto val = registers.at(x);

                memory.at(i_register) = val/100;
                memory.at(i_register+1) = val/10;
                memory.at(i_register+2) = val%10;
                
                std::cout << "LD B, V" << unsigned(x) << "\n";

            } else if (check_instruction(instruction, 0xF055, 0xF0FF)) {
                // Fx55 - LD [I], Vx
                // Store registers V0 through Vx in memory starting at location I.
                for (uint16_t i = 0; i < x; i++) {
                    memory.at(i_register+i) = registers.at(i);
                } // TODO CHECK
                std::cout << "LD [I], V" << unsigned(x) << "\n";

            } else if (check_instruction(instruction, 0xF065, 0xF0FF)) {
                // Fx65 - LD Vx, [I]
                // Read registers V0 through Vx from memory starting at location I.
                for (uint16_t i = 0; i < x; i++) {
                    registers.at(i) = memory.at(i_register+i);
                } // TODO CHECK
                std::cout << "LD V" << unsigned(x) << ", [I]\n";

            } else {
                std::cout << "NOOP? " << unsigned(instruction) << "\n";
            }
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

    std::array<std::array<uint16_t, SCREEN_WIDTH>, SCREEN_HEIGHT> get_video_buffer() {
        auto result = std::array<std::array<uint16_t, SCREEN_WIDTH>, SCREEN_HEIGHT> {};
        
        for (size_t i = 0; i < SCREEN_HEIGHT; i++) {
            for (size_t j = 0; j < SCREEN_WIDTH; j++) {
                if (display.at(i).at(j) == 1)
                {
                    result.at(i).at(j) = 0xffff;
                }
            }
        }

        return result;
    }

    void startup()
    {
        // Load fonts
        std::array<std::uint8_t, 0x50> fonts {
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };

        for (std::uint16_t i = 0; (i + 0x50) <= 0x9F; i++)
        {
            memory.at(i + 0x50) = fonts.at(i);
        }

        // Seed random
        srand((unsigned) time(NULL));
    }
}