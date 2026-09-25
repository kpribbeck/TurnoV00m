
#include <string.h>

#include "doomtype.h"
#include "d_event.h"
#include "m_config.h"
#include "m_controls.h"
#include "i_touch.h"
#include "i_touch_zone.h"
#include "i_touch_tracker.h"
#include "i_touch_analog.h"


// ---------------------------------------------------------------------------
// Tunables (config-bound; see I_TouchAnalogBindVariables)
//
// Deflections are in normalized screen units [0..1], so a threshold of 0.06
// is 6% of screen width/height from the finger-down origin.
// ---------------------------------------------------------------------------


static float touch_move_threshold = 0.06f;

static float touch_turn_deadzone = 0.02f;
static float touch_turn_sensitivity = 900.f;

// ---------------------------------------------------------------------------
// MOVE: virtual d-pad
//
// One binding per direction. dir_axis/dir_sign say which deflection axis and
// which side of zero engages this direction; held[] is the edge-detection
// state, exactly like the digital handler's prev_pressed[].
// ---------------------------------------------------------------------------

typedef enum
{
    TAXIS_X = 0,
    TAXIS_Y = 1,
} analog_axis_t;

typedef struct
{
    analog_axis_t   axis;
    int             sign;
    int             *key_var; // -> key_up, key_down, etc
} move_dir_t;

static const move_dir_t g_move_dirs[] =
{
    { TAXIS_X,           +1,    &key_straferight  },
    { TAXIS_X,           -1,    &key_strafeleft   },
    { TAXIS_Y,           +1,    &key_down         },
    { TAXIS_Y,           -1,    &key_up           },
};

#define NUM_MOVE_DIRS (int)(sizeof(g_move_dirs) / sizeof(g_move_dirs[0]))



static boolean held[NUM_MOVE_DIRS];


static void PostKey(evtype_t type, int key)
{
    event_t event;

    event.type = type;
    event.data1 = key;
    event.data2 = (type == ev_keydown) ? key : 0;
    event.data3 = 0;

    D_PostEvent(&event);
}

static void PostMouseTurn(int turn_delta)
{
    event_t event;

    event.type = ev_mouse;
    event.data1 = 0;
    event.data2 = turn_delta; // X axis -> turn
    event.data3 = 0;

    D_PostEvent(&event);
}

void I_TouchAnalogInit(void)
{
    memset(held, 0, sizeof(held));
}

static void UpdateMove(void)
{
    float dx, dy;
    float deflection[2];

    I_TouchTrackerGetDeflection(TZONE_MOVE, &dx, &dy);
    deflection[0] = dx;
    deflection[1] = dy;

    for (int i = 0; i < NUM_MOVE_DIRS; i++)
    {
        const move_dir_t *d = &g_move_dirs[i];

        float value = deflection[d->axis] * (float)d->sign;
        boolean now = (value > touch_move_threshold);

        if (now && !held[i])
        {
            PostKey(ev_keydown, *d->key_var);
        }
        else if (!now && held[i])
        {
            PostKey(ev_keyup, *d->key_var);
        }

        held[i] = now;
    }
}

static void UpdateTurn(void)
{
    float dx, dy;
    int turn;

    I_TouchTrackerGetDeflection(TZONE_TURN, &dx, &dy);
    (void)dy; // TURN is horizontal only

    if (dx > touch_turn_deadzone || dx < -touch_turn_deadzone)
    {
        turn = (int)(dx * touch_turn_sensitivity);
        if (turn != 0)
        {
            PostMouseTurn(turn);
        }
    }
}

void I_TouchAnalogReleaseAll(void)
{
    // Release any held movement key so it can't stick while we stop polling.
    for (int i = 0; i < NUM_MOVE_DIRS; i++)
    {
        if (held[i])
        {
            PostKey(ev_keyup, *g_move_dirs[i].key_var);
        }
        held[i] = false;
    }
 
    // TURN posts no persistent state (mouse deltas are per-tic), so simply
    // not calling UpdateTurn stops it — nothing to release.
}

void I_UpdateTouchAnalog(void)
{
    UpdateMove();
    UpdateTurn();
}

void I_TouchAnalogBindVariables(void)
{
    M_BindFloatVariable("touch_move_threshold",     &touch_move_threshold);
    M_BindFloatVariable("touch_turn_deadzone",      &touch_turn_deadzone);
    M_BindFloatVariable("touch_turn_sensitivity",   &touch_turn_sensitivity);
}
