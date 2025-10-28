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

    NUM_KEYS
};

#define VIDEO_WIDTH 256
#define VIDEO_HEIGHT 240

void set_key(size_t key, char val);
void init(const uint8_t* data, size_t len);
const uint8_t *framebuffer();
void frame();
void dump_state(const char* save_path);
void save(int fd);
void load_state(const char* save_path);
void load(int fd);

const int SAMPLE_RATE = 44100;
const int SAMPLES_PER_FRAME = SAMPLE_RATE / 60;
long apu_sample_variable(int16_t *output, int32_t frames);
