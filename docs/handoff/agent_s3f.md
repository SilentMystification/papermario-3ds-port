# Agent S3F: handoff

## STATE
branch: nds1 | commits: eeba51266 (S3F-1a matrix record), dfbf5d131 (S3F-1b lights record)
done: S3F-1a, S3F-1b (host PASS, mdldiff 0 diffs, seam frame 200 identical or 4 px; frames 201, 202 about 1 percent diff, world right by eye, passed by rule)
open: S3F-2 (widen recOn to 2CYC, fog, shroud, depth, remap, all modes), S3F-3 (floor reflection, custom gfx ranges), S3F-4 (delete walker model path, grep in done criteria not empty yet)
not checked: matrix stack top is not in TS_FIELDS (mdldiff does not compare it); picture covers it
model classes the record cannot carry yet: panner, billboard, custom gfx, local vertex copy, transform group member (matrix path differs: recOn still excludes billboard/panner/custom/local copy via plain)
Azahar starts on S3F: about 6 (mdldiff x3, seam x4 incl. 1 failed write)
next: S3F-2. Start at recOn = plain && ... in src/model.c, then mdldiff, seam_pic.
