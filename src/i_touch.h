
#ifndef I_TOUCH_H
#define I_TOUCH_H

#include "SDL.h"

extern int usetouch;

void I_BindTouchVariables(void);

void I_InitTouch(void);
void I_UpdateTouch(void);
void I_TouchShutdown(void);
void I_HandleTouchEvent(const SDL_Event* sdlevent);

boolean I_IsMenuActive(void);
typedef boolean (*touch_menuactive_callback_t)(void);
void I_SetTouchMenuActiveCallback(touch_menuactive_callback_t func);

#endif // I_TOUCH_H
