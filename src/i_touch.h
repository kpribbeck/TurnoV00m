
#ifndef I_TOUCH_H
#define I_TOUCH_H

#include "SDL.h"

extern int usetouch;

void I_BindTouchVariables(void);

void I_InitTouch(void);
void I_TouchShutdown(void);
void I_HandleTouchEvent(const SDL_Event* ev);

#endif // I_TOUCH_H
