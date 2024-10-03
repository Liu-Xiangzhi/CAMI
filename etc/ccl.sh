#!/bin/bash
# count code line of this project
cd $(dirname $(readlink -f "$0"))/..
cloc --vcs=git --read-lang-def=etc/cloc_conf.txt "$@"
