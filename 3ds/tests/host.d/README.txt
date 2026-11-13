Per-row host tests. nd_host_test.sh runs every *.sh here in name order inside its docker block (cwd /work, set -e, gcc -m32).
A row adds its own <row>.sh. A script prints its result line and exits non-zero on a failure. Never edit another row's script.
Flags for game headers: -DF3DEX_GBI_2 -D_LANGUAGE_C -DTARGET_3DS -DVERSION_US -D_MIPS_SZLONG=32 -I include -I ver/us/build/include -I 3ds/include -I 3ds/stub_include
