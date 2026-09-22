
#include "i_system.h"
#include "m_config.h"
#include "i_touch_tracker.h"
#include "i_touch_zone.h"
#include "i_touch.h"

int usetouch = 1;

void I_BindTouchVariables(void)
{
    M_BindIntVariable("use_touch", &usetouch);
}

void I_InitTouch(void)
{
    I_TouchValidateLayout();
    I_TouchTrackerInit();
    I_TouchDigitalInit();
    I_AtExit(I_TouchTrackerShutdown, true);
    // TODO: Overlay render init
    // config variable binding
}

void I_HandleTouchEvent(const SDL_Event* ev)
{
    switch(ev->type)
    {
        case SDL_FINGERDOWN:
            I_TouchTrackerFingerDown(&sdlevent.tfinger);
            break;
        case SDL_FINGERMOTION:
            I_TouchTrackerFingerMotion(&sdlevent.tfinger);
            break;
        case SDL_FINGERUP:
            I_TouchTrackerFingerUp(&sdlevent.tfinger);
            break;
        default:
            break;
    }
}
