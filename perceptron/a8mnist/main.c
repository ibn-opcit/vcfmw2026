#include <atari.h>
#include <conio.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <peekpoke.h>
#include <6502.h>
#include <fcntl.h>
#include <unistd.h>

#include "koala.h"

#define CANVAS_X0    8     /* must be multiple of 4 */
#define CANVAS_Y0    6
#define CANVAS_SIZE  28
#define GFX5_BYTES_PER_ROW 20   /* 80px / 4px-per-byte */
#define DISK_CAPACITY_IN_IMAGES 130 // Number of MNIST test samples that can fit on floppy

#define CONFIDENCE_X_START (CANVAS_X0 + CANVAS_SIZE + 8)
#define CONFIDENCE_Y_START (CANVAS_Y0 + CANVAS_SIZE - 2)
#define CH 764
#define RAMTOP 0x6A   /* OS location, 1 byte: top of RAM in 256-byte pages */
#define GFX7_BYTES_PER_ROW 40   /* 160 px / 4 px-per-byte = 40, same as mode 8 coincidentally */

extern void infer_digit(uint8_t *input, int16_t *output);
extern void score_a_digit(uint8_t *input, uint8_t d, int16_t *score);
typedef enum { STATE_DRAWING, STATE_RESULT, STATE_DEMO } AppState;

uint8_t digit_input[28][28];

static int16_t output[10];
unsigned int screen_base;
static int last_px = -1, last_py = -1, have_strokes = 0;
KoalaCal cal = { 0, 228, 0, 228, 0, 0 }; /* sane defaults, tune below */


// Unused
void set_graphics8(void) {
    POKE(RAMTOP, PEEK(RAMTOP) - 28);  /* 28*256 = 7168 bytes, >= 7120 needed for mode 8 */
    _graphics(8);       /* conio.h/atari.h helper: mode 8, +16 keeps split text window */
    screen_base = PEEKW(88); /* SAVMSC, now points at start of the GRAPHICS 8 bitmap */
 /* sanity check while bringing this up - remove once confirmed */
    printf("screen_base = $%04X\r\n", screen_base);
}

void set_graphics7(void) {
    POKE(RAMTOP, PEEK(RAMTOP) - 15);   /* 15*256 = 3840 bytes, exact fit for GR.7 */
    _graphics(7);          /* mode 7, +16 keeps split-screen text window */
    screen_base = (unsigned int)OS.savmsc;   /* fixed per your finding - PEEK(88) was wrong */
    OS.color0 = 0x0F;           /* pixel value 1 = bright white, our "pen" color */
}

void set_graphics5(void) {
    POKE(RAMTOP, PEEK(RAMTOP) - 4);   /* 4*256=1024, comfortably covers 960 bytes needed */
    _graphics(5);
    screen_base = (unsigned int)OS.savmsc;
    OS.color1 = 0x0F;                 /* pixel value 1 = pen color, bright white */
}

uint8_t argmax(int16_t *v)
{
    uint8_t best = 0;
    uint8_t i;
    //printf("butt %d", v[best]);
    for (i = 1; i < 10; i++) {
        if (v[i] > v[best]) {
            //printf("%d", v[i]);
            best = i;
        }
    }
    return best;
}

void canvas_plot(int x, int y, unsigned char color) {
    unsigned int addr = screen_base + (unsigned)y * GFX5_BYTES_PER_ROW + (x >> 2);
    unsigned char shift = (3 - (x & 3)) * 2;
    unsigned char mask = 0x03 << shift;
    unsigned char byte = PEEK(addr);
    POKE(addr, (byte & ~mask) | (color << shift));
}

unsigned char canvas_get(int x, int y) {
    unsigned int addr = screen_base + (unsigned)y * GFX5_BYTES_PER_ROW + (x >> 2);
    unsigned char shift = (3 - (x & 3)) * 2;
    return (PEEK(addr) >> shift) & 0x03;
}

void canvas_line(int x0, int y0, int x1, int y1, unsigned char c) {
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2;
    for (;;) {
        canvas_plot(x0, y0, c);   /* pen color = pixel value 1 */
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void clear_canvas(void) {
    unsigned char y;
    unsigned int addr;
    for (y = 0; y < CANVAS_SIZE; y++) {
        addr = screen_base + (unsigned)(CANVAS_Y0 + y) * GFX5_BYTES_PER_ROW + (CANVAS_X0 >> 2);
        memset((void*)addr, 0, CANVAS_SIZE >> 2);   /* 28/4 = 7 bytes/row */
    }
}

void clear_confidence(void) {
    unsigned char y;
    unsigned int addr;
    for (y = CONFIDENCE_Y_START - 15; y <= CONFIDENCE_Y_START; y++) {
        addr = screen_base + (unsigned)(y) * GFX5_BYTES_PER_ROW + ((CONFIDENCE_X_START + 0) >> 2);
        memset((void*)addr, 0, 22 >> 2);
    }
}

void drawing_poll(void) {
    KoalaSample s;
    koala_read(&s);
    if (s.touching) {
        int px = koala_scale(s.raw_x, cal.x_min, cal.x_max, cal.invert_x, CANVAS_SIZE) + CANVAS_X0;
        int py = koala_scale(s.raw_y, cal.y_min, cal.y_max, cal.invert_y, CANVAS_SIZE) + CANVAS_Y0;
        if (last_px >= 0) canvas_line(last_px, last_py, px, py, 2);
        else canvas_plot(px, py, 2);
        last_px = px; last_py = py;
        have_strokes = 1;
    } else {
        last_px = -1; last_py = -1;   /* pen lifted, break the stroke */
    }
}

void copy_to_model_input(void) {
    unsigned char x, y;
    for (y = 0; y < 28; y++)
        for (x = 0; x < 28; x++)
            digit_input[y][x] = canvas_get(CANVAS_X0 + x, CANVAS_Y0 + y) * 127;
}

void plot_confidence(int* confidence) {
    uint8_t i;
    for (i = 0; i < 10; i++) {
        signed char rawbars = (confidence[i] >> 5);
        unsigned char bars = (rawbars < 0) ? 0:rawbars;  /* 0-7 bars */
        canvas_line(CONFIDENCE_X_START + (2 * i), CONFIDENCE_Y_START, CONFIDENCE_X_START + (2 * i), CONFIDENCE_Y_START - bars, 3);
    }
}

void plot_a_confidence(uint8_t i, int confidence, uint8_t c) {
    signed char rawbars = (confidence >> 5);
    unsigned char bars = (rawbars < 0) ? 0:rawbars;  /* 0-7 bars */
    bars = bars >= 14 ? 14:bars;
    canvas_line(CONFIDENCE_X_START + (2 * i), CONFIDENCE_Y_START, CONFIDENCE_X_START + (2 * i), CONFIDENCE_Y_START - bars, c);
}

void show_results(unsigned char predicted, int *confidence /* [10], 0-255 scaled */) {
    //unsigned char i, j;
    //gotoxy(0, 21);  /* text window row, adjust to your split */
    printf("Digit guess: %d (raw score %d)", predicted, confidence[predicted]);
    //gotoxy(0, 22);
    plot_confidence(confidence);
    /*
    for (i = 0; i < 10; i++) {
        signed char rawbars = (confidence[i] >> 5);
        unsigned char bars = (rawbars < 0) ? 0:rawbars;  // 0-7 bars
        printf("%d:", i);
        for (j = 0; j < bars; j++) putchar('#');
        putchar(' ');
    }*/
}

void draw_line_os(unsigned int x1, unsigned char y1, unsigned int x2, unsigned char y2) {
    // 1. Plot the starting point (X1, Y1)
    OS.iocb[6].command = 0x0B;  // PLOT command
    OS.iocb[6].buffer  = NULL;
    OS.iocb[6].buflen  = 0;
    OS.iocb[6].aux1    = y1;    // Y coordinate
    OS.iocb[6].aux2    = x1;    // X coordinate (low/high handled by OS)
    //OS.addrt &= ~0x40;          // Clear carry flag if needed, or invoke via CIO:
    __asm__("ldx #$60");        // Load IOCB6 offset
    __asm__("jsr $E456");       // Call CIOV (CIO entry point)

    // 2. Draw to the destination point (X2, Y2)
    OS.iocb[6].command = 0x11;  // DRAWTO command
    OS.iocb[6].aux1    = y2;    // Destination Y
    OS.iocb[6].aux2    = x2;    // Destination X
    __asm__("ldx #$60");        // Load IOCB6 offset
    __asm__("jsr $E456");       // Call CIOV
}

void plot_pixel_direct(unsigned int x, unsigned char y) {
    // SAVMSC pointer holds the start of screen memory
    unsigned char* screen_ptr = (unsigned char*)(OS.savmsc);
    
    // Calculate the target byte offset (40 bytes per row)
    unsigned int byte_offset = (y * 40) + (x / 8);
    
    // Determine which bit inside that byte needs to be flipped
    // In Antic 4/Mode 8, the leftmost pixel is the highest bit (7)
    unsigned char bit_mask = 0x80 >> (x % 8);
    
    // Set the pixel
    screen_ptr[byte_offset] |= bit_mask;
}

void draw_frame(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, unsigned char c) {
    canvas_line(x0, y0, x0, y1, c);
    canvas_line(x0, y1, x1, y1, c);
    canvas_line(x1, y1, x1, y0, c);
    canvas_line(x1, y0, x0, y0, c);
}

void load_input(const char *filename)
{
    FILE *fd = fopen(filename, "rb");
    int bytes_read;
    if (fd == NULL) {
        printf("I couldn't read %s", filename);
        return;
    }
    bytes_read = fread((uint8_t *)digit_input, 1, 784, fd);
    if (bytes_read != 784) {
        printf("Error reading from %s, %d bytes read", filename, bytes_read);
    }
    fclose(fd);
}

void plot_input() {
    unsigned char x, y;
    for (y = 0; y < 28; y++)
        for (x = 0; x < 28; x++)
            if (digit_input[y][x] > 10) {
                canvas_plot(x + CANVAS_X0, y + CANVAS_Y0, 2);
            }
}

void do_real_time_eval() {
    uint8_t pred = 0;
    uint8_t i;
    char digit[2];

    sprintf(digit, "%s", "?");

    for (i = 0; i < 10; i++) {
        score_a_digit((uint8_t *)digit_input, i, &output[i]);
        if (output[i] > output[pred]) {
            // Demote
            plot_a_confidence(pred, output[pred], 3);
            pred = i;
            // Promote
            plot_a_confidence(i, output[i], 1);
            if (output[i] >= 40) {
                sprintf(digit, "%d", pred);
            }
        } else {
            plot_a_confidence(i, output[i], 3);
        }
    }
    printf("Digit guess: %s (score %d)\n", digit, output[pred]);
    have_strokes = 0;
}

void main(void) 
{
    //unsigned int x;
    //unsigned char y;
    //KoalaSample s;
    AppState state = STATE_DEMO;
    unsigned long idle_frames = 0;
    //uint8_t pred;
    //uint8_t i;
    int r;
    char rfname[16];
    //char digit[2];
    unsigned char c;

    /*while (1) {
        koala_read(&s);
    }*/
    set_graphics5();

 
    // Draw a direct diagonal line using a basic step loop
    /*for (x = 0; x < 160; ++x) {
        y = (unsigned char)x;
        plot_pixel_direct(x, y);
    }*/
    screen_base = (int)OS.savmsc; /* SAVMSC, now points at start of the GRAPHICS 8 bitmap */
    //printf("screen_base = $%04X\n", screen_base);

    draw_frame(CANVAS_X0 - 1, CANVAS_Y0 - 1, CANVAS_X0 + CANVAS_SIZE + 1, CANVAS_Y0 + CANVAS_SIZE + 1, 2);
    draw_frame(CONFIDENCE_X_START - 4, CONFIDENCE_Y_START - 16, CONFIDENCE_X_START + 23, CONFIDENCE_Y_START + 3, 1);
    // Draw confidence axes
    canvas_line(CONFIDENCE_X_START - 2, CONFIDENCE_Y_START + 1, CONFIDENCE_X_START + 21, CONFIDENCE_Y_START + 1, 2);
    canvas_line(CONFIDENCE_X_START - 2, CONFIDENCE_Y_START, CONFIDENCE_X_START - 2, CONFIDENCE_Y_START - 13, 2);


    //koala_calibrate(&cal);
    //printf("Calibration: (%d, %d) -> (%d, %d)", cal.x_min, cal.x_max, cal.y_min, cal.y_max);
    
    for (;;) {
        if (state == STATE_DEMO) {
            r = rand() % DISK_CAPACITY_IN_IMAGES;
            sprintf(rfname, "D2:DIGIT%03d.DAT", r);
            load_input(rfname);
            clear_canvas();
            clear_confidence();
            plot_input();
            do_real_time_eval();
        }

        drawing_poll();

        if (state == STATE_DRAWING) {
            /* infer on: pen lifted after having drawn something, and idle a few frames
               (avoids re-triggering mid-stroke on brief pen lifts) */
            if (last_px < 0 && have_strokes) {
                if (++idle_frames > 15) {   /* ~0.25s at 60Hz */
                    copy_to_model_input();
                    do_real_time_eval();
                    state = STATE_RESULT;
                }
            } else {
                idle_frames = 0;
            }
        }

        // Either we've finished a demo eval, we've finished a drawn eval, or we're waiting for more drawing input.
        c = PEEK(CH);        
        if (c != 255) {   /* any key = clear */
            clear_canvas();
            clear_confidence();
            //printf("butt1 %d", c);
            if (c == 58) {  // D
                printf("Entering demo mode");
                state = STATE_DEMO;
            } else {
                state = STATE_DRAWING;
                idle_frames = 0;
                have_strokes = 0;
                last_px = -1; last_py = -1;
            }
            POKE(CH, 255);
        }
    }

    // Wait for a key press before exiting back to DOS/OS
    cgetc();

    // Return to standard text mode (Graphics 0)
    _graphics(0);
}
