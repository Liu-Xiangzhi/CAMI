#!/bin/bash
# forward a precompiled product to cami
input=$1
test_suite_dir=$2
out_dir=$3

if [ -z "$input" ]; then
    echo -e "missing input\n\t usage: $0 <input_name> <test_suit_dir> <out_dir>"
    exit 1
fi
if [ -z "$test_suite_dir" ]; then
    echo -e "missing test_suit_dir\n\t usage: $0 <input_name> <test_suit_dir> <out_dir>"
    exit 1
fi
if [ -z "$out_dir" ]; then
    echo -e "missing out_dir\n\t usage: $0 <input_name> <test_suit_dir> <out_dir>"
    exit 1
fi

exec cami run $(echo "$input" | sed "s#$test_suite_dir/\\(.*\\)\\.c#$out_dir/\\1.tbc#")
