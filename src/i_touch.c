
#include "i_system.h"
#include "m_config.h"
#include "i_touch_tracker.h"
#include "i_touch_zone.h"
#include "i_touch_digital.h"
#include "i_touch.h"

int usetouch = 1;
static touch_menuactive_callback_t menuactive_callback = NULL;

void I_BindTouchVariables(void)
{
    M_BindIntVariable("use_touch", &usetouch);
}

void I_SetTouchMenuActiveCallback(touch_menuactive_callback_t func)
{
    menuactive_callback = func;
}

boolean I_IsMenuActive(void)
{
    return menuactive_callback ? menuactive_callback() : false;
}

void I_InitTouch(void)
{
    // Disable simulated mouse events by touch input
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

    I_TouchValidateLayout();
    I_TouchTrackerInit();
    I_TouchDigitalInit();
    I_AtExit(I_TouchTrackerShutdown, true);
    // TODO: Overlay render init
    // config variable binding
}

void I_UpdateTouch(void)
{
    if (!usetouch) return;

    I_UpdateTouchDigital();
    // TODO: I_UpdateTouchAnalog();
}

void I_HandleTouchEvent(const SDL_Event* sdlevent)
{
    switch(sdlevent->type)
    {
        case SDL_FINGERDOWN:
            I_TouchTrackerFingerDown(&sdlevent->tfinger);
            break;
        case SDL_FINGERMOTION:
            I_TouchTrackerFingerMotion(&sdlevent->tfinger);
            break;
        case SDL_FINGERUP:
            I_TouchTrackerFingerUp(&sdlevent->tfinger);
            break;
        default:
            break;
    }
}
