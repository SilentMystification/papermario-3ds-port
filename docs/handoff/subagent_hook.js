// SubagentStart hook: inject subagent_style.txt into every subagent (the only channel that reaches subagents).
const fs = require('fs'), path = require('path');
const text = fs.readFileSync(path.join(__dirname, 'subagent_style.txt'), 'utf8');
process.stdout.write(JSON.stringify({ hookSpecificOutput: { hookEventName: 'SubagentStart', additionalContext: text } }));
