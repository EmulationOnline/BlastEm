/*
 * VDP (Video Display Processor) Test Program
 * ===========================================
 *
 * This test verifies that the Genesis VDP is working correctly by writing
 * a color to the backdrop and checking if it appears in the framebuffer.
 *
 * GENESIS VDP MEMORY MAP
 * ----------------------
 * The 68k CPU communicates with the VDP through memory-mapped I/O:
 *
 *   0xC00000 - VDP Data Port (read/write VRAM/CRAM/VSRAM data)
 *   0xC00004 - VDP Control Port (write commands, read status)
 *   0xC00008 - HV Counter (read horizontal/vertical position)
 *
 * VDP REGISTER WRITES
 * -------------------
 * To set a VDP register, write a 16-bit value to the control port:
 *
 *   Format: 0x8RVV where R = register number (0-23), VV = value
 *   Example: move.w #$8174, $C00004  ; Set register 1 to 0x74
 *
 * Key registers:
 *   Reg 0: Mode Set 1 (H-Int enable, HV counter latch, etc.)
 *   Reg 1: Mode Set 2 (Display enable, V-Int enable, DMA, V30 mode)
 *   Reg 7: Background color (palette line << 4 | color index)
 *
 * VDP MEMORY ACCESS (VRAM/CRAM/VSRAM)
 * -----------------------------------
 * To read/write VDP memory, first write a 32-bit command to control port:
 *
 *   Bits 31-30: Access type
 *     00 = VRAM read     01 = VRAM write
 *     10 = CRAM read     11 = CRAM write  (Color RAM - 64 colors)
 *     00 = VSRAM read    01 = VSRAM write (with CD1=1)
 *
 *   Command format (32-bit, written as two 16-bit words):
 *     CD1 CD0 A13 A12 A11 A10 A9 A8 | A7 A6 A5 A4 A3 A2 A1 A0 (first word)
 *     0   0   0   0   0   0   0  0  | CD3 CD2 0  0  0  A16 A15 A14 (second word)
 *
 *   For CRAM write to address 0: 0xC0000000
 *   For VRAM write to address 0: 0x40000000
 *
 * Then read/write data through the data port at 0xC00000.
 *
 * GENESIS COLOR FORMAT (CRAM)
 * ---------------------------
 * Each color is 16 bits: 0000 BBB0 GGG0 RRR0
 *   - 3 bits per component (0-7), shifted left by 1
 *   - 0x000E = Red, 0x00E0 = Green, 0x0E00 = Blue, 0x0EEE = White
 *
 * TEST PROGRAM FLOW
 * -----------------
 * 1. Write marker 0x12345678 to RAM (proves CPU is executing)
 * 2. Set VDP register 1 to enable display
 * 3. Set VDP register 7 to select backdrop color index 0
 * 4. Write CRAM address 0 command to control port
 * 5. Write blue color (0x0E00) to data port
 * 6. Write marker 0xBD900123 to RAM (proves VDP code was reached)
 * 7. Infinite loop
 *
 * After running frames, we check:
 * - Save state for RAM markers (CPU execution proof)
 * - Framebuffer for non-zero pixels (VDP rendering proof)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "corelib.h"

// Genesis VDP ports
#define VDP_DATA    0xC00000
#define VDP_CTRL    0xC00004

// VDP register set command: 0x8000 | (reg << 8) | value
#define VDP_REG(r, v) (0x8000 | ((r) << 8) | (v))

// CRAM write command: 0xC0000000 | (address << 17)
#define CRAM_WRITE(addr) (0xC0000000 | ((addr) << 17))

// Genesis color format: 0000 BBB0 GGG0 RRR0
#define COLOR_RED    0x000E  // Red
#define COLOR_GREEN  0x00E0  // Green
#define COLOR_BLUE   0x0E00  // Blue
#define COLOR_WHITE  0x0EEE  // White
#define COLOR_CYAN   0x0EE0  // Cyan

static uint8_t test_rom[0x10000];  // 64KB ROM

static void build_test_rom(void) {
    memset(test_rom, 0, sizeof(test_rom));

    // Vector table
    // 0x00: Initial SSP
    test_rom[0x00] = 0x00; test_rom[0x01] = 0xFF;
    test_rom[0x02] = 0xFF; test_rom[0x03] = 0xFE;
    // 0x04: Initial PC = 0x200
    test_rom[0x04] = 0x00; test_rom[0x05] = 0x00;
    test_rom[0x06] = 0x02; test_rom[0x07] = 0x00;

    // Fill exception vectors with address of infinite loop (0x2FE)
    for (int i = 8; i < 0x100; i += 4) {
        test_rom[i] = 0x00; test_rom[i+1] = 0x00;
        test_rom[i+2] = 0x02; test_rom[i+3] = 0xFE;
    }

    // Code at 0x200
    uint8_t *p = &test_rom[0x200];

    // First, write marker to RAM to confirm CPU is running
    // move.l #$12345678, $FF0000
    *p++ = 0x23; *p++ = 0xFC;  // move.l #imm, abs.l
    *p++ = 0x12; *p++ = 0x34; *p++ = 0x56; *p++ = 0x78;  // immediate
    *p++ = 0x00; *p++ = 0xFF; *p++ = 0x00; *p++ = 0x00;  // address

    // Set up VDP registers for basic display
    // VDP register writes go to control port as: move.w #$8Rvv, $C00004
    // where R = register number, vv = value

    // Reg 0: Mode 1 - disable HInt
    // move.w #$8004, $C00004
    *p++ = 0x33; *p++ = 0xFC;  // move.w #imm, abs.l
    *p++ = 0x80; *p++ = 0x04;  // VDP_REG(0, 0x04)
    *p++ = 0x00; *p++ = 0xC0; *p++ = 0x00; *p++ = 0x04;

    // Reg 1: Mode 2 - enable display, VInt, DMA, 224 line mode
    // move.w #$8174, $C00004
    *p++ = 0x33; *p++ = 0xFC;
    *p++ = 0x81; *p++ = 0x64;  // VDP_REG(1, 0x64) - display ON, VInt ON
    *p++ = 0x00; *p++ = 0xC0; *p++ = 0x00; *p++ = 0x04;

    // Reg 7: Background color = palette 0, color 0
    // move.w #$8700, $C00004
    *p++ = 0x33; *p++ = 0xFC;
    *p++ = 0x87; *p++ = 0x00;  // VDP_REG(7, 0x00)
    *p++ = 0x00; *p++ = 0xC0; *p++ = 0x00; *p++ = 0x04;

    // Now write a color to CRAM address 0 (backdrop color)
    // First set CRAM write mode: move.l #$C0000000, $C00004
    *p++ = 0x23; *p++ = 0xFC;  // move.l #imm, abs.l
    *p++ = 0xC0; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;  // CRAM write addr 0
    *p++ = 0x00; *p++ = 0xC0; *p++ = 0x00; *p++ = 0x04;  // to VDP ctrl

    // Write BLUE color to CRAM: move.w #$0E00, $C00000
    *p++ = 0x33; *p++ = 0xFC;  // move.w #imm, abs.l
    *p++ = 0x0E; *p++ = 0x00;  // Blue color
    *p++ = 0x00; *p++ = 0xC0; *p++ = 0x00; *p++ = 0x00;  // to VDP data

    // Write success marker to RAM
    // move.l #$VDPOK123, $FF0004
    *p++ = 0x23; *p++ = 0xFC;
    *p++ = 0xBD; *p++ = 0x90; *p++ = 0x01; *p++ = 0x23;  // 0xBD900123
    *p++ = 0x00; *p++ = 0xFF; *p++ = 0x00; *p++ = 0x04;

    // Infinite loop
    // 0x2FE: bra.s *
    while ((p - test_rom) < 0x2FE) *p++ = 0x4E; *p++ = 0x71;  // nop padding
    test_rom[0x2FE] = 0x60;
    test_rom[0x2FF] = 0xFE;

    printf("Test ROM built, code ends at 0x%lx\n", (long)(p - test_rom));
}

static void dummy_puts(const char *s) {
    printf("EMU: %s\n", s);
}

int main(int argc, char **argv) {
    printf("=== VDP Test ===\n\n");

    build_test_rom();

    corelib_set_puts(dummy_puts);

    printf("Initializing emulator...\n");
    init(test_rom, sizeof(test_rom));

    printf("\nRunning 5 frames...\n");
    for (int i = 0; i < 5; i++) {
        frame();
        printf("Frame %d complete\n", i + 1);
    }

    // Check framebuffer
    printf("\n--- Framebuffer Analysis ---\n");
    const uint8_t *fb = framebuffer();
    int total_pixels = VIDEO_WIDTH * VIDEO_HEIGHT;

    // Count non-zero pixels and sample some colors
    int nonzero = 0;
    uint32_t first_color = 0;
    int color_counts[16] = {0};  // Simple histogram

    const uint32_t *fb32 = (const uint32_t *)fb;
    for (int i = 0; i < total_pixels; i++) {
        uint32_t pixel = fb32[i];
        if (pixel != 0) {
            nonzero++;
            if (first_color == 0) first_color = pixel;
        }
        // Simple bucketing by high nibble of red channel
        int bucket = (pixel >> 20) & 0xF;
        color_counts[bucket]++;
    }

    printf("Total pixels: %d\n", total_pixels);
    printf("Non-zero pixels: %d (%.1f%%)\n", nonzero, 100.0 * nonzero / total_pixels);
    printf("First non-zero color: 0x%08X\n", first_color);

    // Print corners of framebuffer
    printf("\nCorner pixels (RGBA):\n");
    printf("  Top-left:     0x%08X\n", fb32[0]);
    printf("  Top-right:    0x%08X\n", fb32[VIDEO_WIDTH - 1]);
    printf("  Bottom-left:  0x%08X\n", fb32[(VIDEO_HEIGHT-1) * VIDEO_WIDTH]);
    printf("  Bottom-right: 0x%08X\n", fb32[VIDEO_HEIGHT * VIDEO_WIDTH - 1]);
    printf("  Center:       0x%08X\n", fb32[VIDEO_HEIGHT/2 * VIDEO_WIDTH + VIDEO_WIDTH/2]);

    // Check save state for RAM markers and VDP state
    printf("\n--- Save State Analysis ---\n");
    int state_size = save_str(NULL, 0);
    printf("State size: %d bytes\n", state_size);

    if (state_size > 0) {
        uint8_t *state = malloc(state_size);
        save_str(state, state_size);

        // Look for our markers
        int found_cpu_marker = 0, found_vdp_marker = 0;
        for (int i = 0; i < state_size - 4; i++) {
            if (state[i] == 0x12 && state[i+1] == 0x34 &&
                state[i+2] == 0x56 && state[i+3] == 0x78) {
                printf("Found CPU marker (0x12345678) at offset %d\n", i);
                found_cpu_marker = 1;
            }
            if (state[i] == 0xBD && state[i+1] == 0x90 &&
                state[i+2] == 0x01 && state[i+3] == 0x23) {
                printf("Found VDP-written marker (0xBD900123) at offset %d\n", i);
                found_vdp_marker = 1;
            }
        }

        // Try to find CRAM in state (should contain our blue color 0x0E00)
        printf("\nSearching for CRAM blue color (0x0E00)...\n");
        for (int i = 0; i < state_size - 2; i++) {
            // Check both byte orders
            if ((state[i] == 0x0E && state[i+1] == 0x00) ||
                (state[i] == 0x00 && state[i+1] == 0x0E)) {
                printf("  Possible CRAM color at offset %d: %02X %02X\n",
                       i, state[i], state[i+1]);
                if (i < 5) break;  // Only show first few matches
            }
        }

        printf("\n--- Test Results ---\n");
        printf("CPU executing:     %s\n", found_cpu_marker ? "YES" : "NO");
        printf("VDP code reached:  %s\n", found_vdp_marker ? "YES" : "NO");
        printf("Framebuffer data:  %s\n", nonzero > 0 ? "YES" : "NO (BLACK SCREEN)");

        if (found_cpu_marker && found_vdp_marker && nonzero > 0) {
            printf("\n*** SUCCESS: VDP appears to be working! ***\n");
        } else if (found_cpu_marker && found_vdp_marker && nonzero == 0) {
            printf("\n*** PARTIAL: CPU+VDP code ran but framebuffer is empty ***\n");
            printf("    This suggests render path issue, not CPU/VDP issue.\n");
        } else {
            printf("\n*** FAILURE: Check markers above for details ***\n");
        }

        free(state);
    }

    printf("\n=== VDP Test Complete ===\n");
    return 0;
}
