#ifndef KOALA_H
#define KOALA_H

/* Adjust port index (0..3) to whichever jack the KoalaPad is in */
#define KOALA_PORT_X   0   /* PADDL0 */
#define KOALA_PORT_Y   1   /* PADDL1 */
#define KOALA_TRIG     0   /* STRIG0, port 0/1 share one trigger line */

#define PADDL_BASE_SHADOW     0x0270   /* PADDL0..PADDL7 */
#define PADDL_BASE     0xD200   /* PADDL0..PADDL7 */
#define STRIG_BASE     0x027A   /* STRIG0/STRIG1 shadow */


typedef struct {
    unsigned char raw_x, raw_y;
    unsigned char left_button_down, right_button_down;
} KoalaSample;

/* Calibration - fill these in via a one-time calibration pass */
typedef struct {
    unsigned char x_min, x_max;
    unsigned char y_min, y_max;
    unsigned char invert_x, invert_y;
} KoalaCal;


void koala_read(KoalaSample *s);
int koala_scale(unsigned char raw, unsigned char lo, unsigned char hi,
                        unsigned char invert, int size);                        
void koala_calibrate(KoalaCal *c);

#endif