/** ***************************************************************************
 * @file
 * @brief Calculates the position to the cable and the current with the values from the ADC(s)
 *
 * Cable Position
 * ===============
 * To calculate the cable position the function calculate_pos() is using the law of cosines to determine alpha and beta.
 * With those two angles the function is then calculating the X, Y position and the angle gamma, as shown in the picture below.
 *
 * @image html angel_pads.png width=35%
 * @n a = left pad
 * @n b = right pad
 *
 * @note Special care must be taken with the sign of the X position.
 * Here the right side of the zero point is negative and the left side is positive.
 *
 *
 * Averaging
 * =========
 * By calling calculate_pos() function it is possible to change the number of samples,
 * which should be averaged before the the distance value get loaded in to the "LPAD_distance" and "RPAD_distance" variables.
 * With the parameter of this function it is possible to change this number of samples which should be averaged.
 *
 * @note The parameter of the calculate_pos function should not be less than 1 and greater than 3,
 * otherwise the macros FFT_AVG_NUMS must be changed to a higher number.
 *
 * Current
 * =======
 *
 * By calling the calculate_pos function the actually current also will be calculated and stored in the "current" variable.
 * For this, the current can only be calculated if the distance (Y_pos) is between 15 mm and 25 mm and the angle is in the range of -+15 degrees.
 * If these requirements are not fulfilled, an error code is stored in the "current" variable.
 *
 * Error codes
 * ===========
 * The whole calculations.c file is using error codes from the header file error_code.h.
 * This error codes will be stored in the corresponding variables which can not be calculated.
 *
 *
 *
 *
 * ----------------------------------------------------------------------------
 * @author Tim Roos, roostim1@students.zhaw.ch
 * @date   27.12.2022
 *****************************************************************************/



/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"
#include <math.h>

#include "measuring.h"
#include "calculations.h"
#include "error_code.h"

/******************************************************************************
 * Defines
 *****************************************************************************/

#define FFT_AVG_NUMS    3               ///< Max size of average array (LPAD_FFT_avg_array[]).
#define PAD_SPACING     50              ///< Space between pads in mm.
#define RAD_TO_DEGREE   57.295779513;   ///< Factor to calculate from rad to degree.
#define MAX_Y_DISTANCE  200             ///< Max distance to cable.
#define MAX_X_DISTANCE  100             ///< Max offset to cable.
#define CURRENT_FACTOR  0.357              ///< Is used to transform the voltage from The Hall sensor to a current.

/******************************************************************************
 * Variables
 *****************************************************************************/


static float32_t LPAD_samples [ADC_NUMS];          ///< Copy of ADC values for the left pad.
static float32_t RPAD_samples [ADC_NUMS];       ///< Copy of ADC values for the right pad.
static float32_t LHALL_samples [ADC_NUMS];      ///< Copy of ADC values for the left Hall.
static float32_t RHALL_samples [ADC_NUMS];      ///< Copy of ADC values for the right Hall.

static float32_t LPAD_FFT [ADC_NUMS];           ///< Output Array of the FFT for the left pad.
static float32_t RPAD_FFT [ADC_NUMS];           ///< Output Array of the FFT for the right pad.
static float32_t LHALL_FFT [ADC_NUMS];          ///< Output Array of the FFT for the left Hall.
static float32_t RHALL_FFT [ADC_NUMS];          ///< Output Array of the FFT for the right Hall.

static uint32_t LPAD_FFT_avg_array[FFT_AVG_NUMS];   ///< Array which contains multiple values of the left pad after the FFT in the range of 50 Hz.
static uint32_t RPAD_FFT_avg_array[FFT_AVG_NUMS];   ///< Array which contains multiple values of the right pad after the FFT in the range of 50 Hz.
static uint32_t LHALL_FFT_avg_array[FFT_AVG_NUMS];  ///< Array which contains multiple values of the left Hall after the FFT in the range of 50 Hz.
static uint32_t RHALL_FFT_avg_array[FFT_AVG_NUMS];  ///< Array which contains multiple values of the right Hall after the FFT in the range of 50 Hz.

static int32_t LPAD_FFT_distance=0;     ///< Variable which contains the distance of the cable to the left pad.
static int32_t RPAD_FFT_distance=0;     ///< Variable which contains the distance of the cable to the right pad.
static int32_t LHALL_FFT_voltage=0;     ///< Variable which contains the voltage on the left Hall.
static int32_t RHALL_FFT_voltage=0;     ///< Variable which contains the voltage on the right Hall.

static int    X_Pos;                   ///< Contains the X position to the cable (the offset to the right and left to the cable).
static int    Y_Pos;                   ///< Contains the Y position to the cable (the distance).
static double Gamma;                   ///< Contains the angle of the device to the cable.
static float  current;                 ///< Contains the current of the cable.

static int avg_counter=0;              ///< Counts the amount of average values in the in the " "_FFT_avg_array.
int        num_of_samples;             ///< Contains the number of ADC values should be averaged.

const int32_t LPAD_LUT[] = {
     #include "LPAD_lut.csv"
 };                                   ///< The array is initialised with the values stored in the LPAD_lut.csv file and is used as a luck up table.

const int32_t RPAD_LUT[] = {
     #include "RPAD_lut.csv"
 };                                  ///< The array is initialised with the values stored in the RPAD_lut.csv file and is used as a luck up table.

arm_rfft_fast_instance_f32 fft_handler; ///< This struct is needed to initialise the arm_rfft_fast_f32() function.
/******************************************************************************
 * Functions
 *****************************************************************************/

/** ***************************************************************************
 * @brief Returns the X position.
 *
 * This is used to access the X_Pos from an other file.
 * @return X_Pos
 *****************************************************************************/
int get_X_Pos(void)
{
     return X_Pos;
}
/** ***************************************************************************
 * @brief Returns the Y position.
 *
 * This is used to access the Y_Pos from an other file.
 * @return Y_Pos
 *****************************************************************************/
int get_Y_Pos(void)
{
     return Y_Pos;
}
/** ***************************************************************************
 * @brief Returns the angle.
 *
 * This is used to access Gamma from an other file.
 * @return Gamma
 *****************************************************************************/
int get_angle(void)
{
     return (int)Gamma;
}
/** ***************************************************************************
 * @brief Returns the current.
 *
 * This is used to access the current from an other file.
 * @return current
 *****************************************************************************/
float get_current(void)
{
    return current;
}

/** ***************************************************************************
 * @brief Calculate angle, X and Y Position of the cable, from the FFT value.
 *
 * @param Number of FFT output values to be averaged before calculating with it.
 *
 * @note The zero point is at the leading edge of the device between the two pads.
 *****************************************************************************/
void calculate_pos(int fft_avg_num)
{
     num_of_samples = fft_avg_num;
     double cos_alpha;
     double alpha;
     double cos_beta;
     double beta;

     ADC3_IN4_timer_init();
     ADC3_IN4_timer_start();

     if (MEAS_data_ready){
          /* Sets the error code as a default value*/
          X_Pos = CALC_OUTOF_X_RANGE; // ERROR code
          Y_Pos = CALC_OUTOF_Y_RANGE; // ERROR code
          Gamma = CALC_OUTOF_ANGLE_RANGE; // ERROR code

          split_Array();
          calculate_FFT();
          clear_Buffer();
          /*Checks if there is no error code from the FFT function*/
          if(LPAD_FFT_distance != FFT_NO_SIGNAL || RPAD_FFT_distance != FFT_NO_SIGNAL){

            cos_beta   = (double)( LPAD_FFT_distance*LPAD_FFT_distance
                                  + PAD_SPACING*PAD_SPACING
                                  - RPAD_FFT_distance*RPAD_FFT_distance)
                                  / (2*LPAD_FFT_distance*PAD_SPACING);  // Law of cosines

            cos_alpha = (double)( RPAD_FFT_distance*RPAD_FFT_distance
                                  - LPAD_FFT_distance*LPAD_FFT_distance
                                  + PAD_SPACING*PAD_SPACING)
                                  / (2*RPAD_FFT_distance*PAD_SPACING); // Law of cosines
            /* Checks if cos_alpha and cos_beta are in range of arccosine*/
            if(cos_alpha <= 1 && cos_beta <= 1 && cos_alpha > -1 && cos_beta >-1){

                alpha = acos(cos_alpha);   //Left angle
                beta  = acos(cos_beta);    //Right angle

                if (alpha < M_PI/2 && beta < M_PI/2){// First case : cable in between pads
                    X_Pos = fabs((cos(alpha)*RPAD_FFT_distance - PAD_SPACING/2));
                    Y_Pos = sin(alpha)*RPAD_FFT_distance;
                }
                else if (alpha >= M_PI/2 && beta < M_PI/2){ //Second case: cable on the left side of the device
                    X_Pos = cos(M_PI-alpha)*RPAD_FFT_distance + PAD_SPACING/2;
                    Y_Pos = sin(M_PI-alpha)*RPAD_FFT_distance;
                }
                else if (alpha < M_PI/2 && beta >= M_PI/2){ //Third case: cable on the right side of the device
                    X_Pos = (cos(M_PI-beta)*LPAD_FFT_distance + PAD_SPACING/2);
                    Y_Pos =  sin(M_PI-beta)*LPAD_FFT_distance;
                }

                Gamma= atan2(Y_Pos,X_Pos );
                /*sets the correct signs for X_Pos and gamma*/
                if(alpha < beta){//Check if cable is on the right side of the zero point.

                    Gamma = M_PI/2-Gamma;/* Angle minus 90 degree, so that angel is zero degree
                                                 if cable is in front of the device and a negative angle
                                                 when we move the cable to the right.*/
                    X_Pos = -X_Pos;
                }
                else {                     // If cable is on the left side of the zero point.
                     Gamma -= M_PI/2;       /* 90 degree minus angle, so that angel is zero degree
                                               if cable is in front of the device and a positive
                                               angle when we move the cable to the left.*/
                }
                Gamma=Gamma*RAD_TO_DEGREE;

                calculate_current();
                check_display_bounderies();
            }
          }
     }
}
/** ***************************************************************************
 * @brief Calculate the current
 *
 * with the magnetic field detected by the Hall sensor.
 *
 *****************************************************************************/
void calculate_current(void)
{
     /* Checks that the distance between the cable and the Hall sensor is not too large*/
     if(Y_Pos > 15 && Y_Pos < 25  ){

        /*Checks that the angle between the cable and the Hall sensor is not too large.*/
        if(Gamma < 15 && Gamma > -15){

           /* Detection of the higher Hall sensor voltage and storage of the higher voltage in the current variable */
           if(RHALL_FFT_voltage > LHALL_FFT_voltage){
               current = (RHALL_FFT_voltage*CURRENT_FACTOR*Y_Pos)/1000;
            }else{
               current = (LHALL_FFT_voltage*CURRENT_FACTOR*Y_Pos)/1000;
            }

        }else{
             current = CURR_OUTOF_Angle_RANGE; // ERROR code
        }
     }else{
          current = CURR_OUTOF_Y_RANGE;// ERROR code
     }
}
/** ***************************************************************************
 * @brief Checks if the X_Pos and the Y_Pos are not too large to be displayed on the Screen.
 *
 * If they are too large an error code will be saved in the X_Pos or the Y_Pos.
 *
 *****************************************************************************/
void check_display_bounderies(void)
{
      if(fabs(X_Pos) > MAX_X_DISTANCE){
         X_Pos = CALC_OUTOF_X_RANGE;// ERROR code
     }

     if(Y_Pos > MAX_Y_DISTANCE){
          Y_Pos = CALC_OUTOF_Y_RANGE;// ERROR code
     }
}
/** ***************************************************************************
 * @brief Transformers the ADC samples in to the frequency domain with a FFT.
 *
 * The ADC values at50 Hz from both pads and both Hall will be saved in the
 * {LPAD_FFT_avg_array[],RPAD_FFT_avg_array[],LHALL_FFT_avg_array[],RHALL_FFT_avg_array[]}
 * at the position of the avg_counter.
 *
 *****************************************************************************/
void calculate_FFT (void)
{

     uint8_t RFFT =0;  // Flag which declares that an real fast Fourier transformation is required.
     /* Writes real and imaginary part of different frequencies (from 0 to 320 Hz with delta-frequency) in LPAD_FFT.*/
     arm_rfft_fast_f32(&fft_handler,LPAD_samples, LPAD_FFT,RFFT);
     LPAD_FFT_avg_array[avg_counter]=(uint32_t)(hypot(LPAD_FFT[10],LPAD_FFT[11])*sqrt(2)/ADC_NUMS);  /* 50 Hz real part is at position 10 and
                                                                                                      imaginary part is at position 11 of the LPAD_FFT[] array.*/
     arm_rfft_fast_f32(&fft_handler,RPAD_samples,RPAD_FFT,RFFT);
     RPAD_FFT_avg_array[avg_counter]= (uint32_t)(hypot(RPAD_FFT[10],RPAD_FFT[11])*sqrt(2)/ADC_NUMS);
     arm_rfft_fast_f32(&fft_handler,LHALL_samples, LHALL_FFT,RFFT);
     LHALL_FFT_avg_array[avg_counter]=(uint32_t)(hypot(LHALL_FFT[10],LHALL_FFT[11])*sqrt(2)/ADC_NUMS);

     arm_rfft_fast_f32(&fft_handler,RHALL_samples, RHALL_FFT,RFFT);
     RHALL_FFT_avg_array[avg_counter]=(uint32_t)(hypot(RHALL_FFT[10],RHALL_FFT[11])*sqrt(2)/ADC_NUMS);

    averaging_FFT_semples();
}

/** ***************************************************************************
 * @brief Averaging several FFT output values for each pad and Hall sensor.
 *
 * The number of samples (num_of_samples) which will be averaged, is defined in the function calculate_pos.
 *
 * The averages will be saved in the variables {LPAD_FFT_distance, RPAD_FFT_distance, LHALL_FFT_voltage, RHALL_FFT_voltage}.
 *
 *****************************************************************************/
void averaging_FFT_semples(void)
{

      if(avg_counter == num_of_samples-1){

          LPAD_FFT_distance=0;
          RPAD_FFT_distance=0;
          LHALL_FFT_voltage =0;
          RHALL_FFT_voltage =0;

          for(int i =0; i < num_of_samples; i++){                 //If the desired number of samples is achieved, they get summed up.

               LPAD_FFT_distance += LPAD_FFT_avg_array[i];
               RPAD_FFT_distance += RPAD_FFT_avg_array[i];
               LHALL_FFT_voltage += LHALL_FFT_avg_array[i];
               RHALL_FFT_voltage += RHALL_FFT_avg_array[i];
          }

          LPAD_FFT_distance = LPAD_FFT_distance/(num_of_samples); //The summed up samples get divided by the number of samples to get the average.
          RPAD_FFT_distance = RPAD_FFT_distance/(num_of_samples);
          LHALL_FFT_voltage = LHALL_FFT_voltage/(num_of_samples);
          RHALL_FFT_voltage = RHALL_FFT_voltage/(num_of_samples);
          avg_counter = 0;
          distance_LUT();
     }else{
          avg_counter++;
     }

}

/** ***************************************************************************
 * @brief Determines the distance of the cable to the pads from a look-up table.
 *
 *****************************************************************************/
void distance_LUT(void)
{
     const int LPAD_Max = 1458;
     const int LPAD_Min = 200;
     const int RPAD_Max = 1466;
     const int RPAD_Min = 200;

     if(LPAD_FFT_distance > LPAD_Min && LPAD_FFT_distance < LPAD_Max){

          LPAD_FFT_distance = LPAD_LUT[LPAD_FFT_distance-LPAD_Min];

     }else if(LPAD_FFT_distance > LPAD_Max){

          LPAD_FFT_distance = 0;

     }else{
          LPAD_FFT_distance = FFT_NO_SIGNAL; // ERROR code
     }


     if(RPAD_FFT_distance > RPAD_Min && RPAD_FFT_distance < RPAD_Max){

               RPAD_FFT_distance = RPAD_LUT[RPAD_FFT_distance-RPAD_Min];

          }else if(RPAD_FFT_distance > RPAD_Max){

               RPAD_FFT_distance = 0;

          }else{
               RPAD_FFT_distance = FFT_NO_SIGNAL; // ERROR code
          }

}
/** ***************************************************************************
 * @brief Splits the ADC_Samples array from measuring.c into four arrays.
 *
 * A copy of each Array will be saved in { LPAD_samples, RPAD_samples, LHALL_samples, RHALL_samples}.
 *
 *****************************************************************************/
void split_Array(void)
{
     int j=0;
     for(int i =0; i<(4*ADC_NUMS);i+=4){
          LPAD_samples[j++] =MEAS_return_data(i);
     }
     j=0;
     for(int i =1; i<4*ADC_NUMS;i+=4){
          RPAD_samples[j++] = MEAS_return_data(i);
     }
     j=0;
     for(int i =2; i<4*ADC_NUMS;i+=4){
          LHALL_samples[j++] = MEAS_return_data(i);
     }
     j=0;
     for(int i =3; i<4*ADC_NUMS;i+=4){
          RHALL_samples[j++] =MEAS_return_data(i);
     }

}
/** ***************************************************************************
 * @brief Initialisation for the FFT function
 *
 * @note Needs to be initialised only ones before using the arm_rfft_fast_f32() function.
 *****************************************************************************/
void FFT_Init(void)
{
     arm_rfft_fast_init_f32(&fft_handler, ADC_NUMS);
}
/** ***************************************************************************
 * @brief Clearing all four ADS samples arrays
 *
 *****************************************************************************/
void clear_Buffer (void)
{
     /* Clear buffer and flag */
     for (uint32_t i = 0; i < ADC_NUMS/2; i++){
          RPAD_samples[2*i] = 0;
          RPAD_samples[2*i+1] = 0;
          LPAD_samples[2*i] = 0;
          LPAD_samples[2*i+1] = 0;
          RHALL_samples[2*i] = 0;
          RHALL_samples[2*i+1] = 0;
          LHALL_samples[2*i] = 0;
          LHALL_samples[2*i+1] = 0;
     }
}

