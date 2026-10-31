#!/bin/bash
# export_final.sh TAG PART CUT [extra -D args]  -> stl/TAG_PART_CUT.stl (assembly mode, for renders)
t=$1; p=$2; c=$3; shift 3
openscad -o stl/${t}_${p}_${c}.stl -D 'mode="assembly"' -D "part=\"$p\"" -D "cut=$c" "$@" clock_case.scad >/dev/null 2>stl/${t}_${p}_${c}.log || true
