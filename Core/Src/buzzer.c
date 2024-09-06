/** ***************************************************************************
 * @file
 * @brief   Initializes and controls the buzzer.
 * @note    The buzzer must be initialized once with the BUZZER_init() function before use.
 *
 * @author  Marco Rau, raumar02@students.zhaw.ch
 * @date    27.12.2022
 *****************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/

#include "buzzer.h"

/******************************************************************************
 * Variables
 *****************************************************************************/

static bool flag_piezo = false;         ///< pin state on = true / off = false
static bool flag_buzzer = false;        ///< state of buzzer on = true / off = false

static uint32_t tim_prescaler = 2100;   ///< prescaler for clock freq. = 2 kHz

static const int16_t note[]={           ///< notes from C5 to B6

//  0   1   2   3   4   5   6   7   8   9   10  11
//  C5  C#5 D5  D#5 E5  F5  F#5 G5  G#5 A5  A#5 B5
     523,554,587,622,659,698,740,784,831,880,932,988,

//  12   13   14   15   16   17   18   19   20   21   22   23
//  C6   C#6  D6   D#5  E6   F6   F#6  G6   G#6  A6   A#6  B6
    1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1866,1976
};

/******************************************************************************
 * Functions
 *****************************************************************************/

/** ***************************************************************************
 * @brief Initialize Buzzer
 *
 * @note Call BUZZER_turn_on() to turn the buzzer on.
 *****************************************************************************/
void BUZZER_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();           // Enable clock for port A
    GPIOA->MODER |= GPIO_MODER_MODER5_0;    // Configure PA5 as output
    GPIOA->BSRR = GPIO_BSRR_BR5;            // Bit Reset to turn PA5 off

    __HAL_RCC_TIM5_CLK_ENABLE();        // Enable Clock for TIM5
    TIM5->PSC   = tim_prescaler;        // Prescaler for clock freq. = 2kHz
    TIM5->ARR   = TIM_TOP;              // Auto reload = counter top value
    TIM5->CR1  |= TIM_CR1_URS;          // Interrupt when overflow/underflow
    TIM5->DIER |= TIM_DIER_UIE;         // Enable Interrupt
    TIM5->EGR  |= TIM_EGR_UG;           // Update settings

    NVIC_ClearPendingIRQ(TIM5_IRQn);    // Clear pending interrupt on line 0
    NVIC_EnableIRQ(TIM5_IRQn);          // Enable Interrupt
}

/** ***************************************************************************
 * @brief Set a frequency
 * @param [in] frequency
 *****************************************************************************/
void BUZZER_set_freq(uint32_t freq)
{
    tim_prescaler = (F_CLK) / (freq * (TIM_TOP + 1) / 2);    // Calculation for Prescaler
    TIM5->PSC    = tim_prescaler;                            // Prescaler for clock frequency
}


/** ***************************************************************************
 * @brief Set a note
 * @param [in] note
 *
 * @note note refers to the array note
 *****************************************************************************/
void BUZZER_set_note(uint8_t set_note)
{
    tim_prescaler = (F_CLK) / (note[set_note] * (TIM_TOP + 1) / 2);    // Calculation for Prescaler
    TIM5->PSC    = tim_prescaler;                                      // Prescaler for clock frequency
}


/** ***************************************************************************
 * @brief Turn the buzzer on
 *****************************************************************************/
void BUZZER_turn_on(void)
{
    TIM5->CR1  |= TIM_CR1_CEN;    // Turn TIM5 on
    flag_buzzer = true;
}

/** ***************************************************************************
 * @brief Turn the buzzer off
 *****************************************************************************/
void BUZZER_turn_off(void)
{
    TIM5->CR1  &= ~TIM_CR1_CEN;     // Turn TIM5 off
    GPIOA->BSRR = GPIO_BSRR_BR5;    // Set PA5 to low
    flag_buzzer = false;
}

/** ***************************************************************************
 * @brief Check state of Buzzer
 * @return [out] state
 *
 * true when buzzer on or false when buzzer off
 *****************************************************************************/
bool BUZZER_get_status(void){
    return flag_buzzer;
}

/** ***************************************************************************
 * @brief Set a frequency
 * @param [in] note
 * @param [in] duration [ms]
 *****************************************************************************/
void BUZZER_play_note(uint16_t note,uint16_t length)
{
    BUZZER_set_freq(note);
    BUZZER_turn_on();

    HAL_Delay(length);

    BUZZER_turn_off();
}

/** ***************************************************************************
 * @brief Play Nokia ringtone
 *****************************************************************************/
void BUZZER_play_melody(void)
{
    BUZZER_play_note(note[16],150); // E6
    BUZZER_play_note(note[14],150); // D6
    BUZZER_play_note(note[18],300); // F#6
    BUZZER_play_note(note[20],300); // G#6

    BUZZER_play_note(note[13],150); // C#6
    BUZZER_play_note(note[11],150); // B5
    BUZZER_play_note(note[14],300); // D6
    BUZZER_play_note(note[16],300); // E6

    BUZZER_play_note(note[11],150); // B5
    BUZZER_play_note(note[9],150);  // A5
    BUZZER_play_note(note[13],300); // C#6
    BUZZER_play_note(note[16],300); // E6

    BUZZER_play_note(note[21],450); // A6
}

/** ***************************************************************************
 * @brief Interrupt when counter of TIM5 is full
 *
 * Switches buzzer by default with 2 kHz or with set frequency of BUZZER_set_freq(uint32_t freq)
 * @note Interrupt will accrue only when BUZZER_get_status(void) = true
 *****************************************************************************/
void TIM5_IRQHandler(void)
{
    TIM5->SR &= ~TIM_SR_UIF;    // Clear pending interrupt flag
    if(flag_piezo){
        GPIOA->BSRR = GPIO_BSRR_BS5; // Set PA5 to high
    }
    else{
        GPIOA->BSRR = GPIO_BSRR_BR5; // Set PA5 to low
    }

    flag_piezo = !flag_piezo;   // Toggle flag
}
