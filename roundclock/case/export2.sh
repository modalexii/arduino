#!/bin/bash
# export2.sh TAG PART CUT "extra -D args"   (C2 assembly steps)
t=$1; p=$2; c=$3; shift 3
openscad -o stl/${t}_${p}_${c}.stl -D 'opt="C2"' -D "part=\"$p\"" -D "cut=$c" "$@" amoled_case_concepts.scad >/dev/null 2>stl/${t}_${p}_${c}.log || echo "FAIL $t $p $c"
