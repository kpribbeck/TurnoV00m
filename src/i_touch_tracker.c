
#include "SDL.h"
#include <string.h> // memset
#include "doomtype.h"
#include "i_system.h" // I_Error
#include "i_touch_zone.h"
#include "i_touch_tracker.h"


// -------------- State ---------------

static touch_finger_t tracked_fingers[MAX_FINGERS];\

// -------------- Helpers -------------

// Returns the slot index for the given SDL_FingerID, or -1 if not found.
static int FindSlotById(SDL_FingerID id)
{
    for (int i = 0; i <= MAX_FINGERS; i++)
    {
        if (tracked_fingers[i].active && tracked_fingers[i].id == id)
        {
            return i;
        }
    }
    return -1;
}

static int FindFreeSlot(void)
{
    for (int i = 0; i <= MAX_FINGERS; i++)
    {
        if (!tracked_fingers[i].active)
        {
            return i;
        }
    }
    return -1;
}

// -------------- API -------------------

void I_TouchTrackerInit(void)
{
    memset(tracked_fingers, 0, sizeof(tracked_fingers));
}

void I_TouchTrackerShutdown(void)
{
    // No memory has been allocated, so just 0 out the tracked fingers state
    memset(tracked_fingers, 0, sizeof(tracked_fingers));
}

void I_TouchTrackerFingerDown(const SDL_TouchFingerEvent *sdlevent)
{
    int index;
    touch_finger_t* finger;

    // Finger could already be down in a rare lost FingerUp event.
    // Reuse stale slot instead of leaking it.
    index = FindSlotById(sdlevent->fingerId);

    if (index == -1)
        index = FindFreeSlot();

    if (index == -1)
        // All slots occupied, ignore this input
        return;

    finger = &tracked_fingers[index];

    finger->active = true;
    finger->id = sdlevent->fingerId;
    finger->zone_id = I_TouchHitTest(sdlevent->x, sdlevent->y);
    finger->ox = sdlevent->x;
    finger->oy = sdlevent->y;
    finger->x = sdlevent->x;
    finger->y = sdlevent->y;
}

void I_TouchTrackerFingerMotion(const SDL_TouchFingerEvent *sdlevent)
{
    int index;
    touch_finger_t* finger;

    index = FindSlotById(sdlevent->fingerId);

    if (index == -1)
    {
        // Motion event from an untracked finger. Treat is as finger down
        // so we don't lose any input events
        I_TouchTrackerFingerDown(sdlevent);
        return;
    }

    finger = &tracked_fingers[index];

    finger->x = sdlevent->x;
    finger->y = sdlevent->y;

    // NOTE: We do NOT update f->zone here.
    //
    // Once a finger is assigned a zone on FINGERDOWN, it keeps that zone
    // for its entire lifetime.  This is intentional:
    //
    //   - For BUTTON zones (fire, use, etc.): the button stays pressed even
    //     if the finger wanders slightly outside the zone rectangle while
    //     the player holds it.  This matches real mobile game UX where a
    //     button shouldn't suddenly release because of small finger drift.
    //
    //   - For ANALOG zones (move, turn): the deflection is computed from
    //     (x - ox, y - oy), which already handles the finger moving anywhere
    //     on screen relative to where it started.
    //
    // If you later want a "drag into a new zone" gesture, this is the place
    // to reconsider and add a zone-change callback.
}

void I_TouchTrackerFingerUp(const SDL_TouchFingerEvent *sdlevent)
{
    int index = FindSlotById(sdlevent->fingerId);

    if (index == -1)
    {
        return;
    }

    // Zero the whole slot
    memset(&tracked_fingers[index], 0, sizeof(touch_finger_t));
}

boolean I_TouchTrackerZoneIsPressed(touch_zone_id_t zone)
{
    if (zone == TZONE_NONE)
        return false;

    for (int i = 0; i <= MAX_FINGERS; i++)
    {
        if(tracked_fingers->active && tracked_fingers->zone_id == zone)
            return true;
    }
    return false;
}

void I_TouchTrackerGetDeflection(touch_zone_id_t zone, float *out_dx, float *out_dy)
{
    *out_dx = 0;
    *out_dy = 0;

    if (zone == TZONE_NONE)
        return;

    for (int i = 0; i <= MAX_FINGERS; i++)
    {
        const touch_finger_t* finger = &tracked_fingers[i];

        if (finger->active && finger->zone_id == zone)
        {
            *out_dx = finger->x - finger->ox;
            *out_dy = finger->y - finger->oy;
            return;
        }
    }
}

const touch_finger_t* I_TouchTrackerGetFingers(void)
{
    return tracked_fingers;
}