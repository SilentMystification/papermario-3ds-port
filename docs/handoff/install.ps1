# Run from the repo root: powershell -NoProfile -File docs\handoff\install.ps1
# 1. Appends the orchestrator rules to CLAUDE.md (if not there yet).
# 2. Adds a UserPromptSubmit hook to .claude\settings.local.json that prints the rules on every prompt.
$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path
$rules = Get-Content -Raw (Join-Path $root 'docs\handoff\ORCH_RULES.txt')

$claude = Join-Path $root 'CLAUDE.md'
if (-not (Test-Path $claude)) { New-Item -ItemType File $claude | Out-Null }
if (-not (Select-String -Path $claude -SimpleMatch 'ORCHESTRATOR RULES (papermario-3ds-port)' -Quiet)) {
    Add-Content -Path $claude -Value ("`n## Orchestrator rules`n" + $rules)
    Write-Host 'CLAUDE.md: rules appended.'
} else { Write-Host 'CLAUDE.md: rules already there.' }

$sp = Join-Path $root '.claude\settings.local.json'
if (Test-Path $sp) { $j = Get-Content -Raw $sp | ConvertFrom-Json } else { $j = [pscustomobject]@{} }
$hookCmd = 'cat "$CLAUDE_PROJECT_DIR/docs/handoff/ORCH_RULES.txt"'
$hook = [pscustomobject]@{ hooks = @([pscustomobject]@{ type = 'command'; command = $hookCmd }) }
if ($j.PSObject.Properties.Name -contains 'hooks') {
    $j.hooks | Add-Member -NotePropertyName UserPromptSubmit -NotePropertyValue @($hook) -Force
} else {
    $j | Add-Member -NotePropertyName hooks -NotePropertyValue ([pscustomobject]@{ UserPromptSubmit = @($hook) })
}
$j | ConvertTo-Json -Depth 10 | Set-Content -Encoding utf8 $sp
Write-Host 'settings.local.json: hook written. Restart Claude Code or open /hooks once to load it.'
