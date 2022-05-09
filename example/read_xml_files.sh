#! /usr/bin/env bash
# Read all files in \$data_dir using READ_XML.

source ./env.sh

$delete_cache && \
    [ -d $cache_dir ] && \
    rm -r $cache_dir

[ -d $import_dir ] && rm -r $import_dir
mkdir -p $import_dir

files="$data_dir/*.xml.gz"

# Assuming the executables are in this directory and not installed globally.
echo "Reading XML files..."

PATH="$bin_dir:$PATH" OMP_NUM_THREADS="$nthreads" read_xml \
    --structure-file=$structure_file \
    --cache-dir=$cache_dir \
    $files

echo "Finished reading XML files."

$top_dir/cat_results.sh $cache_dir
