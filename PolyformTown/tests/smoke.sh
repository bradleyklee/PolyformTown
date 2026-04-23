#!/usr/bin/env bash
set -euo pipefail

check_eq() {
    local got="$1"
    local want="$2"
    local label="$3"
    if [[ "$got" != "$want" ]]; then
        echo "FAIL: $label" >&2
        echo "  got:  $got" >&2
        echo "  want: $want" >&2
        exit 1
    fi
}

check_contains() {
    local hay="$1"
    local needle="$2"
    local label="$3"
    if [[ "$hay" != *"$needle"* ]]; then
        echo "FAIL: $label" >&2
        echo "  missing: $needle" >&2
        exit 1
    fi
}

check_eq "$(./poly_count 4 tiles/monomino.tile | tail -n 1)" "n=4 count=5" "monomino count"
check_eq "$(./poly_count 4 tiles/domino.tile | tail -n 1)" "n=4 count=211" "domino count"
check_eq "$(./poly_count 4 tiles/chair.tile | tail -n 1)" "n=4 count=7142" "chair count"
check_eq "$(./poly_count 4 tiles/triangle.tile | tail -n 1)" "n=4 count=3" "triangle count"
check_eq "$(./poly_count 4 tests/legacy_monomino.tile | tail -n 1)" "n=4 count=5" "legacy monomino count" 

check_eq "$(./poly_count 8 tiles/kite.tile | tail -n 1)" "n=8 count=873" "kite count n=8"

line="$(./tile_to_imgtable tiles/triangle.tile)"
check_contains "$line" "[ 0 | (r=sqrt(3)) | (6:1,0;1/2,r/2) | (6 0 0,6 1 0,6 0 1) ]" "triangle conversion"

./tile_to_imgtable tiles/triangle.tile | ./imgtable > /tmp/triangle.svg
check_contains "$(cat /tmp/triangle.svg)" "<svg" "triangle svg header"
check_contains "$(cat /tmp/triangle.svg)" "<path" "triangle svg path"

./tile_to_imgtable tiles/kite.tile | ./imgtable > /tmp/kite.svg
check_contains "$(cat /tmp/kite.svg)" "<svg" "kite svg header"
check_contains "$(cat /tmp/kite.svg)" "<path" "kite svg path"


line="$(./poly_print 1 tiles/triangle.tile | head -n 1)"
check_contains "$line" "[ 0 | (r=sqrt(3)) | (6:1,0;1/2,r/2) | (6 0 0,6 1 0,6 0 1) ]" "poly_print format"

line="$(./vcomp_print 0 tiles/triangle.tile | head -n 1)"
check_contains "$line" "[ 0 | (r=sqrt(3)) | (6:1,0;1/2,r/2) | (6 0 0,6 1 0,6 0 1) ]" "vcomp_print format"


echo 0
