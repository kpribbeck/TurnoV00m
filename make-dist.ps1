<#
    make-dist.ps1  -  assemble a portable, ready-to-ship folder for the totem.

    Produces  dist\turnovoom\  containing:
        - the game exe
        - EVERY MinGW/SDL DLL it needs (resolved recursively from the PE
          import tables, using only PowerShell - no ntldd, no MSYS2 tools)
        - the IWAD
        - the touch\ PNG assets (if present)
        - a config file (if present)

    Copy that one folder to the totem and run the exe. No installer.

    Run from the repo root in PowerShell:
        powershell -ExecutionPolicy Bypass -File .\make-dist.ps1

    Adjust the paths in the CONFIG block below to match your layout.
#>

# --------------------------------------------------------------------------
# CONFIG - edit these to match your setup.
# --------------------------------------------------------------------------

# Built executable name.
$ExeName = "TurnoV00m.exe"

# Where the build put the exe (relative to repo root).
$BuildDir = "build\src"

# Your IWAD.
$Iwad = "..\V00mAssets\TurnoV00m.WAD"

# MSYS2 UCRT64 bin - where the runtime/SDL DLLs live. Default MSYS2 install.
$UcrtBin = "C:\msys64\ucrt64\bin"

# Optional extras (copied only if they exist).
$TouchAssetsDir = "assets\touch"
$ConfigFile     = "dist-assets\crispy-doom.cfg"

# Output.
$DistRoot = "dist"
$DistName = "TurnoV00m"

# --------------------------------------------------------------------------
# PE import reader - finds the DLL names an exe/dll imports, from its header.
# Pure PowerShell + .NET; nothing to install.
# --------------------------------------------------------------------------

function Get-ImportedDlls {
    param([string]$FilePath)

    $bytes = [System.IO.File]::ReadAllBytes($FilePath)

    # DOS header -> e_lfanew at 0x3C gives the PE header offset.
    $peOff = [BitConverter]::ToInt32($bytes, 0x3C)
    # "PE\0\0" signature check.
    if ($bytes[$peOff] -ne 0x50 -or $bytes[$peOff+1] -ne 0x45) { return @() }

    $coffOff = $peOff + 4
    $numSections = [BitConverter]::ToUInt16($bytes, $coffOff + 2)
    $optSize     = [BitConverter]::ToUInt16($bytes, $coffOff + 16)
    $optOff      = $coffOff + 20

    # PE32 (0x10B) vs PE32+ (0x20B) changes where the data directories sit.
    $magic = [BitConverter]::ToUInt16($bytes, $optOff)
    $ddOff = if ($magic -eq 0x20B) { $optOff + 112 } else { $optOff + 96 }

    # Data directory [1] = Import Table (RVA at ddOff + 8).
    $importRva = [BitConverter]::ToInt32($bytes, $ddOff + 8)
    if ($importRva -eq 0) { return @() }

    # Section headers follow the optional header. Build RVA->file-offset map.
    $secOff = $optOff + $optSize
    $sections = @()
    for ($i = 0; $i -lt $numSections; $i++) {
        $s = $secOff + ($i * 40)
        $sections += [pscustomobject]@{
            VA   = [BitConverter]::ToInt32($bytes, $s + 12)
            VSz  = [BitConverter]::ToInt32($bytes, $s + 8)
            Raw  = [BitConverter]::ToInt32($bytes, $s + 20)
        }
    }

    function RvaToOffset($rva) {
        foreach ($s in $sections) {
            if ($rva -ge $s.VA -and $rva -lt ($s.VA + $s.VSz)) {
                return $s.Raw + ($rva - $s.VA)
            }
        }
        return -1
    }

    # Walk the import directory: 20-byte entries, terminated by an all-zero one.
    # Field at +12 is the RVA of the imported DLL's name (ASCII, null-terminated).
    $names = @()
    $impOff = RvaToOffset $importRva
    if ($impOff -lt 0) { return @() }

    while ($true) {
        $nameRva = [BitConverter]::ToInt32($bytes, $impOff + 12)
        if ($nameRva -eq 0) { break }
        $nameOff = RvaToOffset $nameRva
        if ($nameOff -lt 0) { break }
        $sb = New-Object System.Text.StringBuilder
        while ($bytes[$nameOff] -ne 0) {
            [void]$sb.Append([char]$bytes[$nameOff]); $nameOff++
        }
        $names += $sb.ToString()
        $impOff += 20
    }
    return $names
}

# Recursively resolve every DLL the exe needs that lives in UcrtBin.
function Resolve-Dependencies {
    param([string]$StartFile, [string]$SearchDir)

    $resolved = @{}          # dll name (lower) -> full path
    $queue    = New-Object System.Collections.Queue
    $queue.Enqueue($StartFile)

    while ($queue.Count -gt 0) {
        $file = $queue.Dequeue()
        foreach ($dll in Get-ImportedDlls $file) {
            $key = $dll.ToLower()
            if ($resolved.ContainsKey($key)) { continue }
            $candidate = Join-Path $SearchDir $dll
            if (Test-Path $candidate) {
                # It's one of ours (MinGW/SDL) - copy it and scan its imports too.
                $resolved[$key] = $candidate
                $queue.Enqueue($candidate)
            }
            # Not in UcrtBin => a Windows system DLL, already on the totem. Skip.
        }
    }
    return $resolved.Values
}

# --------------------------------------------------------------------------
# Preflight
# --------------------------------------------------------------------------

Write-Host "=== Preflight ==="

$ExePath = Join-Path $BuildDir $ExeName
if (-not (Test-Path $ExePath)) {
    Write-Error "exe not found at '$ExePath'. Build first, or fix ExeName/BuildDir."
    exit 1
}
# Resolve to a full path. Bare .NET calls like [System.IO.File]::ReadAllBytes
# ignore PowerShell's current directory and use the process's own, so a
# relative path resolves against the wrong base. Absolute paths avoid that.
$ExePath = (Resolve-Path $ExePath).Path
Write-Host "  exe:      $ExePath"

if (-not (Test-Path $UcrtBin)) {
    Write-Error "UCRT64 bin not found at '$UcrtBin'. Fix `$UcrtBin (where MSYS2 keeps the DLLs)."
    exit 1
}
$UcrtBin = (Resolve-Path $UcrtBin).Path
Write-Host "  ucrt bin: $UcrtBin"

$HaveIwad = Test-Path $Iwad
if ($HaveIwad) { Write-Host "  iwad:     $Iwad" }
else { Write-Warning "IWAD not found at '$Iwad' - folder will build WITHOUT a WAD." }

# --------------------------------------------------------------------------
# Build the folder
# --------------------------------------------------------------------------

Write-Host ""
Write-Host "=== Building $DistRoot\$DistName ==="

$Dest = Join-Path $DistRoot $DistName
if (Test-Path $Dest) { Remove-Item $Dest -Recurse -Force }
New-Item -ItemType Directory -Path $Dest -Force | Out-Null

# 1. The exe.
Copy-Item $ExePath $Dest
Write-Host "  copied $ExeName"

# 2. All DLLs, resolved recursively from the import tables.
Write-Host "  resolving DLLs (reading PE imports)..."
$dlls = Resolve-Dependencies -StartFile $ExePath -SearchDir $UcrtBin
foreach ($d in $dlls) {
    Copy-Item $d $Dest
    Write-Host "    + $([System.IO.Path]::GetFileName($d))"
}
if ($dlls.Count -eq 0) {
    Write-Warning "no DLLs resolved - check `$UcrtBin points at the right bin."
}
Write-Host "  copied $($dlls.Count) DLL(s)"

# 3. IWAD.
if ($HaveIwad) {
    Copy-Item $Iwad $Dest
    Write-Host "  copied $([System.IO.Path]::GetFileName($Iwad))"
}

# 4. Touch assets (optional).
if (Test-Path $TouchAssetsDir) {
    $touchDest = Join-Path $Dest "touch"
    New-Item -ItemType Directory -Path $touchDest -Force | Out-Null
    Copy-Item (Join-Path $TouchAssetsDir "*") $touchDest -Recurse
    Write-Host "  copied touch assets"
} else {
    Write-Host "  no touch assets dir ($TouchAssetsDir) - skipping"
}

# 5. Config (optional).
if (Test-Path $ConfigFile) {
    Copy-Item $ConfigFile $Dest
    Write-Host "  copied config"
} else {
    Write-Host "  no config file ($ConfigFile) - totem writes its own on first run"
}

# --------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------

Write-Host ""
Write-Host "=== Done ==="
Write-Host "  folder: $Dest"
Write-Host ""
Write-Host "Contents:"
Get-ChildItem $Dest | ForEach-Object { Write-Host "    $($_.Name)" }
Write-Host ""
Write-Host "Next: VALIDATE on a clean Windows PC that never had MSYS2 before"
Write-Host "trusting it on the totem, then copy the '$DistName' folder over and run."