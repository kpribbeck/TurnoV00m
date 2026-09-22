
#ifndef I_TOUCH_FINGER_TRACKER
#define I_TOUCH_FINGER_TRACKER

#include "SDL.h"
#include "doomtype.h"
#include "i_touch_zone.h"


// Maximum simultaneous touches.  Ten covers every finger on both hands,
// which is more than any Doom control scheme needs.
#define MAX_FINGERS 10

// Data types
typedef struct
{
    boolean active;

    SDL_FingerID id;
    touch_zone_id_t zone_id;

    // Normalized [0.0, 1.0]
    // original X and Y positions on Finger Down
    float ox, oy;

    // current X and Y positions
    float x, y;
} touch_finger_t;

void I_TouchTrackerInit(void);
void I_TouchTrackerShutdown(void);

// Handlers
// typedef struct SDL_TouchFingerEvent
// {
//     Uint32 type;        /**< SDL_FINGERMOTION or SDL_FINGERDOWN or SDL_FINGERUP */
//     Uint32 timestamp;   /**< In milliseconds, populated using SDL_GetTicks() */
//     SDL_TouchID touchId; /**< The touch device id */
//     SDL_FingerID fingerId;
//     float x;            /**< Normalized in the range 0...1 */
//     float y;            /**< Normalized in the range 0...1 */
//     float dx;           /**< Normalized in the range -1...1 */
//     float dy;           /**< Normalized in the range -1...1 */
//     float pressure;     /**< Normalized in the range 0...1 */
//     Uint32 windowID;    /**< The window underneath the finger, if any */
// } SDL_TouchFingerEvent;
void I_TouchTrackerFingerDown(const SDL_TouchFingerEvent *sdlevent);
void I_TouchTrackerFingerMotion(const SDL_TouchFingerEvent *sdlevent);
void I_TouchTrackerFingerUp(const SDL_TouchFingerEvent *sdlevent);


// Query functions used by other modules

// Returns true if any active finger is currently pressing the given zone.
// Used by the Digital Handler to post ev_keydown/ev_keyup transitions,
// and by the Overlay Renderer to highlight pressed buttons.
boolean I_TouchTrackerZoneIsPressed(touch_zone_id_t zone);

// Writes the stick deflection for an ANALOG zone into *out_dx, *out_dy.
// Deflection is (current - origin), in normalized screen coordinates.
// Both are set to 0.0 if no finger is active in that zone.
// Used by the Analog Handler to compute ev_mouse deltas each tic.
void I_TouchTrackerGetDeflection(touch_zone_id_t zone, float* out_dx, float* out_dy);

// Returns a read-only pointer to the full finger table (MAX_FINGERS slots).
// Slots where active == false should be skipped.
// Used by the Overlay Renderer to iterate active touches.
const touch_finger_t* I_TouchTrackerGetFingers(void);

#endif // I_TOUCH_FINGER_TRACKER
