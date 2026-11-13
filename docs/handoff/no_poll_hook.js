// PreToolUse guard on Bash (user, 2026-11-13): no polling and no idle waits. Blocks sleep, ping -n/-c, tail --pid, timeout /t and
// Start-Sleep at a command position. The harness sends a completion notice when a background job ends: end the turn and wait for it.
let raw = '';
process.stdin.on('data', d => { raw += d; });
process.stdin.on('end', () => {
  let j; try { j = JSON.parse(raw); } catch { process.exit(0); }
  if ((j.tool_name || '') !== 'Bash') process.exit(0);
  const cmd = String((j.tool_input || {}).command || '');
  const re = /(?:^|[;&|(\n]|\bdo\b|\bthen\b)\s*(?:sleep\s+\d|ping\s+-[nc]\s|tail\s[^;&|\n]*--pid|timeout\s+\/t|start-sleep\b)/i;
  if (!re.test(cmd)) process.exit(0);
  process.stderr.write('Blocked (user rule 2026-11-13, CLAUDE.md rule 6): no polling, no sleep, no ping loop, no tail --pid. ' +
    'Run the job in the foreground or in the background, then end your turn. The harness sends a notice when the job ends.\n');
  process.exit(2);
});
