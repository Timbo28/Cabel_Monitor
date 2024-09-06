/** ***************************************************************************
 * @file
 * @brief Sets up the microcontroller, the clock system and the peripherals.
 *
 * Initialization is done for the system, the blue user button, the user LEDs,
 * and the LCD display with the touchscreen.
 * @n Then the code enters an infinite while-loop, where it checks for
 * user input and starts the requested measurement.
 *
 * @author  Marco Rau, raumar02@students.zhaw.ch
 * @date    27.12.2022
 *****************************************************************************/


/******************************************************************************
 * Includes
 *****************************************************************************/

#include "stdio.h"

#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"

#include "main.h"
#include "pushbutton.h"
#include "menu.h"
#include "measuring.h"
#include "buzzer.h"
#include "calculations.h"


/******************************************************************************
 * Defines
 *****************************************************************************/

#define NOTHING         1   ///< Task: empty
#define SINGLE_MEAS     2   ///< Task: Single measurement
#define AVERAGE_MEAS    3   ///< Task: Average measurement

#define MAX_SUBTASKS    2   ///< Max Subtasks
#define SUB_VALUES      1   ///< Subtask: Show measurement in numbers
#define SUB_GRAPHIC     2   ///< Subtask: Show measurement visualized

#define MAX_TABLES      1   ///< Max Tables --> 1: one phase / 2: one phase and two phase
#define TABLE_ONE_PHASE 1   ///< Table: one phase
#define TABLE_TWO_PHASE 2   ///< Table: two phase

#define MAX_DISTANCE    200 ///< Needed for buzzer feedback


/******************************************************************************
 * Functions
 *****************************************************************************/

static void SystemClock_Config(void);   ///< System Clock Configuration
static void gyro_disable(void);         ///< Disable the onboard gyroscope

/** ***************************************************************************
 * @brief  Main function
 * @return not used because main ends in an infinite loop
 *
 * Initialization and infinite while loop
 *****************************************************************************/
int main(void) {
    HAL_Init();                         // Initialize the system

    SystemClock_Config();               // Configure system clocks

#ifdef FLIPPED_LCD
    BSP_LCD_Init_Flipped();             // Initialize the LCD for flipped orientation
#else
    BSP_LCD_Init();                     // Initialize the LCD display
#endif
    BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, LCD_FRAME_BUFFER);
    BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);
    BSP_LCD_DisplayOn();
    BSP_LCD_Clear(LCD_COLOR_WHITE);

    BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());    // Touchscreen

    PB_init();                  // Initialize the user pushbutton
    PB_enableIRQ();             // Enable interrupt on user pushbutton

    BSP_LED_Init(LED3);         // Toggles in while loop
    BSP_LED_Init(LED4);         // Is toggled by user button

    MENU_draw();                // Draw the menu
    MENU_hint();                // Show hint at startup

    gyro_disable();             // Disable gyro, use those analog inputs

    MEAS_GPIO_analog_init();    // Configure GPIOs in analog mode
    MEAS_timer_init();          // Configure the timer

    BUZZER_init();              // Configure buzzer

    FFT_Init();                 // Configure FFT

    // Task
    uint8_t task        = NOTHING;
    uint8_t subtask     = SUB_GRAPHIC;
    uint8_t table_cable = TABLE_ONE_PHASE;

    uint8_t task_old        = task;
    uint8_t subttask_old    = subtask;
    uint8_t table_cable_old = table_cable;

    // Measurement
    int16_t  x_distance = 0;
    int16_t  y_distance = 0;
    int16_t  angle      = 0;
    float    current    = 0.0;

    uint8_t responsive_counter = 0; //Responsiveness for touch

    bool flag_setting_change = false;
    bool flag_blue_btn       = false;

    char text[20];

    /* Infinite while loop */
    while (1) { // Infinitely loop in main function

        BSP_LED_Toggle(LED3); // Visual feedback when running

        MENU_check_transition();

        switch (MENU_get_transition()) { // Handle user menu choice
            case MENU_NONE:

                if(responsive_counter < 10){
                    responsive_counter++;
                }
                break;

            case MENU_SINGLE:
                task = SINGLE_MEAS;
                break;

            case MENU_MULTI:
                task = AVERAGE_MEAS;
                break;

            case MENU_CABLE:
                if(task != NOTHING){
                    if(responsive_counter > 2){

                        ++table_cable;

                        if(table_cable > MAX_TABLES){
                            table_cable = 1;
                        }
                    }
                    responsive_counter = 0;
                }
                break;
            case MENU_SUBTASK:
                if(task != NOTHING){
                    if(responsive_counter > 2){

                        ++subtask;

                        if(subtask > MAX_SUBTASKS){
                            subtask = 1;
                        }
                    }
                    responsive_counter = 0;
                }

                break;
            default:  // Should never occur
                break;
        }

        flag_setting_change = false;

        if(task_old != task || subttask_old != subtask || table_cable_old != table_cable){
            flag_setting_change = true;
        }

        task_old        = task;
        subttask_old    = subtask;
        table_cable_old = table_cable;

        if(flag_setting_change){

            if(task == SINGLE_MEAS && table_cable == TABLE_ONE_PHASE){
                snprintf(text, 18, "SINGLE: ONE PHASE");
            }

            else if(task == SINGLE_MEAS && table_cable == TABLE_TWO_PHASE){
                snprintf(text, 18, "SINGLE: TWO PHASE");
            }

            else if(task == AVERAGE_MEAS && table_cable == TABLE_ONE_PHASE){
                snprintf(text, 19, "AVERAGE: ONE PHASE");
            }

            else if(task == AVERAGE_MEAS && table_cable == TABLE_TWO_PHASE){
                snprintf(text, 19, "AVERAGE: TWO PHASE");
            }

            if(subtask == SUB_VALUES){
                MENU_values_init((uint8_t *)text);
            }
            else if(subtask == SUB_GRAPHIC){
                MENU_visual_init((uint8_t *)text);
            }
        }

        switch(task){

            case NOTHING:

                if (PB_pressed()){
                    BSP_LED_On(LED3);
                    BUZZER_play_melody();
                }

                break;

            case SINGLE_MEAS:

                calculate_pos(1);

                y_distance = get_Y_Pos();
                x_distance = get_X_Pos();
                angle      = get_angle();
                current    = get_current();

                reset_sample_counter();

                break;

            case AVERAGE_MEAS:

                calculate_pos(3);

                y_distance = get_Y_Pos();
                x_distance = get_X_Pos();
                angle      = get_angle();
                current    = get_current();

                reset_sample_counter();

                break;
        }

        if(task != NOTHING){
            switch(subtask){
                case SUB_VALUES:
                    MENU_values_act(x_distance,y_distance,angle,current);
                    break;
                case SUB_GRAPHIC:
                    MENU_visual_act(x_distance,y_distance,current);
                    break;
                default:
                    MENU_empty(); // Should never occur
                    break;
            }

            if (PB_pressed()) {
                BSP_LED_Toggle(LED4);
                flag_blue_btn = !flag_blue_btn;
            }

            if(flag_blue_btn && (x_distance != CALC_OUTOF_X_RANGE) && (y_distance != CALC_OUTOF_Y_RANGE)){

                if(!BUZZER_get_status()){
                    BUZZER_turn_on();
                }

                BUZZER_set_note((MAX_DISTANCE - y_distance ) / 10); // Set frequency when buzzer on
            }
            else{

                if(BUZZER_get_status()){
                    BUZZER_turn_off();
                }
            }
        }
        HAL_Delay(10);
    }
}

/** ***************************************************************************
 * @brief System Clock Configuration
 *
 *****************************************************************************/
static void SystemClock_Config(void){
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    /* Configure the main internal regulator output voltage */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /* Initialize High Speed External Oscillator and PLL circuits */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    /* Initialize gates and clock dividers for CPU, AHB and APB busses */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
    /* Initialize PLL and clock divider for the LCD */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 192;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 4;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
    /* Set clock prescaler for ADCs */
    ADC->CCR |= ADC_CCR_ADCPRE_0;
}


/** ***************************************************************************
 * @brief Disable the GYRO on the microcontroller board.
 *
 * @note MISO of the GYRO is connected to PF8 and CS to PC1.
 * @n Some times the GYRO goes into an undefined mode at startup
 * and pulls the MISO low or high thus blocking the analog input on PF8.
 * @n The simplest solution is to pull the CS of the GYRO low for a short while
 * which is done with the code below.
 * @n PF8 is also reconfigured.
 * @n An other solution would be to remove the GYRO
 * from the microcontroller board by unsoldering it.
 *****************************************************************************/
static void gyro_disable(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();       // Enable Clock for GPIO port C
    /* Disable PC1 and PF8 first */
    GPIOC->MODER &= ~GPIO_MODER_MODER1; // Reset mode for PC1
    GPIOC->MODER |= GPIO_MODER_MODER1_0;    // Set PC1 as output
    GPIOC->BSRR |= GPIO_BSRR_BR1;       // Set GYRO (CS) to 0 for a short time
    HAL_Delay(10);                      // Wait some time
    GPIOC->MODER |= GPIO_MODER_MODER1_Msk; // Analog mode PC1 = ADC123_IN11
    __HAL_RCC_GPIOF_CLK_ENABLE();       // Enable Clock for GPIO port F
    GPIOF->OSPEEDR &= ~GPIO_OSPEEDR_OSPEED8;    // Reset speed of PF8
    GPIOF->AFR[1] &= ~GPIO_AFRH_AFSEL8;         // Reset alternate func. of PF8
    GPIOF->PUPDR &= ~GPIO_PUPDR_PUPD8;          // Reset pulup/down of PF8
    HAL_Delay(10);                      // Wait some time
    GPIOF->MODER |= GPIO_MODER_MODER8_Msk; // Analog mode for PF6 = ADC3_IN4
}
