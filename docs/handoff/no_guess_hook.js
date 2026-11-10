// PreToolUse guard (user, 2026-11-10): no guess and check. Blocks a row file write or an agent prompt/message that asks for a
// debug-log check, a premise-check run or a repro run. Bugs are fixed from the oracle's code read of how the thing draws.
let raw = '';
process.stdin.on('data', d => { raw += d; });
process.stdin.on('end', () => {
  let j; try { j = JSON.parse(raw); } catch { process.exit(0); }
  const t = j.tool_name || '', i = j.tool_input || {};
  let text = '';
  if (t === 'Write' || t === 'Edit') {
    const f = String(i.file_path || '').replace(/\\/g, '/');
    if (!/docs\/handoff\/row_[^/]*\.md$/i.test(f)) process.exit(0);
    text = t === 'Write' ? String(i.content || '') : String(i.new_string || '');
  } else if (t === 'SendMessage') text = String(i.message || '');
  else if (t === 'Agent') text = String(i.prompt || '');
  else process.exit(0);
  const bad = ['log line', 'debug line', 'check first', 'print the value', 'prints the suspect', 'reproduce', 'repro run',
               'premise check', 'not reproduced'];
  const low = text.toLowerCase(), hit = bad.filter(p => low.includes(p));
  if (!hit.length) process.exit(0);
  process.stderr.write('Blocked (user rule 2026-11-10): no guess and check. Fix from the oracle\'s code read of how the thing draws; ' +
    'a run is only the gate of a finished fix. Phrases found: ' + hit.join(', ') + '\n');
  process.exit(2);
});
