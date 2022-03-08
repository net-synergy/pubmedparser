#! /usr/bin/env bash
# If read_xml is run with multiple files it will use multiple threads
# and each thread will print results to it's own file. This will
# concatenate all the threads files into a single master file for each
# node.

cache_dir="$1";

if ! [[ -d $cache_dir ]]; then
   >&2 echo "Error: cache directory \"$cache_dir\" does not exist."
   exit 1
fi

master_files=$(ls $cache_dir/*.tsv | sed -n '/_[0-9]*\.tsv/!p')
for f in $master_files; do
    cat ${f%%\.*}_*.tsv > ${f%%\.*}.tsv
    rm ${f%%\.*}_*.tsv
done
