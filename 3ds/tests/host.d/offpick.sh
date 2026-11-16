#!/bin/sh
# WAVEWIDE: pm_offt_pick takes the slot with the nearest base. The seed (the old first-match rule) must fail the adjacent frame buffer case.
mkdir -p /tmp/offpick_seed
sed 's/if (best < 0 || d < bd) {/if (best < 0) {/' 3ds/include/pm_tex_offpick.h > /tmp/offpick_seed/pm_tex_offpick.h
gcc -m32 -Wall -Wextra -Werror -I /tmp/offpick_seed -o /tmp/offpick_seed/t 3ds/tools/pm_tex_offpick_test.c || exit 1
got=$(/tmp/offpick_seed/t || true); if [ "$got" != "offpick FAIL adj" ]; then echo "offpick seed FAIL: got [$got]"; exit 1; fi
gcc -m32 -Wall -Wextra -Werror -I 3ds/include -o /tmp/offpick_t 3ds/tools/pm_tex_offpick_test.c && /tmp/offpick_t || exit 1
