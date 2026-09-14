
// DESCRIPTION:
//   Touch zone layout implementation.
//
//   The layout divides the screen into non-overlapping rectangles using
//   normalized coordinates [0.0, 1.0].  Adjust the constants in the
//   "Layout constants" section to change zone sizes or positions.
//   I_TouchValidateLayout() will catch any resulting overlap or
//   out-of-bounds error at startup.
//

#include "i_touch_zone.h"
#include "i_system.h"   // I_Error


// ---------------------------------------------------------------------------
// Layout constants
//
// The screen is partitioned into three horizontal bands and two vertical
// columns on the right side.  Change these constants to tune the layout.
//
//   Landscape screen (example: 1920 × 1080):
//
//   x=0.00           x=0.35      x=0.70  x=0.72  x=0.86 x=1.00
//   |                |           |       |       |      |
//   |  WPN- (0.18)   WPN+ (0.18)   <gap>   MAP    PAUSE  |  y=0.00
//   |                                                      |  y=0.15
//   |                            |       USE              |
//   |    MOVE (analog)           |                        |  y=0.55
//   |                 TURN       |       FIRE             |
//   |                 (analog)   |                        |  y=1.00
//
// The gap in the top bar (x=0.36 to x=0.72, y=0.00 to 0.15) is
// intentional — it is unreachable by either thumb and avoids accidental
// presses while staring at the top HUD area.
//
// ---------------------------------------------------------------------------

// Height of the top button bar (weapon prev/next, map, pause).
#define TZ_TOP_H       0.15f

// Right edge of the MOVE zone and left edge of the TURN zone.
// MOVE spans [0.00, TZ_MOVE_W); TURN spans [TZ_MOVE_W, TZ_TURN_R).
#define TZ_MOVE_W      0.35f
#define TZ_TURN_R      0.70f   // right edge of TURN; left edge of action column

// Y coordinate that splits the right action column into USE (above) and
// FIRE (below).
#define TZ_ACTION_Y    0.55f

// Top-bar button widths.  Buttons start at x=0 (left) and x=TZ_MAP_L (right).
#define TZ_WPREV_W     0.18f
#define TZ_WNEXT_W     0.18f
#define TZ_MAP_L       0.72f   // left edge of map button
#define TZ_PAUSE_L     0.86f   // left edge of pause button


// ---------------------------------------------------------------------------
// Zone table
//
// Indexed directly by touch_zone_id_t (g_zones[TZONE_FIRE] == the FIRE zone).
// The [TZONE_X] = { ... } designated-initialiser syntax makes this mapping
// explicit and will produce a compiler warning if an initialiser is omitted.
//
// Do not reorder entries here — use the designated-initialiser syntax so
// the enum and array always stay in sync.
// ---------------------------------------------------------------------------

static const touch_zone_t g_zones[TZONE_COUNT] =
{
    // ---- Analog zones ----

    [TZONE_MOVE] =
    {
        .id         = TZONE_MOVE,
        .input_type = TINPUT_ANALOG,
        .x          = 0.00f,
        .y          = TZ_TOP_H,
        .width          = TZ_MOVE_W,
        .height          = 1.00f - TZ_TOP_H,
        .label      = "MOVE"
    },

    [TZONE_TURN] =
    {
        .id         = TZONE_TURN,
        .input_type = TINPUT_ANALOG,
        .x          = TZ_MOVE_W,
        .y          = TZ_TOP_H,
        .width          = TZ_TURN_R - TZ_MOVE_W,
        .height          = 1.00f - TZ_TOP_H,
        .label      = "TURN"
    },

    // ---- Button zones ----

    [TZONE_FIRE] =
    {
        .id         = TZONE_FIRE,
        .input_type = TINPUT_BUTTON,
        .x          = TZ_TURN_R,
        .y          = TZ_ACTION_Y,
        .width          = 1.00f - TZ_TURN_R,
        .height          = 1.00f - TZ_ACTION_Y,
        .label      = "FIRE"
    },

    [TZONE_USE] =
    {
        .id         = TZONE_USE,
        .input_type = TINPUT_BUTTON,
        .x          = TZ_TURN_R,
        .y          = TZ_TOP_H,
        .width          = 1.00f - TZ_TURN_R,
        .height          = TZ_ACTION_Y - TZ_TOP_H,
        .label      = "USE"
    },

    [TZONE_WEAPON_PREV] =
    {
        .id         = TZONE_WEAPON_PREV,
        .input_type = TINPUT_BUTTON,
        .x          = 0.00f,
        .y          = 0.00f,
        .width          = TZ_WPREV_W,
        .height          = TZ_TOP_H,
        .label      = "WPN-"
    },

    [TZONE_WEAPON_NEXT] =
    {
        .id         = TZONE_WEAPON_NEXT,
        .input_type = TINPUT_BUTTON,
        .x          = TZ_WPREV_W,
        .y          = 0.00f,
        .width          = TZ_WNEXT_W,
        .height          = TZ_TOP_H,
        .label      = "WPN+"
    },

    [TZONE_MAP] =
    {
        .id         = TZONE_MAP,
        .input_type = TINPUT_BUTTON,
        .x          = TZ_MAP_L,
        .y          = 0.00f,
        .width          = TZ_PAUSE_L - TZ_MAP_L,
        .height          = TZ_TOP_H,
        .label      = "MAP"
    },

    [TZONE_PAUSE] =
    {
        .id         = TZONE_PAUSE,
        .input_type = TINPUT_BUTTON,
        .x          = TZ_PAUSE_L,
        .y          = 0.00f,
        .width          = 1.00f - TZ_PAUSE_L,
        .height          = TZ_TOP_H,
        .label      = "PAUSE"
    },
};


// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

touch_zone_id_t I_TouchHitTest(float x, float y)
{
    for (int i = 0; i < TZONE_COUNT; i++)
    {
        const touch_zone_t *zone = &g_zones[i];

        // Half-open interval: [zone->x, zone->x + zone->width) × [zone->y, zone->y + zone->height).
        // A point on the shared edge between two adjacent zones falls into
        // the zone whose range starts there, matching the non-overlap
        // guarantee checked by I_TouchValidateLayout().
        if (x >= zone->x && x < zone->x + zone->width &&
            y >= zone->y && y < zone->y + zone->height)
        {
            return zone->id;
        }
    }

    return TZONE_NONE;
}

const touch_zone_t *I_TouchGetZones(void)
{
    return g_zones;
}

const touch_zone_t *I_TouchGetZone(touch_zone_id_t id)
{
    if (id == TZONE_NONE || id < 0 || id >= TZONE_COUNT)
        return NULL;

    return &g_zones[id];
}

void I_TouchValidateLayout(void)
{
    // Small epsilon for floating-point edge cases in bounds checks.
    // Zones that sum to exactly 1.0 in theory may differ by a ULP in
    // practice; anything beyond this epsilon is a genuine mistake.
    const float EPSILON = 0.001f;

    // ---- Per-zone checks ----

    for (int i = 0; i < TZONE_COUNT; i++)
    {
        const touch_zone_t *z = &g_zones[i];

        // The id-equals-index invariant is what makes I_TouchGetZone O(1).
        if ((int)z->id != i)
            I_Error("I_TouchValidateLayout: g_zones[%d].id is %d "
                    "(expected %d) — fix the zone table or the enum",
                    i, (int)z->id, i);

        // Origin must be inside the screen.
        if (z->x < 0.0f || z->y < 0.0f)
            I_Error("I_TouchValidateLayout: zone '%s' has negative origin "
                    "(%.3f, %.3f)", z->label, z->x, z->y);

        // Size must be positive.
        if (z->width <= 0.0f || z->height <= 0.0f)
            I_Error("I_TouchValidateLayout: zone '%s' has zero or negative "
                    "size (%.3f × %.3f)", z->label, z->width, z->height);

        // Right and bottom edges must not exceed 1.0 (within epsilon).
        if (z->x + z->width > 1.0f + EPSILON)
            I_Error("I_TouchValidateLayout: zone '%s' right edge %.3f "
                    "exceeds 1.0", z->label, z->x + z->width);

        if (z->y + z->height > 1.0f + EPSILON)
            I_Error("I_TouchValidateLayout: zone '%s' bottom edge %.3f "
                    "exceeds 1.0", z->label, z->y + z->height);
    }

    // ---- Pairwise overlap check ----
    //
    // Two axis-aligned rectangles A and B overlap when both axes overlap.
    // Shared edges (A.right == B.left) do NOT count as overlap because
    // I_TouchHitTest uses the half-open interval [x, x+width), so a point
    // on a shared edge belongs to exactly one zone.
    //
    // Using strict < in the overlap test matches the half-open semantics:
    //   overlap iff (A.x < B.x+B.width) && (B.x < A.x+A.width)
    //               && (A.y < B.y+B.height) && (B.y < A.y+A.height)

    for (int i = 0; i < TZONE_COUNT - 1; i++)
    {
        for (int j = i + 1; j < TZONE_COUNT; j++)
        {
            const touch_zone_t *a = &g_zones[i];
            const touch_zone_t *b = &g_zones[j];

            int x_overlap = (a->x < b->x + b->width) && (b->x < a->x + a->width);
            int y_overlap = (a->y < b->y + b->height) && (b->y < a->y + a->height);

            if (x_overlap && y_overlap)
                I_Error("I_TouchValidateLayout: zones '%s' and '%s' overlap — "
                        "adjust the layout constants in i_touch_zone.c",
                        a->label, b->label);
        }
    }
}
