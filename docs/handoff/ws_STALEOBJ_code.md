# ws_STALEOBJ code: 3 edits in 3ds/build.sh (verified against main 02899966ac, file:line)
Make each edit with the exact text. Change no other line.
## Edit 1: build.sh:193 (vol_match needs the new tag)
Before:
`vol_match() { [ -n "$VOLID" ] && [ "$VOLID" = "$(cat $VID 2>/dev/null)" ]; }`
After:
`vol_match() { case "$VOLID" in m*) ;; *) return 1 ;; esac; [ "$VOLID" = "$(cat $VID 2>/dev/null)" ]; } # STALEOBJ: an id without the m tag is from a sync that kept host mtimes`
## Edit 2: build.sh:219 (the new sync id has the tag)
Before:
`if vol_match; then SYNCID=$VOLID; else rm -f $MAN; SYNCID="$(date +%s)_$$"; fi`
After:
`if vol_match; then SYNCID=$VOLID; else rm -f $MAN; SYNCID="m$(date +%s)_$$"; fi`
## Edit 3: build.sh:231 (copied files get the copy time)
Before (part of the line):
`... docker run --rm -i -v $SRCVOL:/src $IMG sh -c "tar -xf - -C /src && printf %s '$SYNCID' > /src/.pm_manifest_id"`
After:
`... docker run --rm -i -v $SRCVOL:/src $IMG sh -c "tar -xmf - -C /src && printf %s '$SYNCID' > /src/.pm_manifest_id"`
Add this comment line above line 231 (same indent, 4 spaces):
`    # STALEOBJ: -m stamps each copied file with the copy time, so it is newer than every object in the build volume (a worktree build without PM_LANE wrote older sources there).`
## Facts for the edits
- The image is devkitpro/devkitarm (Debian, docker/Dockerfile.build:2): GNU tar, `-m` = `--touch` (do not restore the mtime).
- No other reader of the sync id: grep `src_volid|manifest_id|vol_match` in 3ds/ finds only build.sh:186-219. bench.py reads build_key.txt only.
- The key (build.sh:153-161) does not change. vol_match is false once per folder, so the first build after the merge copies all files.
