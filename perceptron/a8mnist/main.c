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
#define DOWN_LOCKOUT 25

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
static int last_px = -10, last_py = -10, have_strokes = 0, have_all_strokes = 0, down_frames = 0;
KoalaCal cal = { 4, 228, 4, 228, 0, 0 }; /* sane defaults, tune below */

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
    //printf("butt1\n");
    if (s.left_button_down) {
        // Is stylus down?
        if ((s.raw_x > (cal.x_min + 8)) && (s.raw_y > (cal.y_min + 8))) {
            if (++down_frames > DOWN_LOCKOUT) {
                int px = koala_scale(s.raw_x, cal.x_min, cal.x_max, cal.invert_x, CANVAS_SIZE) + CANVAS_X0;
                int py = koala_scale(s.raw_y, cal.y_min, cal.y_max, cal.invert_y, CANVAS_SIZE) + CANVAS_Y0;
                if ((last_px > 0) && (last_py > 0)) {
                    int delta_px = abs(last_px - px);
                    int delta_py = abs(last_py - py);
                    //printf("butt0 line %d, %d, %d, %d\n", last_px, last_py, px, py);
                    if ((delta_px + delta_py) < 10) {
                        canvas_line(last_px, last_py, px, py, 2);
                    }
                } else {
                    //printf("butt0 plot %d, %d, %d, %d\n", last_px, last_py, px, py);
                    canvas_plot(px, py, 2);
                }
                last_px = px; last_py = py;
                have_strokes = 1;
            }        
        //printf("butt rawx %d, rawy %d, px %d, py %d, last_px %d, last_py %d\n", s.raw_x, s.raw_y, py, last_px, last_py);
        } else {
            // Stylus up
            last_px = -10; last_py = -10;
            down_frames = 0;
        }
    } else {
        last_px = -10; last_py = -10;   /* pen lifted, break the stroke */
        if (have_strokes) {
            have_all_strokes = 1;
        }
    }
}

void copy_to_model_input(void) {
    unsigned char x, y;
    for (y = 0; y < 28; y++)
        for (x = 0; x < 28; x++)
            digit_input[y][x] = canvas_get(CANVAS_X0 + x, CANVAS_Y0 + y) * 127;
}

void plot_a_confidence(uint8_t i, int confidence, uint8_t c) {
    signed char rawbars = (confidence >> 5);
    unsigned char bars = (rawbars < 0) ? 0:rawbars;  /* 0-7 bars */
    bars = bars >= 14 ? 14:bars;
    canvas_line(CONFIDENCE_X_START + (2 * i), CONFIDENCE_Y_START, CONFIDENCE_X_START + (2 * i), CONFIDENCE_Y_START - bars, c);
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

    score_a_digit((uint8_t *)digit_input, pred, &output[pred]);
    plot_a_confidence(pred, output[pred], 1);
    if (output[pred] >= 40) {
        sprintf(digit, "%d", pred);
    }
    for (i = 1; i < 10; i++) {
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
    have_all_strokes = 0;
}

void main(void) 
{
    //unsigned int x;
    //unsigned char y;
    KoalaSample s;
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
        printf("b %d, %d, %d, %d\n", s.raw_x, s.raw_y, s.left_button_down, PEEK(STRIG_BASE + KOALA_TRIG));
    }*/
    set_graphics5();

 
    screen_base = (int)OS.savmsc; /* SAVMSC, now points at start of the GRAPHICS 8 bitmap */
    //printf("screen_base = $%04X\n", screen_base);

    draw_frame(CANVAS_X0 - 1, CANVAS_Y0 - 1, CANVAS_X0 + CANVAS_SIZE + 1, CANVAS_Y0 + CANVAS_SIZE + 1, 2);
    draw_frame(CONFIDENCE_X_START - 4, CONFIDENCE_Y_START - 16, CONFIDENCE_X_START + 23, CONFIDENCE_Y_START + 3, 1);
    // Draw confidence axes
    canvas_line(CONFIDENCE_X_START - 2, CONFIDENCE_Y_START + 1, CONFIDENCE_X_START + 21, CONFIDENCE_Y_START + 1, 2);
    canvas_line(CONFIDENCE_X_START - 2, CONFIDENCE_Y_START, CONFIDENCE_X_START - 2, CONFIDENCE_Y_START - 13, 2);

    //koala_calibrate(&cal);
    //printf("Calibration: (%d, %d) -> (%d, %d)", cal.x_min, cal.x_max, cal.y_min, cal.y_max);
    printf("<C>alibrate, <D>emo; any other key for drawing mode\n");
    
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

        if (state == STATE_DRAWING && have_all_strokes) {
            copy_to_model_input();
            do_real_time_eval();
            state = STATE_RESULT;
        }

        // Either we've finished a demo eval, we've finished a drawn eval, or we're waiting for more drawing input.
        c = PEEK(CH);        
        if (c != 255) {   /* any key = clear */
            clear_canvas();
            clear_confidence();
            //printf("butt1 %d", c);
            if (c == 58) {  // D
                printf("Entering demo mode\n");
                state = STATE_DEMO;
            } else if (c == 18) { // C
                printf("Calibrating...\n");
                koala_calibrate(&cal);
            } else {
                printf("Entering drawing mode\n");
                state = STATE_DRAWING;
                idle_frames = 0;
                have_strokes = 0;
                have_all_strokes = 0;
                last_px = -10; last_py = -10;
            }
            POKE(CH, 255);
        }
    }

    // Wait for a key press before exiting back to DOS/OS
    cgetc();

    // Return to standard text mode (Graphics 0)
    _graphics(0);
}
