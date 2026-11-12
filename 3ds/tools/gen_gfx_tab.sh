#!/bin/sh
# gen_gfx_tab.sh <out.c> [work dir]: the gfx table step (S7CONV, row design note 1). Runs inside docker pm3ds-host:1 with the repo layout of
# nd_host_test.sh at the cwd (3ds, include, src, assets/us, ver/us/build). Scans the converted gfx sources (gen_gfx_syms.py), compiles them
# for the host with the device defines, builds the tool nd_gfx_conv.c with the pure part of nd_scomp.c and writes the tables to <out.c>.
# The work dir keeps gconv_in.o for the host compare (nd_gtab_test.c). GCONV_SRC: the effect_gfx_<g> sources of the set; S7 rows extend it.
set -e
GCONV_SRC="sparkles effect_3D snowfall snaking_static water_block embers"
GCONV_SRC="$GCONV_SRC aura balloon big_smoke_puff big_snowflakes blast breaking_junk bulb_glow butterflies chomp_drop cloud_puff_trail cold_breath confetti damage_indicator" # S7EFX1
GCONV_SRC="$GCONV_SRC damage_stars debuff disable_x drop_leaves dust effect_46 effect_63 effect_65 effect_75 effect_86 emote ending_decals energy_in_out energy_orb_wave" # S7EFX2
GCONV_SRC="$GCONV_SRC energy_shockwave explosion falling_leaves fire_breath fire_flower firework firework_rocket flashing_box_shockwave floating_cloud_puff floating_flower floating_rock flower_splash_trail footprint" # S7EFX3
GCONV_SRC="$GCONV_SRC fright_jar gather_energy_pink gather_magic got_item_outline green_impact hieroglyphs huff_puff_breath ice_pillar ice_shard landing_dust lens_flare light_rays lightning lightning_bolt" # S7EFX4
GCONV_SRC="$GCONV_SRC" # S7EFX5
GCONV_SRC="$GCONV_SRC" # S7EFX6
GCONV_SRC="$GCONV_SRC" # S7EFX7
GCONV_SRC="$GCONV_SRC thunderbolt_ring tubba_heart_attack underwater water_fountain water_splash waterfall whirlwind attack_result_text" # S7EFX8
GCONV_SRC="$GCONV_SRC" # S7ENT
GCONV_SRC="$GCONV_SRC" # S7MAP
GCONV_SRC="$GCONV_SRC" # S7MDL
OUTC=$1
T=${2:-/tmp/gconv}
mkdir -p "$T"
DEF="-DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -DVERSION=us -D_MIPS_SZLONG=32 -Dnullptr=NULL"
TINC="-I 3ds/tools -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include" # the nd_host order
INC="-I 3ds/tools/gconv_inc -I 3ds/tools -I 3ds/include -I 3ds/stub_include -I include -I src -I assets/us -I ver/us/build/include -I ."
python3 3ds/tools/gen_gfx_syms.py "$T/gconv_in.c" $(for g in $GCONV_SRC; do echo "src/effects/gfx/$g.c"; done)
gcc -m32 -w $DEF $INC -c -o "$T/gconv_in.o" "$T/gconv_in.c"
gcc -m32 -Wall -Wextra -Werror -DND_SCOMP_HOST $DEF $TINC -o "$T/nd_gfx_conv" 3ds/tools/nd_gfx_conv.c 3ds/src/nd_scomp.c "$T/gconv_in.o"
"$T/nd_gfx_conv" > "$T/nd_gfx_tab.c"
[ "$T/nd_gfx_tab.c" -ef "$OUTC" ] || cp "$T/nd_gfx_tab.c" "$OUTC"
