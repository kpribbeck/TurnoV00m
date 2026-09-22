
#include <string.h>

#include "doomtype.h"
#include "d_event.h"
#include "m_controls.h"
#include "i_touch.h"
#include "i_touch_zone.h"
#include "i_touch_tracker.h"
#include "i_touch_digital.h"


// ---------------------------------------------------------------------------
// Zone -> Doom key mapping
//
// Each entry pairs a button zone with the ADDRESS of the key-binding variable
// it maps to.  We store a pointer (int *), not the value, because the binding
// variables are set at config-load time and can change via the setup tool —
// dereferencing at post time always uses the current binding.
//
// Only TINPUT_BUTTON zones appear here.  TZONE_MOVE and TZONE_TURN are analog
// and are handled elsewhere, so they are deliberately absent.
// ---------------------------------------------------------------------------

typedef struct
{
    touch_zone_id_t zone_id;
    int             *key_var; // -> key_fire, key_use, etc
} digital_binding_t;

static const digital_binding_t g_bindings[] =
{
    { TZONE_FIRE,           &key_fire       },
    { TZONE_USE,            &key_use        },
    { TZONE_WEAPON_PREV,    &key_prevweapon },
    { TZONE_WEAPON_NEXT,    &key_nextweapon },
    { TZONE_MAP,            &key_map_toggle },
    { TZONE_PAUSE,          &key_pause      },
};

#define NUM_BINDINGS (int)(sizeof(g_bindings) / sizeof(g_bindings[0]))


// ---------------------------------------------------------------------------
// Edge-detection state
//
// prev_pressed[i] is whether g_bindings[i]'s zone was pressed on the previous
// tic.  Comparing it against the current tracker state gives us the rising
// and falling edges that Doom's event model expects.
// ---------------------------------------------------------------------------

static boolean prev_pressed[NUM_BINDINGS];


// Internal helpers

static void PostKey(evtype_t type, int key)
{
    event_t event;

    // data1 is the Doom key code G_Responder indexes gamekeydown[] with.
    // For ev_keydown, vanilla sets data2 = the same key and data3 = typed char.
    // A Synthetic control press has no typed char, so data3 stays 0.
    // For ev_keyup, data2/data3 are 0 (release is detected from data1).
    event.type = type;
    event.data1 = key;
    event.data2 = (type == ev_keydown) ? key : 0;
    event.data3 = 0;

    D_PostEvent(&event);
}

/////////////////////////////////////////////////////////////////////////////
//                                 API
/////////////////////////////////////////////////////////////////////////////

void I_TouchDigitalInit(void)
{
    memset(prev_pressed, 0, sizeof(prev_pressed));
}

void I_UpdateTouchDigital(void)
{
    for (int i = 0; i < NUM_BINDINGS; i++)
    {
        boolean now = I_TouchTrackerZoneIsPressed(g_bindings[i].zone_id);

        if (now && !prev_pressed[i])
        {
            PostKey(ev_keydown, *g_bindings[i].key_var);
        }
        else if (!now && prev_pressed[i])
        {
            PostKey(ev_keyup, *g_bindings[i].key_var);
        }

        prev_pressed[i] = now;
    }
}
