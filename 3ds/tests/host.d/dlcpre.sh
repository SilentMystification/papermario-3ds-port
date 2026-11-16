#!/bin/sh
# MAPLAG (b): decode_yay0 calls pm_dlc_preload after the shape invalidation and the drain; dlc_reset keeps the tables of the current gen.
# Each seeded fault must print its exact line. The tr and awk strip CR (autocrlf checkout).
mkdir -p build3ds/out/dlcpre_seed
chk() {
    awk '{ sub(/\r$/, "") } /pm_tex_invalidate\(dst, dec\);/ { t = NR } /DLC1: the shape file is final now/ { g = NR } /pm_walk_drain\("dlcpre"\); pm_dlc_preload\(\);/ { p = NR } END { if (!t || !g || !p || p < t || p < g) { print "dlcpre FAIL order"; exit 1 } }' "$1" || return 1
    tr -d '\r' < "$2" | grep -qF 'if (seam_gen != pm_dlc_gen || !seam_ok) seam_reset();' || { echo "dlcpre FAIL reset seam"; return 1; }
    tr -d '\r' < "$2" | grep -qF 'if (dlcp_gen != pm_dlc_gen || !dlcp_f) dlcp_close();' || { echo "dlcpre FAIL reset dlcp"; return 1; }
    tr -d '\r' < "$2" | grep -qF 'void pm_dlc_preload(void) {' || { echo "dlcpre FAIL no preload"; return 1; }
    echo "dlcpre ok"
}
seed() { got=$(chk "$1" "$2" || true); if [ "$got" != "$3" ]; then echo "dlcpre seed FAIL: [$3] got [$got]"; exit 1; fi; }
sed '/pm_walk_drain("dlcpre"); pm_dlc_preload();/d' 3ds/src/pm_map.c > build3ds/out/dlcpre_seed/pm_map.c
seed build3ds/out/dlcpre_seed/pm_map.c 3ds/src/nd_shape.c "dlcpre FAIL order"
sed 's/if (seam_gen != pm_dlc_gen || !seam_ok) seam_reset();/seam_reset();/' 3ds/src/nd_shape.c > build3ds/out/dlcpre_seed/nd_shape.c
seed 3ds/src/pm_map.c build3ds/out/dlcpre_seed/nd_shape.c "dlcpre FAIL reset seam"
sed 's/if (dlcp_gen != pm_dlc_gen || !dlcp_f) dlcp_close();/dlcp_close();/' 3ds/src/nd_shape.c > build3ds/out/dlcpre_seed/nd_shape.c
seed 3ds/src/pm_map.c build3ds/out/dlcpre_seed/nd_shape.c "dlcpre FAIL reset dlcp"
chk 3ds/src/pm_map.c 3ds/src/nd_shape.c || exit 1
