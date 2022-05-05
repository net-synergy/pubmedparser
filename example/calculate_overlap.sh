#!/usr/bin/env bash
# Calculate overlap using OVERLAP for the provided nodes
# Usage: calculate_overlap node1 node2 ...
# Resulting file names will be generated based on node names.

source env.sh

declare -a overlap_nodes

count=0
while [[ "$#" -gt 0 ]]; do
    overlap_nodes[count] = "$1"
    count=(( $count + 1 ));
    shift
done

echo "Calculating overlap between publications..."

for node in "${overlap_nodes[@]}"; do
    [[ $node == "Reference" ]] && node=$key
    echo -e ":START_ID(${key})\t:END_ID(${key})\tweight" > \
        $import_dir/${key}_${node}_overlap.tsv

    PATH="$bin_dir:$PATH" overlap $import_dir/${key}_${node}_edges.tsv >> \
        $import_dir/${key}_${node}_overlap.tsv

    echo "Finished calculating overlap for ${node}."
done

echo "Finished calculating overlap for all nodes."