
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
    { TZONE_FIRE,           &key_fire         },
    { TZONE_USE,            &key_use          },
    { TZONE_WEAPON_PREV,    &key_prevweapon   },
    { TZONE_WEAPON_NEXT,    &key_nextweapon   },
    { TZONE_MAP,            &key_map_toggle   },
    { TZONE_PAUSE,          &key_pause        },
};

static const digital_binding_t g_menu_bindings[] =
{
    { TZONE_MENU_UP,        &key_menu_up      },
    { TZONE_MENU_DOWN,      &key_menu_down    },
    { TZONE_MENU_CONFIRM,   &key_menu_forward },
    { TZONE_MENU_BACK,      &key_menu_back    },
};


#define NUM_BINDINGS (int)(sizeof(g_bindings) / sizeof(g_bindings[0]))
#define NUM_MENU_BINDINGS (int)(sizeof(g_menu_bindings) / sizeof(g_menu_bindings[0]))



// ---------------------------------------------------------------------------
// Edge-detection state
//
// prev_pressed[i] is whether g_bindings[i]'s zone was pressed on the previous
// tic.  Comparing it against the current tracker state gives us the rising
// and falling edges that Doom's event model expects.
// ---------------------------------------------------------------------------


static boolean prev_pressed[NUM_BINDINGS];
static boolean prev_pressed_menu[NUM_MENU_BINDINGS];
static boolean prev_menu_active;


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

static void I_OnMenuActiveChange(void)
{
    const digital_binding_t* bindings;
    int count;
    boolean* prev;

    // We want to clear bindings no longer used
    // If we go from Game -> Menu, then we need to clear Game bindings
    // If we go from Menu -> Game, then we need to clear Menu bindings

    // From Game to Menu
    if (I_IsMenuActive())
    {
        bindings = g_bindings;
        count = NUM_BINDINGS;
        prev = prev_pressed;        
    }
    else // From Menu to Game
    {
        bindings = g_menu_bindings;
        count = NUM_MENU_BINDINGS;
        prev = prev_pressed_menu;
    }

    // Release bindings
    for (int i = 0; i < count; i++)
    {
        if (prev[i])
        {
            PostKey(ev_keyup, *bindings[i].key_var);
        }
        prev[i] = false;
    }
}

void I_TouchDigitalInit(void)
{
    memset(prev_pressed, 0, sizeof(prev_pressed));
    memset(prev_pressed_menu, 0, sizeof(prev_pressed_menu));
    prev_menu_active = false;
}

void I_UpdateTouchDigital(void)
{
    const digital_binding_t* bindings;
    int count;
    boolean* prev;
    boolean menu_active = I_IsMenuActive();

    if (menu_active != prev_menu_active)
    {
        I_OnMenuActiveChange();
    }

    if (menu_active)
    {
        bindings = g_menu_bindings;
        count = NUM_MENU_BINDINGS;
        prev = prev_pressed_menu;
    }
    else
    {
        bindings = g_bindings;
        count = NUM_BINDINGS;
        prev = prev_pressed;
    }

    for (int i = 0; i < count; i++)
    {
        boolean now = I_TouchTrackerZoneIsPressed(bindings[i].zone_id);

        if (now && !prev[i])
        {
            PostKey(ev_keydown, *bindings[i].key_var);
        }
        else if (!now && prev[i])
        {
            PostKey(ev_keyup, *bindings[i].key_var);
        }

        prev[i] = now;
    }
    prev_menu_active = menu_active;
}
