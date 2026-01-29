#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "corelib.h"
#include "blastem/system.h"
#include "blastem/blastem.h"
#include "blastem/util.h"
#include "blastem/vdp.h"
#include "blastem/render.h"
#include "blastem/render_audio.h"
#include "blastem/io.h"
#include "blastem/genesis.h"
#include "blastem/sms.h"
#include "blastem/cdimage.h"

#define REQUIRE_SYSTEM(val) if (!current_system) { printf("Skipping %s\n", __func__); return val; }

// current_system is declared extern in blastem.h, defined in stubs.c
static system_media cart_;
static system_type stype;
static uint8_t started = 0;

// Required by BlastEm internals
const system_media *current_media(void) {
    return &cart_;
}

__attribute__((visibility("default")))
uint32_t fbuffer_[VIDEO_WIDTH * VIDEO_HEIGHT];

__attribute__((visibility("default")))
void set_key(size_t key, char val) {
    REQUIRE_SYSTEM();
}

__attribute__((visibility("default")))
const uint8_t *framebuffer() {
    return (uint8_t*)fbuffer_;
}

__attribute__((visibility("default")))
void frame() {
    REQUIRE_SYSTEM();
    if (started) {
        current_system->resume_context(current_system);
    } else {
        current_system->start_context(current_system, NULL);
        started = 1;
    }
}

__attribute__((visibility("default")))
void dump_state(const char* save_path) {
    REQUIRE_SYSTEM();
}

__attribute__((visibility("default")))
void save(int fd) {
    REQUIRE_SYSTEM();
}
__attribute__((visibility("default")))
void load_state(const char* save_path) {
    REQUIRE_SYSTEM();
}
__attribute__((visibility("default")))
void load(int fd) {
    REQUIRE_SYSTEM();
}

__attribute__((visibility("default")))
void init(const uint8_t* data, size_t len) {
    // Clean up previous system if any
    if (current_system != NULL) {
        current_system->free_context(current_system);
        current_system = NULL;
    }
    if (cart_.buffer) {
        free(cart_.buffer);
        cart_.buffer = NULL;
    }

    // Reset state
    started = 0;
    stype = SYSTEM_UNKNOWN;
    memset(&cart_, 0, sizeof(cart_));

    // Initialize audio subsystem (NTSC master clock / divider)
    render_audio_initialized(RENDER_AUDIO_S16, 53693175 / (7 * 6 * 4), 2, 4, sizeof(int16_t));

    // Copy ROM data to our own buffer (rounded to power of 2 as BlastEm expects)
    size_t alloc_size = nearest_pow2(len);
    cart_.buffer = malloc(alloc_size);
    if (!cart_.buffer) {
        fprintf(stderr, "Failed to allocate %zu bytes for ROM\n", alloc_size);
        return;
    }
    memcpy(cart_.buffer, data, len);
    cart_.size = len;

    // Detect system type (Genesis, SMS, Game Gear, etc.)
    stype = detect_system_type(&cart_);
    printf("Detected system type: %d\n", stype);

    // Allocate and configure the emulator
    current_system = alloc_config_system(stype, &cart_, 0, 0);
    if (!current_system) {
        fprintf(stderr, "Failed to allocate system\n");
        free(cart_.buffer);
        cart_.buffer = NULL;
        return;
    }

    printf("System initialized successfully\n");
}

__attribute__((visibility("default")))
long apu_sample_variable(int16_t *output, int32_t frames) {
    REQUIRE_SYSTEM(0);
    // see mix_and_convert, render_audio.c
    // also render_put_mono_sample (public interface)
    return 0;
}
