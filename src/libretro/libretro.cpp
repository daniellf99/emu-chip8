/*
Copyright (C) 2024 Daniel L. Ferreira

This file is part of emu-chip8.

emu-chip8 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

emu-chip8 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emu-chip8. If not, see <http://www.gnu.org/licenses/>.
*/
// Includes
#include <cstdint>
#include <cstring>

#if _MSC_VER >= 1910 && !__INTEL_COMPILER
#include "win32.h"
#endif

#include "libretro.h"
#include "../chip8.h"   

constexpr int CYCLES_PER_FRAME = 700;
unsigned long cycles_per_frame = CYCLES_PER_FRAME;

// Callbacks
static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

// Cheats
void retro_cheat_reset(void) {}
void retro_cheat_set([[maybe_unused]] unsigned index, [[maybe_unused]] bool enabled, [[maybe_unused]] const char *code) {
}

// Load a cartridge
bool retro_load_game(const struct retro_game_info *info)
{
    // Set the controller descriptor
    struct retro_input_descriptor desc[] = {
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "1" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "2" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "3" },
            { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "4" },
            { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Analog X" },
            { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Analog Y" },

            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "1" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "2" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "3" },
            { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "4" },
            { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Analog X" },
            { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Analog Y" },
            { 1, RETRO_DEVICE_NONE, 0, 0,  nullptr },
    };

    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

    chip8::reset();

    if (info && info->data) { // ensure there is ROM data
        chip8::load_rom((const  uint8_t*) info->data, info->size);
    }

    return true;
}

bool retro_load_game_special([[maybe_unused]] unsigned game_type, [[maybe_unused]] const struct retro_game_info *info, [[maybe_unused]] size_t num_info) { return false; }

// Unload the cartridge
void retro_unload_game(void) { chip8::unload_rom(); }

unsigned retro_get_region(void) { return RETRO_REGION_PAL; }

// libretro unused api functions
void retro_set_controller_port_device([[maybe_unused]] unsigned port, [[maybe_unused]] unsigned device) {}


void *retro_get_memory_data(unsigned id)
{ 
    if (id == RETRO_MEMORY_SYSTEM_RAM) {
        return chip8::get_memory_buffer();
    }

    return nullptr;
}

size_t retro_get_memory_size(unsigned id)
{
    if (id == RETRO_MEMORY_SYSTEM_RAM) {
        return (size_t) chip8::get_memory_size();
    }

    return 0; 
}

// Serialisation methods
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize([[maybe_unused]] void *data, [[maybe_unused]] size_t size) { return false; }
bool retro_unserialize([[maybe_unused]] const void *data, [[maybe_unused]] size_t size) { return false; }

// End of retrolib
void retro_deinit(void) { }

// libretro global setters
void retro_set_environment(retro_environment_t cb) {
  environ_cb = cb;

  struct retro_variable variables[] = {
      { NULL, NULL },
  };

  bool no_rom = true;
  cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);
  cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_init(void)
{
    /* set up some logging */
    struct retro_log_callback log;
    unsigned level = 4;

    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;
    else
        log_cb = nullptr;

    // the performance level is guide to frontend to give an idea of how intensive this core is to run
    environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);

    chip8::startup();
}


/*
 * Tell libretro about this core, it's name, version and which rom files it supports.
 */
void retro_get_system_info(struct retro_system_info *info)
{
    std::memset(info, 0, sizeof(retro_system_info));
    info->library_name = chip8::get_lib_name();
    info->library_version = chip8::get_lib_version();
    info->need_fullpath = false;
    info->valid_extensions = "ch8";
}

/*
 * Tell libretro about the AV system; the fps, sound sample rate and the
 * resolution of the display.
 */
void retro_get_system_av_info(struct retro_system_av_info *info) {

    int pixel_format = RETRO_PIXEL_FORMAT_RGB565;

    memset(info, 0, sizeof(retro_system_av_info));
    info->timing.fps            = 60.0;
    info->timing.sample_rate    = 44100.0; // TODO
    info->geometry.base_width   = chip8::SCREEN_WIDTH;
    info->geometry.base_height  = chip8::SCREEN_HEIGHT;
    info->geometry.max_width    = chip8::SCREEN_WIDTH;
    info->geometry.max_height   = chip8::SCREEN_HEIGHT;

    // the performance level is guide to frontend to give an idea of how intensive this core is to run
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixel_format);
}

void retro_reset(void)
{
    chip8::reset();
}

// Run a single frame with our chip8 emulator
void retro_run(void)
{
    chip8::fetch_decode_execute(10u);
    
    video_cb(chip8::get_video_buffer().begin(),
        chip8::SCREEN_WIDTH, chip8::SCREEN_HEIGHT, sizeof(uint16_t) * chip8::SCREEN_WIDTH);
}
