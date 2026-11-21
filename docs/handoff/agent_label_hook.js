// PreToolUse guard on Agent (user, 20 Nov): every agent launch carries its model in the label, so the user sees who runs what.
// The description must start with `[<Model> <role>]` (Haiku, Sonnet, Opus or Fable). With a `model` input the label must match it.
// Without one the label must match the model of the agent definition (.claude/agents/<type>.md front matter), else the default (Sonnet).
const fs = require('fs'), path = require('path');
let raw = '';
process.stdin.on('data', d => { raw += d; });
process.stdin.on('end', () => {
  let j; try { j = JSON.parse(raw); } catch { process.exit(0); }
  if ((j.tool_name || '') !== 'Agent') process.exit(0);
  const t = j.tool_input || {};
  const label = String(t.description || '').match(/^\[(Haiku|Sonnet|Opus|Fable) [A-Za-z ]+\]/);
  let want = String(t.model || '').toLowerCase();
  if (!want) {
    want = 'sonnet';
    try {
      const root = process.env.CLAUDE_PROJECT_DIR || process.cwd();
      const f = path.join(root, '.claude', 'agents', String(t.subagent_type || 'general-purpose') + '.md');
      const m = fs.readFileSync(f, 'utf8').match(/^model:\s*(\S+)/m);
      if (m) want = m[1].toLowerCase();
    } catch { /* no definition: default */ }
  }
  if (!label) {
    process.stderr.write('Blocked (user rule 20 Nov): the Agent description must start with the model and role, e.g. "[' +
      want.charAt(0).toUpperCase() + want.slice(1) + ' coder] GD7 status bar memo". Add the label and launch again.\n');
    process.exit(2);
  }
  if (label[1].toLowerCase() !== want) {
    process.stderr.write('Blocked (user rule 20 Nov): the label says ' + label[1] + ' but this agent runs on ' + want +
      '. Fix the label or the model input.\n');
    process.exit(2);
  }
  process.exit(0);
});
