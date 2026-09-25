
#include <stddef.h> // NULL

#include "SDL.h"

#include "doomtype.h"
#include "m_config.h"
#include "i_video.h"
#include "i_touch.h"
#include "i_touch_zone.h"
#include "i_touch_tracker.h"
#include "i_touch_overlay.h"

static int touch_overlay_alpha = 60;

#define ALPHA_PRESSED   150 // body alpha when a zone is held
#define ALPHA_BORDER    180 // zone outline alpha
#define ALPHA_STICK     200 // analog stick dot alpha

#define UI_R    230
#define UI_G    230
#define UI_B    230

// Convert a zone's normalized rect to pixels for the current output size.
static SLD_Rect ZoneRect(const touch_zone_t *zone, int width, int height)
{
    SDL_Rect rect;
    rect.x = (int)(zone->x * width);
    rect.y = (int)(zone->y * height);
    rect.w = (int)(zone->width * width);
    rect.h = (int)(zone->height * height);
    return r;
}

static void FillDot(SDL_Renderer *renderer, int cx, int cy, int half)
{
    SDL_Rect dot;
    dot.x = cx - half;
    dot.y = cy - half;
    dot.w = half * 2;
    dot.h = half * 2;
    SDL_RenderFillRect(renderer, &dot);
}

// Draw one analog zone's stick indicator. A dot offset from the zone center
// by the finger's current deflection, clamped to the zone so it can't escape.
static void DrawStick(SDL_Renderer *renderer, const touch_zone_t *zone, const SDL_Rect *rect)
{
    float dx, dy;
    int cx, cy, half, off_x, off_y, reach;

    I_TouchTrackerGetDeflection(z->id, &dx, &dy);

    cx = rect->x + rect->w / 2;
    cy = rect->y + rect->h / 2;

    // Deflection is normalized to the whole screen. Scale to this zone and
    // clamp the dot within a reach that keeps it inside the zone bounds
    reach = (rect->w < rect->h ? rect->w : rect->h) / 2;
    off_x = (int)(dx * rect->w);
    off_y = (int)(dy * rect->h);
    if (off_x >  reach) off_x =  reach;
    if (off_x < -reach) off_x = -reach;
    if (off_y >  reach) off_y =  reach;
    if (off_y < -reach) off_y = -reach;

    // Guard to prevent the dot from becoming too small
    half = reach / 4;
    if (half < 3) half = 3;

    SDL_SetRenderDrawColor(r, UI_R, UI_G, UI_B, STICK_ALPHA);
    FillDot(r, cx + off_x, cy + off_y, half);
}

static void DrawZone(SDL_Renderer *renderer, const touch_zone_t *zone, int width, int height)
{
    SDL_Rect rect = ZoneRect(zone, width, height);
    boolean pressed = I_TouchTrackerZoneIsPressed(zone->id);
    int body_alpha = pressed ? ALPHA_PRESSED : touch_overlay_alpha;

    // Translucent body
    SDL_SetRenderDrawColor(renderer, UI_R, UI_G, UI_B, body_alpha);
    SDL_RenderFillRect(renderer, &rect);

    // Analog zones also show where the thumb is
    if (zone->input_type == TINPUT_ANALOG)
    {
        DrawStick(renderer, zone, &rect);
    }
}

//////////////////////////////////////
////              API
//////////////////////////////////////

void I_InitTouchOverlay(void)
{
    // Nothing to initialize for now
}

void I_TouchOverlayDraw(void)
{
    SDL_Renderer *renderer;
}