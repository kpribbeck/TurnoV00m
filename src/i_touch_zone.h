// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// DESCRIPTION:
//   Touch zone layout — defines the non-overlapping interactive regions
//   that make up the on-screen gamepad.
//
//   Coordinates are normalized to [0.0, 1.0] so the layout scales
//   automatically to any screen resolution.  SDL2's finger events
//   already arrive in this range, so no pixel conversion is needed.
//
//   Two zone types exist:
//
//     TINPUT_BUTTON — A discrete tap region.  Finger-down posts ev_keydown;
//                     finger-up posts ev_keyup.  Used for Fire, Use, etc.
//
//     TINPUT_ANALOG — A continuous drag region.  The deflection from the
//                     touch origin is posted as ev_mouse deltas each tic,
//                     even when the finger is not moving.  Used for Move
//                     (forward/back/strafe) and Turn (horizontal rotation).
//

#ifndef I_TOUCH_ZONE_H
#define I_TOUCH_ZONE_H


// ---------------------------------------------------------------------------
// Zone identity
//
// Each value is both the logical name of a zone and its index into the
// internal zone table.  This invariant (id == table index) is asserted by
// I_TouchValidateLayout() at startup and allows O(1) lookup via
// I_TouchGetZone(id).
//
// TZONE_NONE (-1) is a sentinel meaning "no zone was touched" and is
// never a valid table index.  Callers of I_TouchHitTest() must check
// for it before using the return value.
// ---------------------------------------------------------------------------

typedef enum
{
    TZONE_NONE        = -1,    // sentinel — "no zone hit"

    // Analog zones (continuous input)
    TZONE_MOVE        =  0,    // left thumb: forward / backward / strafe
    TZONE_TURN        =  1,    // right thumb: horizontal turn + vertical look

    // Button zones (discrete press / release)
    TZONE_FIRE        =  2,    // primary attack
    TZONE_USE         =  3,    // open door / activate switch
    TZONE_WEAPON_PREV =  4,    // cycle to previous weapon
    TZONE_WEAPON_NEXT =  5,    // cycle to next weapon
    TZONE_MAP         =  6,    // toggle automap
    TZONE_PAUSE       =  7,    // pause / open menu

    TZONE_COUNT                // must remain last — equals the table length

} touch_zone_id_t;


// ---------------------------------------------------------------------------
// Input interpretation
// ---------------------------------------------------------------------------

typedef enum
{
    TINPUT_BUTTON,    // tap   → ev_keydown / ev_keyup
    TINPUT_ANALOG     // drag  → ev_mouse deltas posted each tic

} touch_input_type_t;


// ---------------------------------------------------------------------------
// Zone descriptor
//
// Describes a single interactive region of the screen.
// ---------------------------------------------------------------------------

typedef struct
{
    touch_zone_id_t    id;          // must match this zone's index in the table
    touch_input_type_t input_type;

    // Normalized screen-space rectangle, origin at top-left corner.
    // All values in [0.0, 1.0].  Half-open interval: [x, x+w) × [y, y+h).
    // No two zones overlap.
    float x;                        // left edge
    float y;                        // top edge
    float width;                        // width
    float height;                        // height

    const char *label;              // short label drawn by the overlay renderer

} touch_zone_t;


// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Returns the id of the zone containing the point (x, y) in normalized
// screen coordinates, or TZONE_NONE if the point falls outside every zone.
//
// x and y are expected to be SDL2 finger coordinates (already normalized).
// Because zones are guaranteed non-overlapping, at most one zone can match.
touch_zone_id_t     I_TouchHitTest(float x, float y);

// Returns a read-only pointer to the full zone table (TZONE_COUNT entries).
// The overlay renderer uses this to iterate all zones for drawing.
const touch_zone_t *I_TouchGetZones(void);

// Returns a read-only pointer to zone `id`, or NULL for TZONE_NONE or any
// out-of-range value.  Convenience accessor for the finger tracker.
const touch_zone_t *I_TouchGetZone(touch_zone_id_t id);

// Validates the layout at startup.  Checks that every zone is within
// [0.0, 1.0] bounds, that the id-equals-index invariant holds, and that
// no two zones overlap.  Calls I_Error() on any failure.
//
// Call this once from I_TouchInit(), before any touch events are processed.
void                I_TouchValidateLayout(void);


#endif // I_TOUCH_ZONE_H
