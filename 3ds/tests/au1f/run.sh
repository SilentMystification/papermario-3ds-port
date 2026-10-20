#!/bin/sh
# AU1f host test. Run from the repo root. Compiles the new file and the old copy (functions renamed ref_*) for x86 without SSE4.1
# (so the plain C path, the one the 3DS uses, is compared) and runs random input through every op.
set -e
cd "$(dirname "$0")/../../.."
D=3ds/tests/au1f; O=${TMPDIR:-/tmp}/au1f_test; mkdir -p $O
INC="-D_LANGUAGE_C -I3ds/include -I3ds/stub_include -I include"
REN=""
for f in aLoadBufferImpl aSaveBufferImpl aLoadADPCMImpl aSetLoopImpl aADPCMdecImpl aResampleImpl aMixImpl aEnvMixerImpl aSetVolumeImpl aPoleFilterImpl aInterleaveImpl aClearBufferImpl aDMEMMoveImpl aDisableImpl n_aspMainDataStart n_aspMainTextStart; do REN="$REN -D$f=ref_$f"; done
gcc -O2 -w -fsigned-char -fno-strict-aliasing -fwrapv $INC $REN -DPM_AU_REF -c $D/pm_rsp_audio_ref.c -o $O/ref.o
gcc -O2 -w -fsigned-char -fno-strict-aliasing -fwrapv $INC -c 3ds/src/pm_rsp_audio.c -o $O/new.o
gcc -O2 -w $INC -c $D/test_au1f.c -o $O/t.o
gcc $O/t.o $O/ref.o $O/new.o -o $O/t
$O/t
