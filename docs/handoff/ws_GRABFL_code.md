# ws_GRABFL code blocks (copy exactly)

## Block A (pm_gpu.c, Edit 1: above `static int grab_impl(const void* cfb, int y0, int y1, int prev_frame) {`)
```c
/* GRABFL: the grab split queues part 1, and grab_queue_copy can run the queue at once (pm_gx_room, copy_guard). The ring words of part 1 leave the
 * data cache here. gring_fl and gidx_fl stay, so gtrip and the frame end flush see the same state as before: every draw keeps its words. */
static void grab_ring_flush(void) {
    if (nb.gring && nb.gring_n > nb.gring_fl) pm_fl_dc(nb.gring + nb.gring_fl * 16, (nb.gring_n - nb.gring_fl) * 16);
    if (nb.gidx && nb.gidx_n > nb.gidx_fl) pm_fl_dc(nb.gidx + nb.gidx_fl, (nb.gidx_n - nb.gidx_fl) * 2);
}
```

## Block B (pm_gpu.c, Edit 2: the line between `        pm_tex_flush_pend_only();` and `        sh_clear(); ndunit_scan(); rd_sync(); C3D_FrameSplit(...` in grab_impl)
```c
        grab_ring_flush(); /* GRABFL: part 1 can run before the frame end flush */
```

## Block C (3ds/tests/host.d/grabfl.sh, LF endings)
```sh
#!/bin/sh
# GRABFL: each command list split in pm_gpu.c has a ring flush in its function. The seed (the grab_ring_flush call removed) must fail at grab_impl.
cat > /tmp/grabfl.awk <<'EOF'
/^[A-Za-z_].*\) *\{ *(\/\*.*)?$/ { fn = $0; gsub(/__attribute__\(\([^)]*\)\)/, "", fn); sub(/\(.*/, "", fn); n = split(fn, a, /[ *]+/); fn = a[n]; fl = 0 }
/gpu_flush_written\(\)|sr_split\(\)|grab_ring_flush\(\)/ { fl = 1 }
/C3D_FrameSplit\(GX_CMDLIST_FLUSH\)/ { s++; if (!fl) { print "grabfl FAIL " fn; bad = 1 } }
END { if (!bad) print "grabfl ok splits " s; exit bad }
EOF
mkdir -p /tmp/grabfl_seed
sed '/grab_ring_flush();/d' 3ds/src/pm_gpu.c > /tmp/grabfl_seed/pm_gpu.c
got=$(awk -f /tmp/grabfl.awk /tmp/grabfl_seed/pm_gpu.c || true)
if [ "$got" != "grabfl FAIL grab_impl" ]; then echo "grabfl seed FAIL: got [$got]"; exit 1; fi
awk -f /tmp/grabfl.awk 3ds/src/pm_gpu.c || exit 1
```
