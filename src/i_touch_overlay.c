
#include <stddef.h> // NULL

#include "SDL.h"

#include "doomtype.h"
#include "m_config.h"
#include "i_video.h"
#include "i_touch.h"
#include "i_touch_zone.h"
#include "i_touch_tracker.h"
#include "i_touch_overlay.h"

static int touch_overlay_alpha = 200;

#define ALPHA_PRESSED   150 // body alpha when a zone is held
#define ALPHA_BORDER    180 // zone outline alpha
#define ALPHA_STICK     200 // analog stick dot alpha

#define UI_R    230
#define UI_G    230
#define UI_B    230

// Convert a zone's normalized rect to pixels for the current output size.
static SDL_Rect ZoneRect(const touch_zone_t *zone, int width, int height)
{
    SDL_Rect rect;
    rect.x = (int)(zone->x * width);
    rect.y = (int)(zone->y * height);
    rect.w = (int)(zone->width * width);
    rect.h = (int)(zone->height * height);
    return rect;
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

    I_TouchTrackerGetDeflection(zone->id, &dx, &dy);

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

    SDL_SetRenderDrawColor(renderer, UI_R, UI_G, UI_B, ALPHA_STICK);
    FillDot(renderer, cx + off_x, cy + off_y, half);
}

static void DrawZone(SDL_Renderer *renderer, const touch_zone_t *zone, int width, int height)
{
    SDL_Rect rect = ZoneRect(zone, width, height);
    boolean pressed = I_TouchTrackerZoneIsPressed(zone->id);
    int body_alpha = pressed ? ALPHA_PRESSED : touch_overlay_alpha;

    // Translucent body
    SDL_SetRenderDrawColor(renderer, UI_R, UI_G, UI_B, body_alpha);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, UI_R, UI_G, UI_B, ALPHA_BORDER);
    SDL_RenderDrawRect(renderer, &rect);

    // Analog zones also show where the thumb is
    if (zone->input_type == TINPUT_ANALOG)
    {
        DrawStick(renderer, zone, &rect);
    }
}

//////////////////////////////////////
////              API
//////////////////////////////////////

void I_TouchOverlayInit(void)
{
    // Nothing to initialize for now
}

void I_TouchOverlayDraw(void)
{
    SDL_Renderer *renderer;
    const touch_zone_t *zones;
    touch_mode_t current_mode;
    int width, height, i;

    if (!usetouch) return;

    renderer = I_GetRenderer();
    if (renderer == NULL) return;


    // Logical output size in pixels. These are the sizes used by the renderer
    // and are scaled to real size afterwards.
    SDL_RenderGetLogicalSize(renderer, &width, &height);
    if (width == 0 || height == 0)
    {
        // Logical size is (0,0) when aspect_ratio_correct and integer_scaling are
        // both off (see i_video.c). SDL then draws in raw window pixels, so fall
        // back to the output size or every rect would compute to zero and vanish.
        if (SDL_GetRendererOutputSize(renderer, &width, &height) != 0)
            return;
    }
    
    // Alpha blending must be on for translucent fills.
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    current_mode = I_IsMenuActive() ? TMODE_MENU : TMODE_GAME;

    zones = I_TouchGetZones();
    for (i = 0; i < TZONE_COUNT; i++)
    {
        if (zones[i].mode != current_mode)
            continue;
        
        DrawZone(renderer, &zones[i], width, height);
    }
}

void I_TouchOverlayBindVariables(void)
{
    M_BindIntVariable("touch_overlay_alpha", &touch_overlay_alpha);
}
