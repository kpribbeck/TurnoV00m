#!/usr/bin/env bash
#
# make-dist.sh — assemble a self-contained, portable folder for the totem.
#
# Produces  dist/TurnoV00m/  containing:
#   - the game exe
#   - EVERY MinGW/SDL DLL it needs (resolved recursively from the PE import
#     tables with objdump, which ships with the UCRT64 toolchain)
#   - the IWAD
#   - the touch/ PNG assets (if present)
#   - a config file (if present)
#   - the standalone .ico (only if SHIP_ICON=1)
#
# Copy that one folder to the totem and run the exe. No installer, no MSYS2.
#
# Run from the MSYS2 UCRT64 shell, at the repo root:
#     ./make-dist.sh
#
# make-dist.ps1 does the same from PowerShell — keep the two in sync.

set -euo pipefail

# ---------------------------------------------------------------------------
# Configuration — adjust these paths to match your layout.
# ---------------------------------------------------------------------------

# Name of the built executable (change if you renamed it via OUTPUT_NAME).
EXE_NAME="TurnoV00m.exe"

# Where the build put the exe.
BUILD_DIR="build/src"

# Your IWAD.
IWAD="../V00mAssets/TurnoV00m.WAD"

# UCRT64 bin dir (where MSYS2 keeps the runtime/SDL DLLs).
UCRT_BIN="/ucrt64/bin"

# Optional extras — copied only if they exist. Leave as-is; the script skips
# any that are missing.
TOUCH_ASSETS_DIR="assets/touch"
CONFIG_FILE="dist-assets/turnov00m.cfg"

# The exe carries its icon embedded in its resources, so a plain double-click
# shows the right icon and auto-detects TurnoV00m.WAD sitting beside it - no
# launcher needed. Optionally also ship the standalone .ico (handy if you want
# to make a desktop shortcut on the totem, or set a custom folder icon). Off
# by default: leave it 0 for a clean exe+DLLs+WAD folder.
SHIP_ICON=0
ICON_FILE="data/turno-v00m.ico"

# Output location.
DIST_ROOT="dist"
DIST_NAME="TurnoV00m"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

info()  { printf '  %s\n' "$*"; }
warn()  { printf 'WARNING: %s\n' "$*" >&2; }
die()   { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

# DLL names a PE file imports, read from its import table.
imported_dlls() {
    objdump -p "$1" | sed -n 's/^[[:space:]]*DLL Name:[[:space:]]*//p' | tr -d '\r'
}

# ---------------------------------------------------------------------------
# Preflight checks — fail early and clearly rather than making a broken folder.
# ---------------------------------------------------------------------------

echo "=== Preflight ==="

EXE_PATH="$BUILD_DIR/$EXE_NAME"
[ -f "$EXE_PATH" ] || die "exe not found at '$EXE_PATH'. Build first, or fix EXE_NAME/BUILD_DIR."
info "exe:      $EXE_PATH"

[ -d "$UCRT_BIN" ] || die "UCRT64 bin not found at '$UCRT_BIN'. Fix UCRT_BIN (where MSYS2 keeps the DLLs)."
info "ucrt bin: $UCRT_BIN"

command -v objdump >/dev/null 2>&1 || \
    die "objdump not found. Run this from the MSYS2 UCRT64 shell (it comes with the toolchain)."

if [ -f "$IWAD" ]; then
    info "iwad:     $IWAD"
    HAVE_IWAD=1
else
    warn "IWAD not found at '$IWAD' - folder will build WITHOUT a WAD."
    HAVE_IWAD=0
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

# 2. Every DLL it needs, resolved recursively from the import tables.
#    Anything found in UCRT_BIN is ours (MinGW/SDL): copy it and scan its
#    imports too, so transitive deps (SDL2_mixer -> libvorbis -> libogg, etc.)
#    are caught. Anything not there is a Windows system DLL, already on the
#    totem, so it's skipped.
info "resolving DLLs (reading PE imports)..."

declare -A SEEN=()
QUEUE=("$EXE_PATH")
DLL_COUNT=0
while [ "${#QUEUE[@]}" -gt 0 ]; do
    file="${QUEUE[0]}"
    QUEUE=("${QUEUE[@]:1}")
    while IFS= read -r dll; do
        [ -z "$dll" ] && continue
        key="${dll,,}"
        [ -n "${SEEN[$key]:-}" ] && continue
        SEEN[$key]=1
        candidate="$UCRT_BIN/$dll"
        if [ -f "$candidate" ]; then
            cp "$candidate" "$DEST/"
            info "  + $dll"
            DLL_COUNT=$((DLL_COUNT + 1))
            QUEUE+=("$candidate")
        fi
    done < <(imported_dlls "$file")
done

[ "$DLL_COUNT" -gt 0 ] || warn "no DLLs resolved - check UCRT_BIN points at the right bin."
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
    info "copied touch assets"
else
    info "no touch assets dir ($TOUCH_ASSETS_DIR) - skipping"
fi

# 5. Config (optional).
if [ -f "$CONFIG_FILE" ]; then
    cp "$CONFIG_FILE" "$DEST/"
    info "copied config"
else
    info "no config file ($CONFIG_FILE) - totem writes its own on first run"
fi

# 6. Standalone icon file (optional).
if [ "$SHIP_ICON" -eq 1 ]; then
    if [ -f "$ICON_FILE" ]; then
        cp "$ICON_FILE" "$DEST/"
        info "copied $(basename "$ICON_FILE")"
    else
        warn "icon not found at '$ICON_FILE' - skipping (exe still has its embedded icon)."
    fi
else
    info "not shipping standalone .ico (SHIP_ICON=0; exe icon is embedded)"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

echo ""
echo "=== Done ==="
info "folder: $DEST"
echo ""
echo "Contents:"
ls -1 "$DEST" | sed 's/^/    /'
echo ""
echo "Launch: double-click $EXE_NAME. It auto-detects TurnoV00m.WAD beside"
echo "it, and the icon is embedded in the exe - no launcher needed."
echo ""
echo "Next: VALIDATE on a clean Windows PC that never had MSYS2 before"
echo "trusting it on the totem, then copy the '$DIST_NAME' folder over and run."
