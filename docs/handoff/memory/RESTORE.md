# Restore the Claude memory files on a new machine

Memory does not travel with git. These files are verbatim copies of
/home/andrew/.claude/projects/-home-andrew-Documents-GitHub-papermario-3ds-port/memory/ (old machine).

Path pattern: ~/.claude/projects/<project-dir-name>/memory/
<project-dir-name> = the absolute path of the repo folder with every "/" replaced by "-"
(and a leading "-"). Example: /home/andrew/Documents/GitHub/papermario-3ds-port
becomes -home-andrew-Documents-GitHub-papermario-3ds-port.
If the repo is in another folder on the new machine, use that folder name instead.

Install:
```
D=~/.claude/projects/$(pwd | sed 's#/#-#g')/memory     # run in the repo root
mkdir -p "$D"
cp docs/handoff/memory/*.md "$D"/ && rm "$D/RESTORE.md"
ls "$D"
```
Files: MEMORY.md (index), feedback_luma_and_baselines.md, feedback_push_failures.md,
feedback_sweeps_manual.md, feedback_visible_status.md, project_perf_azahar_only.md.
Start Claude Code once in the repo folder so the project directory exists, then copy.
