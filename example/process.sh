#! /usr/bin/env bash
# Processes Pubmed XML files with read_xml then converts those into
# node and edge files for use with neo4j.

top_dir=../$(dirname $0)
bin_dir=$top_dir/bin
cache_dir=~/Documents/david/data/synergy/cache
data_dir=~/Documents/david/data/pubmed
structure_file=$top_dir/example/structure.yml
import_dir=~/Documents/david/data/synergy/import
delete_cache=false # If true clear cache.
nthreads=64

$delete_cache && \
    [ -d $cache_dir ] && \
    rm -r $cache_dir

[ -d $import_dir ] && rm -r $import_dir
mkdir -p $import_dir

if [[ -f $cache_dir/processed.txt ]]; then
    files=$(cat $cache_dir/processed.txt <(ls "$data_dir/*.xml.gz") \
        | sort | uniq -u)
else
    files="$data_dir/*.xml.gz"
fi

# Assuming the executables are in this directory and not installed globally.
echo "Reading XML files..."

PATH="$bin_dir:$PATH" OMP_NUM_THREADS="$nthreads" read_xml \
    --structure-file=$structure_file \
    --cache-dir=$cache_dir \
    $files

echo "Finished reading XML files."

$top_dir/cat_results.sh $cache_dir

components() {
    local name=$1
    PATH="$bin_dir:$PATH" yaml_get_key_component \
        --structure-file=$structure_file $name
}

tabsep="=+=t=+=" # Key to keep non-id columns together
spcsep="=+s+="

gen_node() {
    local key=$1

    key_file=$cache_dir/$key.tsv
    paste <(cut -f1 $key_file) \
        <(cut -f1 --complement $key_file | \
        sed -e "s/\\t/$tabsep/g" -e  "s/\\s/$spcsep/g") | \
        sort -k 2 > \
        tmp_${key} && mv tmp_${key} $key_file

    cut -f1 --complement $key_file | sort -u | \
        cat -n | sed 's/^\s*//' > $import_dir/${key}_nodes.tsv
}

echo "Generating node files..."

while IFS=': ' read key value; do
    [[ $key == "Reference" ]] && continue
    gen_node $key &
done <<< "$(components nodes)"
wait

key_value=$(components key)
key=${key_value%%:*}
cat <(cut -f1 $cache_dir/$key.tsv) <(cut -f2 $cache_dir/Reference.tsv) \
    | sort -u | cat -n | sed 's/^\s*//' > $import_dir/${key}_nodes.tsv

echo "Finished generating node files."

echo "Generating edge files..."

while IFS=': ' read node value; do
    [[ $node == "Reference" ]] && continue
    join -j 2 $cache_dir/${node}.tsv $import_dir/${node}_nodes.tsv | \
        sort -k 2b,2 > $cache_dir/${node}_tmp.tsv &
done <<< "$(components nodes)"
wait

while IFS=': ' read node value; do
    [[ $node == "Reference" ]] && continue
    join -j 2 $cache_dir/${node}_tmp.tsv $import_dir/${key}_nodes.tsv | \
        awk '{ print $4,"\t",$3 }'> \
        $import_dir/${key}_${node}_edges.tsv &
done <<< "$(components nodes)"
wait

paste <(join -1 2 -2 1 $import_dir/${key}_nodes.tsv <(sort -k 1 $cache_dir/Reference.tsv) | cut -d" " -f2) \
    <(join -j 2 $import_dir/${key}_nodes.tsv <(sort -k 2 $cache_dir/Reference.tsv) | cut -d" " -f2) > \
    $import_dir/${key}_${key}_edges.tsv

awk '{ print $2,$1 }' < $import_dir/${key}_nodes.tsv > tmp && \
    mv tmp $import_dir/${key}_nodes.tsv
while IFS=': ' read key_feature value; do
    join -j 1 $import_dir/${key}_nodes.tsv \
        <(sort -k 1b,1 $cache_dir/${key_feature}.tsv) > tmp \
        && mv tmp $import_dir/${key}_nodes.tsv
done <<< "$(components key_features)"
sed 's/\s/\t/g' < $import_dir/${key}_nodes.tsv | cut -f 2- > tmp && \
    mv tmp $import_dir/${key}_nodes.tsv

echo "Finished generating edge files."

while IFS=': ' read node value; do
    sed -e 's/ /\t/g' -e "s/$tabsep/\t/g" -e "s/$spcsep/ /g" < $import_dir/${node}_nodes.tsv > tmp_${node} && \
        mv tmp_${node} $import_dir/${node}_nodes.tsv &
done <<< "$(components nodes)"
wait

key_value=$(components key)
key=${key_value%%:*}
header="${key}Id:ID($key)"
while IFS=': ' read node value; do
    header="${header}\t${node}"
done <<< "$(components key_features)"

cat <(echo -e $header) $import_dir/${key}_nodes.tsv > \
    tmp && mv tmp $import_dir/${key}_nodes.tsv

while IFS=': ' read node value; do
    [[ $node == "Reference" ]] && continue
    header="${node}Id:ID(${node})"
    IFS=','; for v in $value; do
        header="${header}\t${v}"
    done

    cat <(echo -e $header) $import_dir/${node}_nodes.tsv > \
        tmp && mv tmp $import_dir/${node}_nodes.tsv
done <<< "$(components nodes)"

while IFS=': ' read node value; do
    [[ $node == "Reference" ]] && continue
    header=":START_ID(${key})"
    header="${header}\t:END_ID(${node})"

    cat <(echo -e $header) $import_dir/${key}_${node}_edges.tsv > \
        tmp && mv tmp $import_dir/${key}_${node}_edges.tsv
done <<< "$(components nodes)"

header=":START_ID($key)\t:END_ID($key)"
cat <(echo -e $header) $import_dir/${key}_${key}_edges.tsv > \
    tmp && mv tmp $import_dir/${key}_${key}_edges.tsv

echo "Calculating overlap between publications..."

while IFS=': ' read node value; do
    [[ $node == "Reference" ]] && node=$key
    echo -e ":START_ID(${key})\t:END_ID(${key})\tweight" > \
        $import_dir/${key}_${node}_overlap.tsv

    PATH="$bin_dir:$PATH" overlap $import_dir/${key}_${node}_edges.tsv >> \
        $import_dir/${key}_${node}_overlap.tsv &
done <<< "$(components nodes)"
wait

echo "Finished calculating overlap."
