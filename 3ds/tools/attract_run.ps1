<#
attract_run.ps1 - one command: run each demo scene (demoscene=<n>, 600 frames) in Azahar, then write the per-scene report and the checks.

  PM_ROW=<row> powershell -File 3ds/tools/attract_run.ps1 [-Name <tag>] [-Scenes 0,1,2,3,4,5,6] [-Timeout 90]

Each run stops at "game mode 17 frame 600" (600 frames into the demo state). Timeout is capped at 90 s. No turbo: the numbers need normal speed.
Scene index: DemoScenes in src/state_demo.c (0 nok_12, 1 isk_04, 2 tik_03, 3 trd_09, 4 jan_04, 5 mac_00, 6 kzn_02).
Uses build3ds/pm_3ds.3dsx as it is. Build first if the source changed (see 3ds/run_azahar.ps1 header). One Azahar start per scene.
Output per scene:
  3ds/logs/perf/attract_<tag>_s<n>_run.txt   game log
  3ds/logs/perf/attract_<tag>_s<n>.txt       per scene rows and the PI1-PI5 checks
  3ds/logs/perf/console_ready.txt            verdict of the last scene run
#>
param(
    [string]$Name = "attract",
    [int[]]$Scenes = @(0, 1, 2, 3, 4, 5, 6),
    [int]$Timeout = 90
)
$ErrorActionPreference = "Stop"
if ($Timeout -gt 90) { Write-Output "Timeout is capped at 90 s. Fix the until-line, do not extend the timeout."; exit 2 }
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$perf = "$repo\3ds\logs\perf"
if (-not $env:PM_ROW) { Write-Output "PM_ROW is not set. Set PM_ROW to your row id first."; exit 2 }
$last = "$repo\build3ds\last_log.txt"
$worst = 0
foreach ($s in $Scenes) {
    if (Test-Path $last) { Remove-Item $last -Force }
    & powershell -NoProfile -File "$repo\3ds\run_azahar.ps1" -Until "game mode 17 frame 600" -Timeout $Timeout -Force -Debug "loglevel=debug phase demoscene=$s"
    $rc = $LASTEXITCODE
    if (-not (Test-Path $last)) { Write-Output "scene $s NO LOG: run_azahar exit $rc, no game log written."; exit 1 }
    $tag = "$($Name)_s$s"
    Copy-Item $last "$perf\attract_$($tag)_run.txt" -Force
    & python "$repo\3ds\tools\attract_report.py" "$perf\attract_$($tag)_run.txt" "$perf\attract_$tag.txt" $tag "$perf\console_ready.txt"
    if ($LASTEXITCODE -ne 0) { Write-Output "scene $s REPORT FAILED."; exit 1 }
    if ($rc -ne 0) { $worst = $rc }
    Write-Output "scene $s run exit $rc. Report: 3ds/logs/perf/attract_$tag.txt"
}
exit $worst
