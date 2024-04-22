#!/bin/bash
# count code line of this project
cd $(dirname $(readlink -f "$0"))/..
cloc src include config compiler doc evaluation  CMakeLists.txt \
      --fullpath --not-match-d='evaluation/venv' --read-lang-def=etc/cami_bc_cloc_conf.txt "$@"