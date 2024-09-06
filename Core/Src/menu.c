/** ***************************************************************************
 * @file
 * @brief The menu
 *
 * Initializes and displays the menu.
 * @n   Provides the function MENU_check_transition() for polling user actions.
 *      The variable MENU_transition is set to the touched menu item.
 *      If no touch has occurred the variable MENU_transition is set to MENU_NONE
 * @n   If the interrupt handler is enabled by calling BSP_TS_ITConfig();
 *      the variable MENU_transition is set to the touched menu entry as above.
 * @n   Either call once BSP_TS_ITConfig() to enable the interrupt
 *      or MENU_check_transition() in the main while loop for polling.
 * @n   The function MENU_get_transition() returns the new menu item.
 * @n   MENU_values_act(int16_t x_distance, uint16_t y_distance, int16_t angle,
 *      float current) and MENU_visual_act(int16_t x_distance,
 *      uint16_t y_distance, float current) display show the orientation to the cable.
 *
 * @author  Hanspeter Hochreutener, hhrt@zhaw.ch and Marco Rau, raumar02@students.zhaw.ch
 * @date    27.12.2022
 *****************************************************************************/


/******************************************************************************
 * Includes
 *****************************************************************************/

#include "menu.h"

/******************************************************************************
 * Variables
 *****************************************************************************/

static MENU_item_t MENU_transition = MENU_NONE;    ///< Transition to this menu
static MENU_entry_t MENU_entry[MENU_ENTRY_COUNT] = {
        {"Average",    "Measurement",  LCD_COLOR_BLACK,    MENU_COLOR},
        {"Single",     "Measurement",  LCD_COLOR_BLACK,    MENU_COLOR},
};      ///< All the menu entries

static uint16_t x_circle_old = 20;  ///< X erase position of old data
static uint16_t y_circle_old = 20;  ///< Y erase position of old data


/******************************************************************************
 * Functions
 *****************************************************************************/

/** ***************************************************************************
 * @brief Set Layout for all values
 *
 *****************************************************************************/
void MENU_values_init(uint8_t *title)
{
    MENU_clear();
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(MENU_COLOR);

    BSP_LCD_SetTextColor(MENU_COLOR);
    BSP_LCD_FillRect(5, 5, BSP_LCD_GetXSize()-10, TITLE_HIGHT-10);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, TITLE_HIGHT/2 - 5, (uint8_t *)title, CENTER_MODE);

    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_DisplayStringAt(10, TITLE_HIGHT+20,  (uint8_t *)"X-Distance:       mm", LEFT_MODE); // offset to cable
    BSP_LCD_DisplayStringAt(10, TITLE_HIGHT+40,  (uint8_t *)"Y-Distance:       mm", LEFT_MODE); // distance to cable

    BSP_LCD_DisplayStringAt(10, TITLE_HIGHT+80,  (uint8_t *)"Distance:         mm", LEFT_MODE); // abs. distance to cable
    BSP_LCD_DisplayStringAt(10, TITLE_HIGHT+100, (uint8_t *)"Angle:            ",   LEFT_MODE); // angle to cable
    BSP_LCD_DrawCircle(210,TITLE_HIGHT+102,2);                                                  // degree (°)

    BSP_LCD_DisplayStringAt(10, TITLE_HIGHT+140, (uint8_t *)"Current:          A ", LEFT_MODE); // current in cable
}


/** ***************************************************************************
 * @brief Display actual values
 * @param [in] Y-Distance [mm]
 * @param [in] X-Distance [mm]
 * @param [in] Angle      [°]
 * @param [in] Current    [A]
 *
 * Shows the offset, distance and angle to the cable.
 * When the cable is in a certain range the current will be displayed.
 * @note Call MENU_values_init() first
 *****************************************************************************/
void MENU_values_act(int16_t x_distance, uint16_t y_distance, int16_t angle, float current)
{
    char text_x_distance[7];
    char text_y_distance[7];
    char text_abs_distance[7];
    char text_angle[7];
    char text_current[8];

    // check error code
    if(x_distance == CALC_OUTOF_X_RANGE){
        snprintf(text_x_distance, 6, "NaNs");
    }
    else{
        snprintf(text_x_distance, 5, "%4d",(int)(x_distance));
    }

    if(y_distance == CALC_OUTOF_Y_RANGE){
        snprintf(text_y_distance, 6, "NaNs");
    }
    else{
        snprintf(text_y_distance, 5, "%4d",(int)(y_distance));
    }

    if(x_distance == CALC_OUTOF_X_RANGE || y_distance == CALC_OUTOF_Y_RANGE){
        snprintf(text_abs_distance, 6, "NaNs");
    }
    else{
        snprintf(text_abs_distance, 5, "%4d",(int)(hypot(y_distance, x_distance)));
    }

    if(angle == CALC_OUTOF_ANGLE_RANGE){
        snprintf(text_angle, 6, "NaNs");
    }
    else{
        snprintf(text_angle, 5, "%4d",(int)(angle));
    }

    if(current == CURR_OUTOF_Y_RANGE || current == CURR_OUTOF_Angle_RANGE){
        snprintf(text_current, 6, "NaNs");
    }
    else{
        snprintf(text_current, 7, " %.1f",(float)(current));
    }

    // display values
    BSP_LCD_DisplayStringAt(160, TITLE_HIGHT+20,  (uint8_t *)text_x_distance,   LEFT_MODE);
    BSP_LCD_DisplayStringAt(160, TITLE_HIGHT+40,  (uint8_t *)text_y_distance,   LEFT_MODE);

    BSP_LCD_DisplayStringAt(160, TITLE_HIGHT+80,  (uint8_t *)text_abs_distance, LEFT_MODE);
    BSP_LCD_DisplayStringAt(160, TITLE_HIGHT+100, (uint8_t *)text_angle,        LEFT_MODE);

    BSP_LCD_DisplayStringAt(160, TITLE_HIGHT+140, (uint8_t *)text_current,      LEFT_MODE);
}


/** ***************************************************************************
 * @brief Initialize visual Interface
 * @param [in] Title
 *
 * @note Call MENU_visual_act(int16_t x_distance, uint16_t y_distance,
 * float current) to show new data.
 *****************************************************************************/
void MENU_visual_init(uint8_t *title)
{
    MENU_clear();
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetBackColor(MENU_COLOR);

    BSP_LCD_SetTextColor(MENU_COLOR);
    BSP_LCD_FillRect(5, 5, BSP_LCD_GetXSize()-10, TITLE_HIGHT-10);

    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, TITLE_HIGHT/2 - 5, (uint8_t *)title, CENTER_MODE);

    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);

}


/** ***************************************************************************
 * @brief Display a visualized position to the cable
 * @param [in] Y-Distance [mm]
 * @param [in] X-Distance [mm]
 * @param [in] Current    [A]
 *
 * Shows the cable position to the device visually and
 * in mm, when the cable is in range of the device.
 * When the cable is in a certain range the current will be displayed.
 * @note Call MENU_visual_init(uint8_t *title) first
 *****************************************************************************/
void MENU_visual_act(int16_t x_distance, uint16_t y_distance, float current)
{
    char text_position[9];  // in mm
    char text_current[9];   // in A

    snprintf(text_current, 8, "NaNs A"); // default

    // erase old position
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DrawCircle(x_circle_old,y_circle_old+TITLE_HIGHT,10);
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,x_circle_old,y_circle_old+TITLE_HIGHT);

    // set static elements
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DrawCircle(120,TITLE_HIGHT+220,4);                  // origin
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,10,TITLE_HIGHT+110);   // -45°
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,120,TITLE_HIGHT+10);   // 0°
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,230,TITLE_HIGHT+110);  // +45°

    BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);                  // intermediate steps
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,10,TITLE_HIGHT+156);   // -60°
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,10,TITLE_HIGHT+30);    // -30°
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,66,TITLE_HIGHT+10);    // -15°
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,174,TITLE_HIGHT+10);   // +15°
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,230,TITLE_HIGHT+30);   // +30°
    BSP_LCD_DrawLine(120,TITLE_HIGHT+220,230,TITLE_HIGHT+156);  // +60°

    BSP_LCD_SetTextColor(LCD_COLOR_RED);

    // check if cable in range
    if ( x_distance != CALC_OUTOF_X_RANGE && y_distance != CALC_OUTOF_Y_RANGE ){

        // conversion for display
        uint16_t x_circle = (uint16_t)(120+x_distance);
        uint16_t y_circle = 220 - y_distance;

        // display position to device
        BSP_LCD_DrawCircle(x_circle,y_circle+TITLE_HIGHT,10);
        BSP_LCD_DrawLine(120,TITLE_HIGHT+220,x_circle,y_circle+TITLE_HIGHT);

        // calculate distance to device
        snprintf(text_position, 8, "%4d mm",(int)(hypot(x_distance, y_distance)));

        // check if current measurement possible
        if(current != CURR_OUTOF_Y_RANGE && current !=  CURR_OUTOF_Angle_RANGE){
            snprintf(text_current, 9, " %.1f A",(float)(current));
        }

        // set new position for erase
        x_circle_old = x_circle;
        y_circle_old = y_circle;
    }
    else{
        snprintf(text_position, 9, "NaNs mm"); // when no cable detected
    }

    // display distance to device
    BSP_LCD_DisplayStringAt(150, TITLE_HIGHT+215, (uint8_t *)text_position,LEFT_MODE);
    BSP_LCD_DisplayStringAt(20, TITLE_HIGHT+215, (uint8_t *)text_current,LEFT_MODE);
}


/** ***************************************************************************
 * @brief Draw the menu onto the display.
 *
 * Each menu entry has two lines.
 * Text and background colors are applied.
 * @n These attributes are defined in the variable MENU_draw[].
 *****************************************************************************/
void MENU_draw(void)
{
    BSP_LCD_SetFont(MENU_FONT);
    uint32_t x, y, m, w, h;
    y = MENU_Y;
    m = MENU_MARGIN;
    w = BSP_LCD_GetXSize()/MENU_ENTRY_COUNT;
    h = MENU_HEIGHT;
    for (uint32_t i = 0; i < MENU_ENTRY_COUNT; i++) {
        x = i*w;
        BSP_LCD_SetTextColor(MENU_entry[i].back_color);
        BSP_LCD_FillRect(x+m, y+m, w-2*m, h-2*m);
        BSP_LCD_SetBackColor(MENU_entry[i].back_color);
        BSP_LCD_SetTextColor(MENU_entry[i].text_color);
        BSP_LCD_DisplayStringAt(x+3*m, y+3*m, (uint8_t *)MENU_entry[i].line1, LEFT_MODE);
        BSP_LCD_DisplayStringAt(x+3*m, y+h/2, (uint8_t *)MENU_entry[i].line2, LEFT_MODE);
    }
}


/** ***************************************************************************
 * @brief Shows a hint at startup.
 *
 *****************************************************************************/
void MENU_hint(void)
{
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

    BSP_LCD_SetFont(&Font24);
    BSP_LCD_DisplayStringAt(0,10, (uint8_t *)"Cable Monitor", CENTER_MODE);
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_DisplayStringAt(0,35, (uint8_t *)"by M. Rau & T. Roos", CENTER_MODE);

    BSP_LCD_DisplayStringAt(10, 70, (uint8_t *)"Press black pushbutton to", LEFT_MODE);
    BSP_LCD_DisplayStringAt(10, 85, (uint8_t *)"-> reset system",           LEFT_MODE);

    BSP_LCD_DisplayStringAt(10, 120, (uint8_t *)"Press blue pushbutton to", LEFT_MODE);
    BSP_LCD_DisplayStringAt(10, 135, (uint8_t *)"-> Turn buzzer on/off",    LEFT_MODE);

    BSP_LCD_DisplayStringAt(10, 170, (uint8_t *)"Tap on the screen to",      LEFT_MODE);
    BSP_LCD_DisplayStringAt(10, 185, (uint8_t *)"-> change visual feedback", LEFT_MODE);

    BSP_LCD_DisplayStringAt(10, 220, (uint8_t *)"To start measurement press on", LEFT_MODE);
    BSP_LCD_DisplayStringAt(10, 235, (uint8_t *)"-> \"Average Measurement\" or", LEFT_MODE);
    BSP_LCD_DisplayStringAt(10, 250, (uint8_t *)"-> \"Single Measurement\"",     LEFT_MODE);
}


/** ***************************************************************************
 * @brief Shows when no function is available
 *
 *****************************************************************************/
void MENU_empty(void)
{
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);

    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(0,120, (uint8_t *)"EMPTY", CENTER_MODE);
}


/** ***************************************************************************
 * @brief Clear screen
 *
 * Clears data area. The menu bar and title will stay.
 *****************************************************************************/
void MENU_clear(void)
{
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_FillRect(0,0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize()-MENU_HEIGHT);
}

/** ***************************************************************************
 * @brief Set a menu entry.
 * @param [in] item number of menu bar
 * @param [in] entry attributes for that item
 *
 * @note Call MENU_draw() to update the display.
 *****************************************************************************/
void MENU_set_entry(const MENU_item_t item, const MENU_entry_t entry)
{
    if ((0 <= item) && (MENU_ENTRY_COUNT > item)) {
        MENU_entry[item] = entry;
    }
}


/** ***************************************************************************
 * @brief Get a menu entry.
 * @param [in] item number of menu bar
 * @return Menu_entry[item] or Menu_entry[0] if item not in range
 *****************************************************************************/
MENU_entry_t MENU_get_entry(const MENU_item_t item)
{
    MENU_entry_t entry = MENU_entry[0];
    if ((0 <= item) && (MENU_ENTRY_COUNT > item)) {
        entry = MENU_entry[item];
    }
    return entry;
}


/** ***************************************************************************
 * @brief Get menu selection/transition
 *
 * @return the selected MENU_item or MENU_NONE if no MENU_item was selected
 *
 * MENU_transition is used as a flag.
 * When the value is read by calling MENU_get_transition()
 * this flag is cleared, respectively set to MENU_NONE.
 *****************************************************************************/
MENU_item_t MENU_get_transition(void)
{
    MENU_item_t item = MENU_transition;
    MENU_transition = MENU_NONE;
    return item;
}


/** ***************************************************************************
 * @brief Check for selection/transition
 *
 * If the last transition has been consumed (MENU_NONE == MENU_transition)
 * and the touchscreen has been touched for a defined period
 * the variable MENU_transition is set to the touched item.
 * @note  Evalboard revision E (blue PCB) has an inverted y-axis
 * in the touch controller compared to the display.
 * Uncomment or comment the <b>\#define EVAL_REV_E</b> in main.h accordingly.
 *****************************************************************************/
void MENU_check_transition(void)
{
    static MENU_item_t item_old = MENU_NONE;
    static MENU_item_t item_new = MENU_NONE;
    static TS_StateTypeDef  TS_State;    // State of the touch controller
    BSP_TS_GetState(&TS_State);            // Get the state


// Evalboard revision E (blue) has an inverted y-axis in the touch controller
#ifdef EVAL_REV_E
    TS_State.Y = BSP_LCD_GetYSize() - TS_State.Y;    // Invert the y-axis
#endif
    // Invert x- and y-axis if LCD ist flipped
#ifdef FLIPPED_LCD
    TS_State.X = BSP_LCD_GetXSize() - TS_State.X;    // Invert the x-axis
    TS_State.Y = BSP_LCD_GetYSize() - TS_State.Y;    // Invert the y-axis
#endif


/*
    #if (defined(EVAL_REV_E) && !defined(FLIPPED_LCD)) || (!defined(EVAL_REV_E) && defined(FLIPPED_LCD))
    TS_State.Y = BSP_LCD_GetYSize() - TS_State.Y;    // Invert the y-axis
#endif
#ifdef EVAL_REV_E
#endif
*/
    if (TS_State.TouchDetected) {        // If a touch was detected
        /* Do only if last transition not pending anymore */
        if (MENU_NONE == MENU_transition) {
            item_old = item_new;        // Store old item
            /* If touched within the menu bar? */
            if ((MENU_Y < TS_State.Y) && (MENU_Y+MENU_HEIGHT > TS_State.Y)) {
                item_new = TS_State.X    // Calculate new item
                        / (BSP_LCD_GetXSize()/MENU_ENTRY_COUNT);
                if ((0 > item_new) || (MENU_ENTRY_COUNT <= item_new)) {
                    item_new = MENU_NONE;    // Out of bounds
                }
                if (item_new == item_old) {    // 2 times the same menu item
                    item_new = MENU_NONE;
                    MENU_transition = item_old;
                }
            } else if((0 < TS_State.Y) && (TITLE_HIGHT > TS_State.Y)){
                item_new = MENU_CABLE;
                if (item_new == item_old) {    // 2 times the same menu item
                    item_new = MENU_NONE;
                    MENU_transition = item_old;
                }
            }
            else{
                item_new = MENU_SUBTASK;
                if (item_new == item_old) {    // 2 times the same menu item
                    item_new = MENU_NONE;
                    MENU_transition = item_old;
                }
            }
        }
    }
}



/** ***************************************************************************
 * @brief Interrupt handler for the touchscreen
 *
 * @note BSP_TS_ITConfig(); must be called in the main function
 * to enable touchscreen interrupt.
 * @note There are timing issues when interrupt is enabled.
 * It seems that polling is the better choice with this evaluation board.
 * @n Call MENU_check_transition() from the while loop in main for polling.
 *
 * The touchscreen interrupt is connected to PA15.
 * @n The interrupt handler for external line 15 to 10 is called.
 *****************************************************************************/
void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR15) {        // Check if interrupt on touchscreen
        EXTI->PR |= EXTI_PR_PR15;        // Clear pending interrupt on line 15
        if (BSP_TS_ITGetStatus()) {        // Get interrupt status
            BSP_TS_ITClear();                // Clear touchscreen controller int.
            MENU_check_transition();
        }
        EXTI->PR |= EXTI_PR_PR15;        // Clear pending interrupt on line 15
    }
}
