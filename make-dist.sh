#!/usr/bin/env bash
#
# make-dist.sh — assemble a self-contained, portable folder for the totem.
#
# Produces  dist/turnovoom/  containing:
#   - the game exe
#   - EVERY DLL it needs (resolved recursively from the UCRT64 runtime)
#   - the IWAD
#   - the touch/ PNG assets (if present)
#   - a config file (if present)
#
# Copy that one folder to the totem and run the exe. No installer, no MSYS2.
#
# Run from the MSYS2 UCRT64 shell, at the repo root:
#     ./make-dist.sh
#
# Requires ntldd (pacman -S mingw-w64-ucrt-x86_64-ntldd) to find the DLLs.

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration — adjust these paths to match your layout.
# ---------------------------------------------------------------------------

# Name of the built executable (change if you renamed it via OUTPUT_NAME).
EXE_NAME="TurnoV00m.exe"

# Where the build put the exe.
BUILD_DIR="build/src"

# Your IWAD. Adjust to wherever DOOM.WAD actually lives.
IWAD="../V00mAssets/DOOM.WAD"

# Optional extras — copied only if they exist. Leave as-is; the script skips
# any that are missing.
TOUCH_ASSETS_DIR="assets/touch"        # PNG button art, if you use the SDL_image route
CONFIG_FILE="dist-assets/crispy-doom.cfg"  # a tuned kiosk config to ship

# Output location.
DIST_ROOT="dist"
DIST_NAME="turnovoom"

# UCRT64 bin dir (where MSYS2 keeps the runtime DLLs).
UCRT_BIN="/ucrt64/bin"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

info()  { printf '  %s\n' "$*"; }
warn()  { printf 'WARNING: %s\n' "$*" >&2; }
die()   { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------------------
# Preflight checks — fail early and clearly rather than making a broken folder.
# ---------------------------------------------------------------------------

echo "=== Preflight ==="

EXE_PATH="$BUILD_DIR/$EXE_NAME"
[ -f "$EXE_PATH" ] || die "exe not found at '$EXE_PATH'. Build first, or fix EXE_NAME/BUILD_DIR."
info "exe:  $EXE_PATH"

command -v ntldd >/dev/null 2>&1 || \
    die "ntldd not found. Install it:  pacman -S mingw-w64-ucrt-x86_64-ntldd"
info "ntldd: $(command -v ntldd)"

if [ ! -f "$IWAD" ]; then
    warn "IWAD not found at '$IWAD' — the folder will build WITHOUT a WAD."
    warn "The game won't run on the totem until a WAD is placed next to the exe."
    HAVE_IWAD=0
else
    info "iwad: $IWAD"
    HAVE_IWAD=1
fi

# ---------------------------------------------------------------------------
# Build the clean output folder.
# ---------------------------------------------------------------------------

echo ""
echo "=== Building $DIST_ROOT/$DIST_NAME ==="

DEST="$DIST_ROOT/$DIST_NAME"
rm -rf "$DEST"
mkdir -p "$DEST"

# 1. The executable.
cp "$EXE_PATH" "$DEST/"
info "copied $EXE_NAME"

# 2. Every DLL it needs, resolved recursively.
#    ntldd -R walks the whole dependency tree, so transitive deps (SDL2_mixer
#    -> libvorbis -> libogg, etc.) are all caught. We keep only the ones under
#    the UCRT64 bin — the rest are Windows system DLLs already on the totem.
echo ""
info "resolving DLLs (ntldd -R)..."

DLL_COUNT=0
# ntldd prints lines like:  libSDL2.dll => /ucrt64/bin/libSDL2.dll (0x...)
# Grab the resolved path in the third field, keep UCRT64 ones, dedupe.
while IFS= read -r dll; do
    [ -z "$dll" ] && continue
    if [ -f "$dll" ]; then
        cp -n "$dll" "$DEST/"
        info "  + $(basename "$dll")"
        DLL_COUNT=$((DLL_COUNT + 1))
    else
        warn "  resolved DLL path does not exist: $dll"
    fi
done < <(
    ntldd -R "$EXE_PATH" \
    | grep -iF "$UCRT_BIN" \
    | sed -E 's/.*=>[[:space:]]*([^ ]+).*/\1/' \
    | sort -u
)

[ "$DLL_COUNT" -gt 0 ] || warn "no UCRT64 DLLs were copied — check ntldd output and UCRT_BIN='$UCRT_BIN'."
info "copied $DLL_COUNT DLL(s)"

# 3. The IWAD.
if [ "$HAVE_IWAD" -eq 1 ]; then
    cp "$IWAD" "$DEST/"
    info "copied $(basename "$IWAD")"
fi

# 4. Touch assets (optional).
if [ -d "$TOUCH_ASSETS_DIR" ]; then
    mkdir -p "$DEST/touch"
    cp -r "$TOUCH_ASSETS_DIR/." "$DEST/touch/"
    info "copied touch assets from $TOUCH_ASSETS_DIR"
else
    info "no touch assets dir ($TOUCH_ASSETS_DIR) — skipping (fine if you draw rects)"
fi

# 5. Config (optional).
if [ -f "$CONFIG_FILE" ]; then
    cp "$CONFIG_FILE" "$DEST/"
    info "copied config $(basename "$CONFIG_FILE")"
else
    info "no config file ($CONFIG_FILE) — the totem will write its own on first run"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

echo ""
echo "=== Done ==="
info "folder: $DEST"
info "size:   $(du -sh "$DEST" | cut -f1)"
echo ""
echo "Contents:"
ls -1 "$DEST" | sed 's/^/    /'
echo ""
echo "Next steps:"
echo "  1. VALIDATE on a clean Windows machine that never had MSYS2 —"
echo "     if it runs there, it runs on the totem. Testing only on this dev"
echo "     box gives a false pass (MSYS2's PATH hides missing DLLs)."
echo "  2. Copy the '$DIST_NAME' folder to the totem and run $EXE_NAME."
