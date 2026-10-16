# E01: extract assets from the US ROM with the repo's splat setup and build generated headers + .png.bin files.
# Run from anywhere: powershell -File 3ds\tools\extract_assets.ps1 [-Rom path\to\papermario.z64]
# Needs: Python 3, ninja on PATH. Outputs (all git-ignored): assets/us, ver/us/build, build.ninja, ver/us/baserom.z64, ver/us/.splat_cache.
param([string]$Rom = "$env:APPDATA\Azahar\sdmc\3ds\PaperMario\papermario.z64")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
Set-Location $root
$env:PYTHONUTF8 = "1"

# 1. ROM hash check against ver/us/checksum.sha1
$want = (Get-Content ver\us\checksum.sha1).Split(" ")[0]
if ((Get-FileHash $Rom -Algorithm SHA1).Hash -ne $want.ToUpper()) { throw "ROM SHA1 mismatch (want $want)" }
Copy-Item $Rom ver\us\baserom.z64 -Force   # baserom.* is git-ignored

# 2. venv with the repo's python deps (+ pigment64/crunch64 python packages and pyinstaller for the crunch64 exe)
$venv = "build3ds\.venv_e01"
if (-not (Test-Path "$venv\Scripts\python.exe")) {
    python -m venv $venv
    & "$venv\Scripts\python.exe" -m pip install -q -r tools\configure\requirements.txt pigment64 pyinstaller
}
$py = "$venv\Scripts\python.exe"

# 3. splat extraction + build.ninja (3ds\tools\extract_assets.py)
& $py 3ds\tools\extract_assets.py
if ($LASTEXITCODE) { throw "extraction failed" }

# 4. sprites.py calls a `crunch64` exe by name: build one from the python package (once)
if (-not (Test-Path build3ds\e01_bin\crunch64.exe)) {
    Push-Location build3ds
    & ".venv_e01\Scripts\python.exe" -m PyInstaller --onefile --distpath e01_bin --workpath e01_pyi --specpath e01_pyi -n crunch64 ..\3ds\tools\crunch64_shim.py
    Pop-Location
}
Remove-Item build3ds\e01_bin\crunch64.cmd -ErrorAction SilentlyContinue

# 5. generated headers and image bins only (no N64 compile). crash_screen/font.png.bin (i1) is expected to fail.
$env:PATH = "$root\build3ds\e01_bin;$env:PATH"
$null | ninja e01_assets -k 0
Write-Host "done (one expected failure: ver/us/build/crash_screen/font.png.bin)"
