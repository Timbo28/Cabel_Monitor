/** ***************************************************************************
 * @file
 * @brief See calculations.c
 *
 *
 *
 *****************************************************************************/

#ifndef CALCULATIONS_H_
#define CALCULATIONS_H_


/******************************************************************************
 * Includes
 *****************************************************************************/

#include "arm_math.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/



/******************************************************************************
 * Functions
 *****************************************************************************/
void calculate_pos(int num_of_samples);
void split_Array(void);
void calculate_RMS(void);
void calculate_FFT (void);
void FFT_Init(void);
void clear_Buffer (void);
void distance_LUT(void);
void calculate_current(void);
void check_display_bounderies(void);
void averaging_FFT_semples(void);
int  get_X_Pos(void);
int  get_Y_Pos(void);
int  get_angle(void);
float  get_current(void);
#endif
