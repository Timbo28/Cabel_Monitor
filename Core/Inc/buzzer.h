/** ***************************************************************************
 * @file
 * @brief See buzzer.c
 *
 * Prefix BUZZER
 *
 *****************************************************************************/

#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

/******************************************************************************
 * Includes
 *****************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "stm32f4xx.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define F_CLK   21000000    ///< Frequency of System
#define TIM_TOP 9           ///< Highest counter value

/******************************************************************************
 * Functions
 *****************************************************************************/
void BUZZER_init(void);
void BUZZER_turn_on(void);
void BUZZER_turn_off(void);

void BUZZER_set_freq(uint32_t freq);
void BUZZER_set_note(uint8_t note);

bool BUZZER_get_status(void);

void BUZZER_play_note(uint16_t note,uint16_t length);
void BUZZER_play_melody(void);

#endif /* INC_BUZZER_H_ */
