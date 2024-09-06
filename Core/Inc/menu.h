/** ***************************************************************************
 * @file
 * @brief See menu.c
 *
 * Prefix MENU
 *
 *****************************************************************************/

#ifndef MENU_H_
#define MENU_H_


/******************************************************************************
 * Includes
 *****************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "stm32f4xx.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"

#include "math.h"
#include "error_code.h"

/******************************************************************************
 * Defines
 *****************************************************************************/
#define MENU_ENTRY_COUNT        2        ///< Number of menu entries

#define TITLE_HIGHT         45      ///< Height of Title
#define MENU_FONT           &Font12 ///< Possible font sizes: 8 12 16 20 24
#define MENU_HEIGHT         40      ///< Height of menu bar
#define MENU_MARGIN         2       ///< Margin around a menu entry

/** Position of menu bar: 0 = top, (BSP_LCD_GetYSize()-MENU_HEIGHT) = bottom */
#define MENU_Y              (BSP_LCD_GetYSize()-MENU_HEIGHT)

#define MENU_COLOR          LCD_COLOR_LIGHTGRAY

/******************************************************************************
 * Types
 *****************************************************************************/
/** Enumeration of possible menu items */
typedef enum {
    MENU_SINGLE = 0, MENU_MULTI, MENU_CABLE, MENU_SUBTASK, MENU_NONE
} MENU_item_t;
/** Struct with fields of a menu entry */
typedef struct {
    char line1[16];                     ///< First line of menu text
    char line2[16];                     ///< Second line of menu text
    uint32_t text_color;                ///< Text color
    uint32_t back_color;                ///< Background color
} MENU_entry_t;


/******************************************************************************
 * Functions
 *****************************************************************************/
void MENU_hint(void);

void MENU_values_init(uint8_t *title);
void MENU_values_act(int16_t x_distance, uint16_t y_distance, int16_t angle, float current);

void MENU_visual_init(uint8_t *title);
void MENU_visual_act(int16_t x_distance, uint16_t y_distance, float current);

void MENU_no_cable(void);

void MENU_draw(void);

void MENU_clear(void);
void MENU_empty(void);
void MENU_set_entry(const MENU_item_t item, const MENU_entry_t entry);
MENU_entry_t MENU_get_entry(const MENU_item_t item);
void MENU_check_transition(void);
MENU_item_t MENU_get_transition(void);


#endif

