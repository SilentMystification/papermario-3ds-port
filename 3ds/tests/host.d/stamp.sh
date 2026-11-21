#!/bin/sh
# MTX1 (fixed /tmp paths: the unit runs in the container, HOSTTMP): pm_stamp.h (the reader stamp rule of nd_wq.c). The seeded fault must print its exact line.
mkdir -p /tmp/stamp_seed
seed() {
    sed "$1" 3ds/include/pm_stamp.h > /tmp/stamp_seed/pm_stamp.h
    gcc -m32 -Wall -Wextra -I /tmp/stamp_seed -o /tmp/stamp_seed/t 3ds/tools/pm_stamp_test.c || exit 1
    got=$(/tmp/stamp_seed/t || true)
    if [ "$got" != "$2" ]; then echo "stamp seed FAIL: [$1] got [$got]"; exit 1; fi
}
seed 's/>= 0/> 0/' 'stamp FAIL 0'
gcc -m32 -Wall -Wextra -I 3ds/include -o /tmp/stamp_t 3ds/tools/pm_stamp_test.c && /tmp/stamp_t || exit 1
