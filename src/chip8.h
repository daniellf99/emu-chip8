#include <cstdint>
#include <cstddef>

namespace chip8 {
    bool check_instruction(std::uint16_t inst, std::uint16_t target, std::uint16_t mask);
    
    void decompile();

    void dump_memory();

    void dump_display();

    void display_registers();

    void fetch_decode_execute();

    void load_rom(uint8_t *data, size_t size);
}