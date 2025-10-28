#include "corelib.h"
#include "blastem/system.h"
#include "blastem/util.h"
#include "blastem/vdp.h"
#include "blastem/render.h"
#include "blastem/io.h"
#include "blastem/genesis.h"
#include "blastem/sms.h"
#include "blastem/cdimage.h"

#define REQUIRE_SYSTEM(val) if (!system_) { printf("Skipping %s\n", __func__); return val; }
// Global state
struct system_header *system_ = NULL;
struct system_media cart_;

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
    // current_system->resume_context(current_system)
    // OR
    // current_system->start_context(current_system, NULL) (first time)
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
    if (system_ != NULL) {
        system_request_exit(system_, /*force_exit*/1);
        system_ = NULL;
    }
    cart_.buffer = (void*)data;
    cart_.size = len;
    puts("loading rom");
    system_ = alloc_config_system(SYSTEM_UNKNOWN, &cart_, /*opts*/0, /*force_region*/0);
    // from libretro:
    puts("initializing renderer");
	render_audio_initialized(RENDER_AUDIO_S16, 53693175 / (7 * 6 * 4), 2, 4, sizeof(int16_t));

    // Render first frame, so frame() doesn't need conditional.
    system_->start_context(system_, NULL);

    // system_type detect_system_type(system_media *media);
    // system_header *alloc_config_system(system_type stype, system_media *media, uint32_t opts, uint8_t force_region);
    // system_header *alloc_config_player(system_type stype, event_reader *reader);
    // void system_request_exit(system_header *system, uint8_t force_release);
    // uint32_t load_media(char * filename, system_media *dst, system_type *stype);
    // void* load_media_subfile(const system_media *media, char *path, uint32_t *sizeout);
    // load_media()  // reqs path

    // load_game

    // from genesis.h
    // genesis_context *alloc_config_genesis(void *rom, uint32_t rom_size, void *lock_on, uint32_t lock_on_size, uint32_t system_opts, uint8_t force_region);
    // genesis_context *alloc_config_genesis_cdboot(system_media *media, uint32_t system_opts, uint8_t force_region);
    // genesis_context* alloc_config_pico(void *rom, uint32_t rom_size, void *lock_on, uint32_t lock_on_size, uint32_t ym_opts, uint8_t force_region, system_type stype);
    // void genesis_serialize(genesis_context *gen, serialize_buffer *buf, uint32_t m68k_pc, uint8_t all);
    // void genesis_deserialize(deserialize_buffer *buf, genesis_context *gen);


    // from blastem:
    // alloc_config_system
	// game_system = alloc_config_system(stype, &cart, opts, force_region);
    // setup_saves(&cart, game_system);
    // init_system_with_media(next_rom, force_stype);
    // current_system->arena = set_current_arena(game_system->arena);
}

__attribute__((visibility("default")))
long apu_sample_variable(int16_t *output, int32_t frames) {
    REQUIRE_SYSTEM(0);
    // see mix_and_convert, render_audio.c
    // also render_put_mono_sample (public interface)
    return 0;
}
