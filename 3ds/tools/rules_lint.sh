#!/bin/sh
# rules_lint.sh: the rule files are edited in place. Fails when one grows past its cap or holds a stale phrase.
# Run by the Stop hook (docs/handoff/orch_facts.js). Exit 0 OK, 1 problem. Rule files are a current list, never a log.
cd "$(git rev-parse --show-toplevel)" || exit 2
bad=0
chk() { # file max-lines
    n=$(wc -l <"$1" | tr -d ' ')
    if [ "$n" -gt "$2" ]; then
        echo "rules_lint: $1 has $n lines, cap $2: merge the new text into the existing rule lines and delete the old ones"
        bad=1
    fi
}
chk docs/handoff/orchestrator_rules.md 70
chk CLAUDE.md 60
for f in docs/handoff/orchestrator_rules.md CLAUDE.md .claude/agents/coder.md .claude/agents/oracle.md; do
    if grep -q -E 'handback_<id>|INDEX\.md line' "$f"; then
        echo "rules_lint: $f holds a stale phrase (handback_<id> or INDEX.md line): rewrite that rule in place"
        bad=1
    fi
done
for f in docs/handoff/orchestrator_rules.md CLAUDE.md .claude/agents/coder.md .claude/agents/oracle.md docs/handoff/push_runbook.md; do
    if grep -q -E '20[0-9]{2}-[0-9]{2}-[0-9]{2}|\(user [0-9]|violations? on' "$f"; then
        echo "rules_lint: $f holds a date or a (user ...) tag: a rule file is a current list, history is in git. Remove it"
        bad=1
    fi
done
core=$(awk '/<!-- CORE -->/{f=1;next} /<!-- \/CORE -->/{f=0} f' docs/handoff/orchestrator_rules.md | wc -c | tr -d ' ')
if [ "$core" -gt 1500 ]; then
    echo "rules_lint: the Core block is $core characters, cap 1500"
    bad=1
fi
[ "$bad" = 0 ] && echo "rules_lint OK"
exit "$bad"
