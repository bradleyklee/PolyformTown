#!/usr/bin/env bash
set +e

i=1
fails=0
check_eq() {
  if [[ "$1" != "$2" ]]; then
    echo "FAIL [$i]: expected $2 got $1"
    fails=$((fails+1))
  else
    echo "PASS [$i]"
  fi
  i=$((i+1))
}

get_count() {
  tail -n1 | awk '{print $2}' | sed 's/count=//'
}

# ---- POLY COUNT ----
check_eq "$(./poly_count 10 tiles/monomino.tile | get_count)" "4655"
check_eq "$(./poly_count 5  tiles/domino.tile   | get_count)" "2227"
check_eq "$(./poly_count 4  tiles/chair.tile    | get_count)" "7142"
check_eq "$(./poly_count 12 tiles/triangle.tile | get_count)" "3334"
check_eq "$(./poly_count 8  tiles/hexagon.tile  | get_count)" "1448"
check_eq "$(./poly_count 4  tiles/hh.tile       | get_count)" "1552"
check_eq "$(./poly_count 9  tiles/kite.tile     | get_count)" "2917"
check_eq "$(./poly_count 3  tiles/hat.tile      | get_count)" "459"

# ---- POLY PRINT (holes only) ----
check_eq "$(./poly_print 10 tiles/monomino.tile | grep '^\[ *1' | wc -l)" "195"
check_eq "$(./poly_print 5  tiles/domino.tile   | grep '^\[ *1' | wc -l)" "69"
check_eq "$(./poly_print 4  tiles/chair.tile    | grep '^\[ *1' | wc -l)" "634"
check_eq "$(./poly_print 12 tiles/triangle.tile | grep '^\[ *1' | wc -l)" "108"
check_eq "$(./poly_print 8  tiles/hexagon.tile  | grep '^\[ *1' | wc -l)" "13"
check_eq "$(./poly_print 4  tiles/hh.tile       | grep '^\[ *1' | wc -l)" "54"
check_eq "$(./poly_print 9  tiles/kite.tile     | grep '^\[ *1' | wc -l)" "141"
check_eq "$(./poly_print 3  tiles/hat.tile      | grep '^\[ *1' | wc -l)" "94"

# ---- POLY PRINT (hole removal) ----
check_eq "$(./poly_print 3 tiles/chair.tile --live-only | wc -l)" "246"
check_eq "$(./poly_print 2 tiles/tetL.tile  --live-only | wc -l)" "63"
check_eq "$(./poly_print 2 tiles/hat.tile   --live-only | wc -l)" "16"

# ---- VCOMP COUNT ----
check_eq "$(./vcomp_count 6 tiles/monomino.tile | get_count)" "30"
check_eq "$(./vcomp_count 3 tiles/domino.tile   | get_count)" "38"
check_eq "$(./vcomp_count 2 tiles/chair.tile    | get_count)" "46"
check_eq "$(./vcomp_count 5 tiles/triangle.tile | get_count)" "21"
check_eq "$(./vcomp_count 7 tiles/hexagon.tile  | get_count)" "13"
check_eq "$(./vcomp_count 1 tiles/hh.tile       | get_count)" "27"
check_eq "$(./vcomp_count 6 tiles/kite.tile     | get_count)" "85"
check_eq "$(./vcomp_count 4 tiles/hat.tile      | get_count)" "36"

# ---- VCOMP PRINT (holes only) ----
check_eq "$(./vcomp_print 6 tiles/monomino.tile | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "0"
check_eq "$(./vcomp_print 3 tiles/domino.tile   | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "0"
check_eq "$(./vcomp_print 2 tiles/chair.tile    | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "0"
check_eq "$(./vcomp_print 5 tiles/triangle.tile | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "0"
check_eq "$(./vcomp_print 7 tiles/hexagon.tile  | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "0"
check_eq "$(./vcomp_print 1 tiles/hh.tile       | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "0"
check_eq "$(./vcomp_print 6 tiles/kite.tile     | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "0"
check_eq "$(./vcomp_print 4 tiles/hat.tile      | awk '/^Aggregate$/{getline;print}' | grep '^\[ *1' | wc -l)" "11"




if [[ "$fails" -gt 0 ]]; then
  echo "TOTAL FAILS: $fails"
  exit 1
fi
echo 0
