#include <cstdint>
#include <cstddef>
#include <string>

namespace chip8 {
    inline constexpr int SCREEN_HEIGHT = 32;
    inline constexpr int SCREEN_WIDTH = 64;

    const char *get_lib_name();
    const char *get_lib_version();

    bool check_instruction(std::uint16_t inst, std::uint16_t target, std::uint16_t mask);
    
    void decompile();

    void dump_memory();

    void dump_display();

    void display_registers();

    void fetch_decode_execute();

    void load_rom(uint8_t *data, size_t size);

    void unload_rom();

    void reset();
}