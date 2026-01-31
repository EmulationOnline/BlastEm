// Stubs for JIT functions not available in interpreter (NEW_CORE) mode
#ifdef NEW_CORE

#include <stdlib.h>
#include "blastem/m68k_core.h"

void m68k_invalidate_code_range(m68k_context *context, uint32_t start, uint32_t end) {
    // No native code to invalidate in interpreter
}

m68k_context *m68k_handle_code_write(uint32_t address, m68k_context *context) {
    // No native code to invalidate in interpreter
    return context;
}

void m68k_options_free(m68k_options *opts) {
    // Interpreter options cleanup
    free(opts);
}

void *get_native_address_trans(m68k_context *context, uint32_t address) {
    // No native address translation in interpreter
    return NULL;
}

void resume_68k(m68k_context *context) {
    // No-op for interpreter - execution handled differently
}

void z80_handle_code_write(uint32_t address, void *context) {
    // No native code to invalidate in interpreter
}

#endif
