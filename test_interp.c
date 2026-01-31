// Test program to verify interpreter execution
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "corelib.h"

// Minimal Genesis ROM that writes to RAM
// This is a hand-assembled 68k program
static uint8_t test_rom[] = {
    // Vector table (first 256 bytes)
    // 0x00: Initial SSP (Stack Pointer)
    0x00, 0xFF, 0xFF, 0xFE,  // SSP = 0x00FFFFFE (top of RAM)
    // 0x04: Initial PC (Program Counter)
    0x00, 0x00, 0x02, 0x00,  // PC = 0x00000200 (start of our code)

    // Rest of vectors (0x08-0xFF) - fill with dummy handler at 0x200
    // We'll just pad with zeros for now, real hardware would need proper handlers
};

// Our actual code starts at 0x200
static uint8_t test_code[] = {
    // At address 0x200:
    // move.l #$DEADBEEF, D0
    0x20, 0x3C, 0xDE, 0xAD, 0xBE, 0xEF,

    // move.l D0, $FF0000  (write to start of work RAM)
    0x23, 0xC0, 0x00, 0xFF, 0x00, 0x00,

    // move.l #$CAFEBABE, D1
    0x22, 0x3C, 0xCA, 0xFE, 0xBA, 0xBE,

    // move.l D1, $FF0004  (write to RAM + 4)
    0x23, 0xC1, 0x00, 0xFF, 0x00, 0x04,

    // Infinite loop: bra.s *
    0x60, 0xFE
};

static void dummy_puts(const char *s) {
    printf("EMU: %s\n", s);
}

int main(int argc, char **argv) {
    printf("=== Interpreter Test ===\n");

    // Build a complete ROM image
    size_t rom_size = 0x10000;  // 64KB should be enough
    uint8_t *rom = calloc(1, rom_size);
    if (!rom) {
        fprintf(stderr, "Failed to allocate ROM buffer\n");
        return 1;
    }

    // Copy vector table
    memcpy(rom, test_rom, sizeof(test_rom));

    // Pad vectors to 0x200
    // Vectors 0x08-0xFF should point somewhere safe, let's point them to our loop
    for (int i = 8; i < 256; i += 4) {
        rom[i] = 0x00;
        rom[i+1] = 0x00;
        rom[i+2] = 0x02;
        rom[i+3] = 0x00 + sizeof(test_code) - 2;  // Point to the infinite loop
    }

    // Copy our test code at 0x200
    memcpy(rom + 0x200, test_code, sizeof(test_code));

    // Set up the emulator
    corelib_set_puts(dummy_puts);

    printf("Initializing emulator with %zu byte test ROM...\n", rom_size);
    init(rom, rom_size);

    printf("Running 3 frames...\n");
    for (int i = 0; i < 3; i++) {
        frame();
        printf("Frame %d complete\n", i + 1);
    }

    // Now we need to check RAM - but we don't have direct access
    // Let's use save state to examine memory
    printf("\nGetting save state to examine memory...\n");
    int state_size = save_str(NULL, 0);
    printf("State size: %d bytes\n", state_size);

    if (state_size > 0) {
        uint8_t *state = malloc(state_size);
        save_str(state, state_size);

        // The save state format is complex, but we can at least verify it's non-empty
        printf("First 64 bytes of state:\n");
        for (int i = 0; i < 64 && i < state_size; i++) {
            printf("%02X ", state[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\n");

        // Look for our magic values in the state (they should be in RAM section)
        int found_deadbeef = 0, found_cafebabe = 0;
        for (int i = 0; i < state_size - 4; i++) {
            if (state[i] == 0xDE && state[i+1] == 0xAD &&
                state[i+2] == 0xBE && state[i+3] == 0xEF) {
                printf("Found DEADBEEF at state offset %d\n", i);
                found_deadbeef = 1;
            }
            if (state[i] == 0xCA && state[i+1] == 0xFE &&
                state[i+2] == 0xBA && state[i+3] == 0xBE) {
                printf("Found CAFEBABE at state offset %d\n", i);
                found_cafebabe = 1;
            }
        }

        if (found_deadbeef && found_cafebabe) {
            printf("\n*** SUCCESS: Test values found in RAM! Interpreter is executing code. ***\n");
        } else {
            printf("\n*** FAILURE: Test values NOT found. Interpreter may not be executing. ***\n");
            printf("  DEADBEEF: %s\n", found_deadbeef ? "FOUND" : "NOT FOUND");
            printf("  CAFEBABE: %s\n", found_cafebabe ? "FOUND" : "NOT FOUND");
        }

        free(state);
    } else {
        puts("FAIL! No save state generated!");
    }

    free(rom);
    printf("\n=== Test Complete ===\n");
    return 0;
}
