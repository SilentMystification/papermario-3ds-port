// orch_facts.js: facts from disk for the orchestrator (hook script).
//   node orch_facts.js facts   (UserPromptSubmit) prints the facts of this turn
//   node orch_facts.js stop    (Stop) blocks the end of a turn once when a hand-back waits or main is dirty
// Agent states are NOT here: ListAgents is a tool, not a shell command. The facts line says so.
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const root = process.env.CLAUDE_PROJECT_DIR || process.cwd();
const mode = process.argv[2] || 'facts';

function sh(cmd, cwd) {
  try {
    return execSync(cmd, { cwd: cwd || root, encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'], timeout: 4000 }).trim();
  } catch (e) { return ''; }
}

function newestMtime(dir) {
  let best = 0;
  let names;
  try { names = fs.readdirSync(dir); } catch (e) { return 0; }
  for (const n of names) {
    try { const m = fs.statSync(path.join(dir, n)).mtimeMs; if (m > best) best = m; } catch (e) { /* skip */ }
  }
  return best;
}

function worktrees() {
  const out = [];
  const base = path.join(root, 'build3ds', 'wt');
  let ids;
  try { ids = fs.readdirSync(base); } catch (e) { return out; }
  const now = Date.now();
  for (const id of ids) {
    const wt = path.join(base, id);
    const ahead = sh('git rev-list --count main..' + id);
    if (ahead === '') continue; // no branch: a leftover folder, not a worktree
    const outDir = path.join(wt, 'build3ds', 'out');
    const act = newestMtime(outDir);
    let hb = '';
    let hbAge = null;
    try {
      for (const n of fs.readdirSync(outDir)) {
        if (n === 'handback_' + id + '.txt') { // only the file of this row: copies of old rows are stale
          const age = (now - fs.statSync(path.join(outDir, n)).mtimeMs) / 60000;
          if (hbAge === null || age < hbAge) { hbAge = age; hb = n; }
        }
      }
    } catch (e) { /* no out dir */ }
    out.push({ id, ahead: parseInt(ahead, 10) || 0, actMin: act ? Math.round((now - act) / 60000) : null, hb, hbAge });
  }
  return out;
}

const dirty = sh('git status --porcelain').split('\n').filter(Boolean);
const wts = worktrees();

if (mode === 'facts') {
  const lines = [];
  lines.push('FACTS FROM DISK (orch_facts.js). Agent states: call ListAgents now (completed = IDLE). Then check idle capacity.');
  lines.push('main ' + sh('git rev-parse --short HEAD') + (dirty.length ? ' DIRTY: ' + dirty.slice(0, 4).join(' | ') + (dirty.length > 4 ? ' ...' : '') : ' clean'));
  // Token cost: list only worktrees with a file in the last 4 hours or a young hand-back. The rest is one count.
  let old = 0;
  for (const w of wts) {
    if (w.actMin === null || w.actMin > 240) { old++; continue; }
    lines.push('worktree ' + w.id + ': ahead ' + w.ahead + ', newest file ' + w.actMin + ' min ago');
  }
  if (old) lines.push(old + ' old worktrees with no file in 4 h (parked branches, ignore)');
  let pages = [];
  try { pages = fs.readdirSync(path.join(root, 'docs', 'handoff')).filter(n => /^ws_.*\.md$/.test(n) && !wts.some(w => 'ws_' + w.id + '.md' === n)); } catch (e) { /* none */ }
  lines.push('free goal pages: ' + (pages.length ? pages.join(' ') : 'NONE (ask the oracle for pages)'));
  process.stdout.write(lines.join('\n') + '\n');
  process.exit(0);
}

if (mode === 'stop') {
  let input = '';
  try { input = fs.readFileSync(0, 'utf8'); } catch (e) { /* no stdin */ }
  let data = {};
  try { data = JSON.parse(input || '{}'); } catch (e) { /* ignore */ }
  if (data.stop_hook_active) process.exit(0); // never loop
  const problems = [];
  // Files the oracle writes while it runs are not a problem yet: the orchestrator commits them at the hand-back.
  const stray = dirty.filter(l => !/docs\/handoff\/(ws_|design_|oracle_handoff|row_|INDEX\.md|audit_)/.test(l));
  { // rules_lint: the rule files are edited in place, never appended (user 2026-11-16)
    const r = require('child_process').spawnSync('sh', ['3ds/tools/rules_lint.sh'], { cwd: root, encoding: 'utf8', timeout: 4000 });
    if (r.status === 1) problems.push(String(r.stdout || '').trim().split('\n').join(' | '));
  }
  if (stray.length) problems.push('main is dirty (' + stray.slice(0, 3).join(', ') + '): commit or remove the files');
  // Hand-backs arrive as agent messages and ListAgents shows the status: no disk check for them (false alarms, user 2026-11-16).
  if (problems.length) {
    process.stdout.write(JSON.stringify({ decision: 'block', reason: 'Disk facts before you end the turn: ' + problems.join('; ') + '.' }));
  }
  process.exit(0);
}
