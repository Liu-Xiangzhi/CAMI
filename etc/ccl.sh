#!/bin/bash
# count code line of this project
cd $(dirname $(readlink -f "$0"))/..
cloc src include config camic doc evaluation  CMakeLists.txt \
      --fullpath --not-match-d='evaluation/venv|camic/_build' --read-lang-def=etc/cami_bc_cloc_conf.txt "$@"
