#ifndef SMUL8_H
#define SMUL8_H

/* Multiply two signed bytes, return signed 16-bit result */
//int __fastcall__ smul8(signed char a, signed char b);
/* Pack two signed chars into one int to avoid the software stack */
int __fastcall__ smul8(int args);

/* Convenience macro to call it cleanly */
#define SMUL8(a, b) smul8(((signed char)(a) << 8) | (unsigned char)(b))

#endif