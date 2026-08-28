#include "koala.h"
#include <atari.h>
#include <peekpoke.h>
#include <stdio.h>

void koala_read(KoalaSample *s) {
    //s->raw_x    = PEEK(PADDL_BASE + KOALA_PORT_X);
    s->raw_x  = (*(unsigned char*)(PADDL_BASE + KOALA_PORT_X));
    //s->raw_y    = PEEK(PADDL_BASE + KOALA_PORT_Y);
    s->raw_y = (*(unsigned char*)(PADDL_BASE + KOALA_PORT_Y));
    s->touching = (unsigned char)((PEEK(STRIG_BASE + KOALA_TRIG) & 0x04) == 0);
    //printf("b %d, %d, %d, %d\n", s->raw_x, s->raw_y, s->touching, PEEK(STRIG_BASE + KOALA_TRIG));
}

/* Map raw pot value to canvas pixel coord, with calibration + clamping */
int koala_scale(unsigned char raw, unsigned char lo, unsigned char hi,
                        unsigned char invert, int size) {
    long v;
    if (raw < lo) raw = lo;
    if (raw > hi) raw = hi;
    v = (long)(raw - lo) * (size - 1) / (hi - lo);
    return invert ? (size - 1 - (int)v) : (int)v;
}

void koala_calibrate(KoalaCal *c) {
    KoalaSample s;
    printf("Touch TOP-LEFT corner, hold still, press trigger\n");
    do { koala_read(&s); } while (!s.touching);
    c->x_min = s.raw_x; c->y_min = s.raw_y;
    printf("ul %d, %d\n", s.raw_x, s.raw_y);
    do { koala_read(&s); } while (s.touching);

    printf("Touch BOTTOM-RIGHT corner, hold still, press trigger\n");
    do { koala_read(&s); } while (!s.touching);
    c->x_max = s.raw_x; c->y_max = s.raw_y;
    printf("lr %d, %d\n", s.raw_x, s.raw_y);

    do { koala_read(&s); } while (s.touching);

    /* handle pads that report inverted axes */
    if (c->x_min > c->x_max) { unsigned char t=c->x_min; c->x_min=c->x_max; c->x_max=t; c->invert_x=1; }
    if (c->y_min > c->y_max) { unsigned char t=c->y_min; c->y_min=c->y_max; c->y_max=t; c->invert_y=1; }
}