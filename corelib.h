#pragma once

#include <stdint.h>
#include <stddef.h>

enum Keys {
    BTN_A = 0,
    BTN_B,
    BTN_Sel,
    BTN_Start,
    BTN_Up,
    BTN_Down,
    BTN_Left,
    BTN_Right,
    BTN_L,  // Shoulder buttons // 8
    BTN_R, // 9
    BTN_L2, // 10
    BTN_R2, // 11
    BTN_X, // 12
    BTN_Y, // 13
    NUM_KEYS
};

// Genesis active display area (excluding borders)
#define VIDEO_WIDTH 320
#define VIDEO_HEIGHT 240  // Max for PAL; NTSC is 224

void set_key(size_t key, char val);
void init(const uint8_t* data, size_t len);
const uint8_t *framebuffer();
void frame();
// dynamic video
int framerate();
int width();
int height();

void dump_state(const char* save_path);
void save(int fd);
void load_state(const char* save_path);
void load(int fd);

#define SAMPLE_RATE 44100
#define SAMPLES_PER_FRAME (SAMPLE_RATE / 60)
long apu_sample_variable(int16_t *output, int32_t frames);
