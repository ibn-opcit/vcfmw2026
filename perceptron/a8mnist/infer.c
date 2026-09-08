#include <stdint.h>
#include <stdio.h>
#include <conio.h>
#include "smul8.h"

// Q8.8 fixed‑point scale constants
//#define SCALE_Q8_8       433   // weight_scale * input_scale
//#define BIAS_SCALE_Q8_8  325   // bias_scale
#define SCALE_Q8_8       1
#define BIAS_SCALE_Q8_8  1

extern int8_t model_weights[];   // 10 × 784 int8
extern int8_t model_bias[];      // 10 int8

// Compute logit for 1 digit
void score_a_digit(uint8_t *input, uint8_t d, int *score) {
    int idx = d * 784;
    int acc = 0;
    int i;
    int bias_scaled;
    for (i = 0; i < 784; i++) {
        // Fastcall mul8: b in A, a on stack
        int prod = SMUL8(model_weights[idx + i], input[i]);
        acc += (prod >> 8);
    }

    // ---- Apply Q8.8 scaling ----
    // acc_scaled = acc_raw * SCALE / 256
    //acc_scaled = (acc * SCALE_Q8_8) >> 8;
    //acc_scaled = acc;

    // ---- Add scaled bias ----
    bias_scaled = (model_bias[d] * BIAS_SCALE_Q8_8) >> 8;

    *score = acc + bias_scaled;
    //printf("butt %d, %d, %d, %d \n", d, output[d], acc, bias_scaled);
    return;
}

// Compute logits for all 10 digits
void infer_digit(uint8_t *input, int *output)
{
    uint8_t d;
    uint16_t i;
    int acc;
    int bias_scaled;
    //int acc_scaled;
    
    /*
    int result;

    result = SMUL8(-10, 5);
    printf("%d\n", result);   // expect -50 

    result = SMUL8(-12, -12);
    printf("%d\n", result);   // expect 144

    result = SMUL8(127, 127);
    printf("%d\n", result);   // expect 16129
    */  
    /*
    for (d=0; d < 10; d++){
        printf("mb[%d] = %d\n", d, model_bias[d]);
    }
    cgetc();
    */
    //printf("butt %x, %x, %x, %d, %x", a, b << 8, (int8_t)a | (b << 8), c, c);
    //cgetc();
    for (d = 0; d < 10; d++) {

        int idx = d * 784;
        acc = 0;

        //printf("%d %d %d %d", input[0], input[1], input[2], input[3]);
        // ---- Inner dot‑product loop ----
        for (i = 0; i < 784; i++) {

            // Convert input pixel to signed char
            // (your model expects 0–255 mapped to int8)
            //signed char x = (signed char)input[i];
            //signed char x = 10;

            // Weight is already int8
            //signed char w = model_weights[idx + i];
            //signed char w = -5;

            // Fastcall mul8: b in A, a on stack
            int prod = SMUL8(model_weights[idx + i], input[i]);
            //int prod = -50;
            acc += (prod >> 8);
            //acc += prod;
            //printf("i %d, w %d, x %d, prod %d, acc %d \n", i, model_weights[idx + i], input[i], prod, acc);
            //cgetc();
        }
        //printf("d %d, acc %d, mb[d] %d\n", d, acc, model_bias[d]);

        // ---- Apply Q8.8 scaling ----
        // acc_scaled = acc_raw * SCALE / 256
        //acc_scaled = (acc * SCALE_Q8_8) >> 8;
        //acc_scaled = acc;

        // ---- Add scaled bias ----
        bias_scaled = (model_bias[d] * BIAS_SCALE_Q8_8) >> 8;

        output[d] = acc + bias_scaled;
        printf("butt %d, %d, %d, %d \n", d, output[d], acc, bias_scaled);
    }
}