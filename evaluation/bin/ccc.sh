#!/bin/bash
# a fake 'Cami C Compiler', just forward a precompiled product to cami
input=$1
if [ -z "$input" ]; then
    echo -e "missing input\n\t usage: $0 <input_name>"
    exit 1
fi
if [ -z "$test_suite_dir" ]; then
    echo missing env test_suite_dir
    exit 1
fi
if [ -z "$out_dir" ]; then
    echo missing env out_dir
    exit 1
fi

exec cami run $(echo "$input" | sed "s#$test_suite_dir/\\(.*\\)\\.c#$out_dir/cami/\\1.tbc#")
