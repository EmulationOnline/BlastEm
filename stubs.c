#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "corelib.h"
#include "blastem/blastem.h"
#include "blastem/system.h"
#include "blastem/render.h"
#include "blastem/render_audio.h"
#include "blastem/nuklear_ui/debug_ui.h"
#include "blastem/68kinst.h"
#include "blastem/disasm.h"
#include "blastem/upd78k2.h"

// Variables expected by blastem code
int headless = 0;  // Must be 0 for render_framebuffer_updated to be called
int exit_after = 0;
int z80_enabled = 1;
char *save_filename = NULL;
struct system_header *current_system = NULL;
tern_node *config = NULL;
uint8_t use_native_states = 0;

void process_events(void) {}

// Render stubs
uint8_t render_create_window(char *caption, uint32_t width, uint32_t height, window_close_handler close_handler) { return 0; }
void render_destroy_window(uint8_t win_idx) {}

// State in libmd:
extern uint8_t is_pal;
extern struct ring_i16 ring_;

void render_set_video_standard(vid_std std) {
    is_pal = (std == VID_PAL);
}
void render_set_external_sync(uint8_t ext_sync) {}
uint8_t render_fullscreen(void) {return 0; }
void render_framebuffer_updated(uint8_t which, int width) {
    // Signal frame complete so resume_context returns
    system_request_exit(current_system, 0);
}
uint32_t render_map_color(uint8_t r, uint8_t g, uint8_t b) {
    return r | g << 8 | b << 16 | 0xFF << 24;  // RGBA with alpha=0xFF
}
uint32_t render_min_buffered(void) { return 0; }
uint8_t render_is_audio_sync(void) { return 1; }
uint8_t render_should_release_on_exit(void) { return 0; }
void render_buffer_consumed(audio_source *src) {}
void *render_new_audio_opaque(void) { return NULL; }
void render_free_audio_opaque(void *opaque) {}
void render_lock_audio(void) {}
void render_unlock_audio(void) {}
uint32_t render_audio_syncs_per_sec(void) { return 0; }
void render_audio_created(audio_source *src) {}
int16_t audio_tmp_[AUDIO_TMP_LEN];
size_t ring_push(struct ring_i16* ring, const int16_t* src, size_t count);
void render_do_audio_ready(audio_source *src) {
    puts("render_do_audio_ready");
    int16_t *tmp = src->front;
    src->front = src->back;
    src->back = tmp;
    src->front_populated = 1;
    src->buffer_pos = 0;

    if (all_sources_ready()) {
        mix_and_convert((uint8_t*)audio_tmp_, sizeof audio_tmp_, NULL);
        size_t written = ring_push(&ring_, audio_tmp_, AUDIO_TMP_LEN);
        if (written < AUDIO_TMP_LEN) {
            printf("ring_push underwrite: %zu / %d samples\n", written, AUDIO_TMP_LEN);
        }
    }
}
void render_source_paused(audio_source *src, uint8_t remaining_sources) {}
void render_source_resumed(audio_source *src) {}
uint8_t render_is_threaded_video(void) { return 0; }
uint8_t render_get_active_framebuffer(void) { return 0; }
// Framebuffer sized for Genesis: LINEBUF_SIZE (347) * 294 * 2 for interlace
#define LINEBUF_SIZE 347
uint32_t genesis_fb[LINEBUF_SIZE * 294 * 2];

pixel_t *render_get_framebuffer(uint8_t which, int *pitch) {
    *pitch = LINEBUF_SIZE * sizeof(uint32_t);
    if (which) {
        return genesis_fb + LINEBUF_SIZE;
    }
    return genesis_fb;
}
void render_video_loop(void) {}
void render_wait_quit(void) {}
uint32_t render_emulated_width(void) { return 320; }
uint32_t render_emulated_height(void) { return 240; }
uint32_t render_overscan_top(void) { return 0; }
uint32_t render_overscan_bot(void) { return 0; }
uint32_t render_overscan_left(void) { return 0; }
uint32_t render_overscan_right(void) { return 0; }
uint8_t render_create_thread(void *thread, const char *name, render_thread_fun fun, void *data) { return 0; }


// Bindings stubs
void bindings_set_mouse_mode(uint8_t mode) {}
void bindings_reacquire_capture(void) {}
void bindings_release_capture(void) {}

// Debug stubs
uint8_t debug_create_window(uint8_t debug_type, char *caption, uint32_t width, uint32_t height, window_close_handler close_handler) { return 0; }
void upd_debugger(upd78k2_context *upd) {}

// Message boxes
void render_errorbox(char *title, char *message) {
    fprintf(stderr, "ERROR: %s: %s\n", title, message);
}
void render_infobox(char *title, char *message) {
    fprintf(stderr, "INFO: %s: %s\n", title, message);
}

// Bundled file support - rom.db is compiled in
extern const char rom_db_data[];
char *read_bundled_file(char *name, uint32_t *sizeret) {
    if (!strcmp(name, "rom.db")) {
        *sizeret = strlen(rom_db_data);
        char *ret = malloc(*sizeret + 1);
        memcpy(ret, rom_db_data, *sizeret + 1);
        return ret;
    }
    if (sizeret) *sizeret = 0;
    return NULL;
}

// Disassembler display stub (format_label is display only)
int format_label(char *dst, uint32_t address, disasm_context *context) { return 0; }
