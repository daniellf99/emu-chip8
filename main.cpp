#include <cstdint>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <sstream>

bool check_instruction(std::uint16_t inst, std::uint16_t target, std::uint16_t mask) {
    return ((inst & mask) ^ target) == 0;
}

int main() {
    // Given a file, disassemble the first instruction
    char instruction[2] = {0};
    uint16_t current_address = 0;
    std::ifstream input("roms/IBM Logo.ch8", std::ios::binary);

    while (input.read(instruction, 2)) {
        std::uint16_t first_byte = static_cast<std::uint16_t>(static_cast<unsigned char>(instruction[0])) << 8;
        std::uint16_t second_byte = static_cast<std::uint16_t>(static_cast<unsigned char>(instruction[1]));

        std::uint16_t current_instruction = first_byte | second_byte;

        printf("0x%04x  0x%04x ", current_address, current_instruction); // Print instruction address and value as hex
        current_address += 2; // Advance to next address

        if (current_instruction == 0x00E0)
            // CLS - Clear screen
            std::cout << "CLS\n";

        else if (check_instruction(current_instruction, 0xA000, 0xF000)) {
            // LD - Load from address
            std::uint8_t reg = static_cast<std::uint8_t>((current_instruction & 0x0F00) >> 8);
            std::uint8_t address = static_cast<std::uint8_t>(current_instruction & 0x00FF);
            std::cout << "LD V" << unsigned(reg) << ", ";
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

    return 0;
}
