#!/bin/bash
file_directory=$(dirname $(readlink -f "$0"))
version="$1"

if [ -z "$version" ]; then
    echo 'option "version" missing, default to "C23"'
    version="C23"
fi

cd $file_directory/$version

cat ./lexical.ebnf ./phrase_structure.ebnf ./preprocessing_directives.ebnf ./floating-point_subject_sequence.ebnf > ./sylloge.ebnf