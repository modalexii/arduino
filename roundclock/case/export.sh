#!/bin/bash
# export.sh OPT PART CUT
o=$1; p=$2; c=$3
openscad -o stl/${o}_${p}_${c}.stl -D "opt=\"$o\"" -D "part=\"$p\"" -D "cut=$c" amoled_case_concepts.scad >/dev/null 2>stl/${o}_${p}_${c}.log || echo "FAIL $o $p $c"
