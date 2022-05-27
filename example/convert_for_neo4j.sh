#!/usr/bin/env bash
# Convert outputs from READ_XML_FILES to node and edge files for
# importing to neo4j.

source ./env.sh

tabsep="=+=t=+=" # Key to keep non-id columns together
spcsep="=+s+="

gen_node() {
    local node=$1

    node_file=$cache_dir/$node.tsv
    paste <(cut -f1 $node_file) \
        <(cut -f1 --complement $node_file | \
        sed -e "s/\\t/$tabsep/g" -e  "s/\\s/$spcsep/g") | \
        sort -k 2 > \
        tmp_${node} && mv tmp_${node} $node_file

    cut -f1 --complement $node_file | sort -u | \
        cat -n | sed 's/^\s*//' > $import_dir/${node}_nodes.tsv
}

echo "Generating node files..."

while IFS=': ' read node value; do
    [[ $node == "Reference" ]] && continue
    gen_node $node &
done <<< "$(components nodes)"
wait

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
    [[ $node == "Reference" ]] && continue
    sed -e 's/ /\t/g' -e "s/$tabsep/\t/g" -e "s/$spcsep/ /g" < $import_dir/${node}_nodes.tsv > tmp_${node} && \
        mv tmp_${node} $import_dir/${node}_nodes.tsv &
done <<< "$(components nodes)"
wait

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
