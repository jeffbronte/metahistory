#include <stdint.h>
#include <string.h>

#include "ipserver.h"
#include "debug.h"

#define RADIX 14

static void _perform_mixing(int n_samples, 
                    int n_srcs, 
                    int16_t dest[], 
                    int16_t src[][n_samples], 
                    int en[], 
                    uint16_t vol[]) 
{

    // repeat calculations for each sample
    for (int sample_no = 0; sample_no < n_samples; ++sample_no) {

        // start with existing audio
        int64_t accum = (int64_t)dest[sample_no] << RADIX;
        // mix in each source if enabled
        for (int src_no = 0; src_no < n_srcs; ++src_no) {
            if (en[src_no]) {
                // get gain from volume table, or use 1.0 if no volume
                // do 32-bit multiply and 64-bit accumulate
                accum += (int64_t)((int32_t)( src[src_no][sample_no] )* (int32_t)( volume_gain_table[vol[src_no]] ));
//                accum += (int64_t)((int32_t)( src[src_no][sample_no] )* (int32_t)(  (1 << RADIX) ));
            }
        }

        // round
        accum += (1LL << (RADIX - 1));

        // shift out radix
        accum >>= RADIX;

        // saturate
        if      (accum > INT16_MAX) dest[sample_no] = INT16_MAX;
        else if (accum < INT16_MIN) dest[sample_no] = INT16_MIN;
        else                        dest[sample_no] = (int16_t)accum;
    }
}


void perform_radio_mixing(globals_t *g, bool mute)
{
    _perform_mixing(g->audio_bufsize,R_SOURCES, g->audio_buf, g->r_sources, g->r_enables, g->r_volumes);
}

void perform_flight_mixing(globals_t *g, mic_e off1, mic_e off2)
{
    switch(g->whoami) {
        case CAPTAIN: 
            if (off1) {
                memcpy(g->f_sources[off_1], F->audio_buf, BLOCK_SIZE);
                g->f_enables[off_1] = 1;
             }
            if (off2) {
                memcpy(g->f_sources[off_2], O->audio_buf, BLOCK_SIZE);
                g->f_enables[off_2] = 1;
             }
        break;
        case FIRST_OFFICER:
            if (off1) {
                memcpy(g->f_sources[off_1], C->audio_buf, BLOCK_SIZE);
                g->f_enables[off_1] = 1;
             }
            if (off2) {
                memcpy(g->f_sources[off_2], O->audio_buf, BLOCK_SIZE);
                g->f_enables[off_2] = 1;
             }
        break;
        case OBSERVER:
            if (off1) {
                memcpy(g->f_sources[off_1], F->audio_buf, BLOCK_SIZE);
                g->f_enables[off_1] = 1;
             }
            if (off2) {
                memcpy(g->f_sources[off_2], C->audio_buf, BLOCK_SIZE);
                g->f_enables[off_2] = 1;
             }
        break;
        default: return;
    }
    _perform_mixing(g->audio_bufsize,F_SOURCES, g->audio_buf, g->f_sources, g->f_enables, g->f_volumes);
    g->f_enables[off_1] = g->f_enables[off_2] = 0;
}

/* ashr
 *
 *  preserves the sign bit for an arithmetic shift right
 *
 */
int ashr(int x, int n)
{
    if (x < 0 && n > 0)
        return x >> n | ~(~0U >> n);
    else
        return x >> n;
}

/* sat16
 *
 * Assumes x is half the size coming in and checks for saturation
 * at half of the MAX positive (0x3fff instead of 0x7fff)
 * and for MAX negative (-16384 instead of -32768)
 *
 * The caller should left shift by 1 after the call to this function
 */
fract16 sat16(fract16 x)
{
    int16_t s;
    if (x > (0x3FFF))
        s = (0x3FFF);
    else if (x < -16384)
        s = -16384;
    else s = x;

    return ((int16_t)s);
}

/* q_mul
 *
 * Multiply 2 fixed point 1.15 format numbers
 *
 * Multiply into a 32 bit number to preserve precision
 *
 */
fract16 q_mul(fract16 a, fract16 b)
{
    int16_t result, t1;
    int32_t temp, a1, b1;


    a1 = ashr(a,1);         // Start at half size

    b1 = ashr(b,1);         // Start at half size
    temp = a1 * b1;
    temp += 1;              // Add 1 for rounding
    temp <<= 1;             // Get rid of extra sign bit
    t1 = temp >> 16;        // convert back to 1.15 format, sign bit preserved with 16 bit shift

    result = sat16(t1<<1);  // Saturate at half the size

    result <<=1;            //Convert back to full size result

    return (result);
}

/* q_add
 *
 * Add 2 fixed point 1.15 format numbers
 *
 *
 */
fract16 q_add(fract16 a, fract16 b)
{
    int16_t result;
    int32_t temp;

    temp = (int32_t)a + (int32_t)b;
    if (temp > 0x7fff)
        temp = 0x7fff;
    if (temp < (-1 * 0x8000))
        temp = (-1 * 0x8000);

    result = temp;

    return (result);
}

