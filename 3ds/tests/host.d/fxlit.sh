#!/bin/sh
# FXLIT: PRIMDEPTH is a unit only (nd_gfx_conv.c v2_walk), so these 6 src lists are gset ok. Reads /tmp/gconv of nd_host_test.sh.
# The converter before FXLIT (main with FXOPS) fails on the first name.
for l in D_8014E8F0 D_8014E9A8 kmr_20_records_screen_gfx mgm_00_Gfx_RecordDisplay_Init hos_05_gfx_setup_story_viewport NoControllerGfx; do
  grep -qx "$l" /tmp/gconv/gset_ok.txt || { echo "fxlit FAIL: $l is not gset ok"; exit 1; }
done
echo "fxlit ok"
