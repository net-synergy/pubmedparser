#!/usr/bin/env bash
#
# Convert outputs from READ_XML_FILES to node and edge files for
# importing to neo4j.
# TODO: add input arguments

file_dir=$(dirname $0)
bin_dir=$file_dir/../bin

usage() {
    cat <<-_EOF_
Useage: $(basename $0) OPTION...
Convert files read with read_xml to graphs.

Graphs are made up of node and edge files.

    -s, --source          source directory, where read_xml's outputs are.
    -d, --destination     where the graphs are written to.
    -f, --structure-file  a yaml file with the xml paths to collect (same as used by read_xml).
    -c, --clean           remove the source directory.
    -h, --help            Show this help.
_EOF_
}

components() {
    local name=$1
    PATH="$bin_dir:$PATH" yaml_get_key_component \
        --structure-file=$structure_file $name
}

## Argument parsing and set up
src_dir=$PWD
dest_dir=$PWD/../graphs
structure_file=structure.yml
clean_flag=false

for arg in "$@"; do
    shift
    case "$arg" in
    '--source') set -- "$@" '-s' ;;
    '--destination') set -- "$@" '-d' ;;
    '--structure-file') set -- "$@" '-f' ;;
    '--clean') set -- "$@" '-c' ;;
    '--help') set -- "$@" '-h' ;;
    *) set -- "$@" "$arg" ;;
    esac
done

while getopts 's:d:f:ch' arg; do
    case "$arg" in
    s) src_dir="$OPTARG" ;;
    d) dest_dir="$OPTARG" ;;
    f) structure_file="$OPTARG" ;;
    c) clean_flag=true ;;
    h)
        usage
        exit 0
        ;;
    ?)
        usage >&2
        exit 1
        ;;
    esac
done

[ -d $dest_dir ] || mkdir -p $dest_dir

key_value=$(components key)
key=${key_value%%:*}

# Seperators to keep non-id columns together, needed because join only
# using one column so to join on multiple columns (like last name and
# first name combined) the columns need to be combined.
tabsep="=+=t=+="
spcsep="=+s+="

while IFS=': ' read node value; do
    {
        f=$src_dir/$node.tsv
        cut -f1 $f >$f.pmid
        paste $f.pmid <(cut --complement -f1 $f |
            sed -e "s/\\t/$tabsep/g" -e "s/\\s/$spcsep/g") |
            sort -k 2 >$f.bak
        rm $f.pmid
        mv $f.bak $f
    } &
done <<<"$(components nodes)"
wait

## Create nodes
echo "Generating node files..."

while IFS=': ' read node value; do
    cat $src_dir/$node.tsv |
        cut --complement -f1 |
        sort -u |
        cat -n |
        sed -e "s/^\\s\+//g" -e "s/\\s\+/\t/g" >$dest_dir/${node}_nodes.tsv &
done <<<"$(components nodes)"
wait

while IFS=': ' read edge value; do
    mv $src_dir/$edge.tsv $dest_dir/${key}_${edge}_edges.tsv
done <<<"$(components edges)"

cp $src_dir/$key.tsv $dest_dir/${key}_nodes.tsv
## NOTE: The key features are posing some problems due to missing
## values. If a publication is missing a year then the join will skip
## that year rather than leaving the Year column blank. Secondly,
## there's the question of how to handle a publication that was
## written in multiple languanges. And these don't seem useful at the
## moment so leaving this until it's needed.
# while IFS=': ' read key_feature value; do
#     paste $dest_dir/${key}_nodes.tsv \
#         <(cut --compliment -f1 $src_dir/$key_feature) >$dest_dir/tmp &&
#         mv tmp $dest_dir/${key}_nodes.tsv
# done <<<"$(components key_features)"

echo "Finished generating node files."

## Create edges
echo "Generating edge files..."

while IFS=': ' read node value; do
    join -j 2 $src_dir/${node}.tsv $dest_dir/${node}_nodes.tsv |
        sort -k2n | # output of join is: node_name key_id node_id
        tr ' ' '\t' |
        cut --complement -f1 >$dest_dir/${key}_${node}_edges.tsv &
done <<<"$(components nodes)"
wait

echo "Finished generating edge files."

if $clean_flag; then
    rm -r $src_dir
fi

## Revert temporary seperators.
while IFS=': ' read node value; do
    {
        f=$dest_dir/${node}_nodes.tsv
        sed -e "s/$tabsep/\t/g" -e "s/$spcsep/ /g" $f >$f.bak
        mv $f.bak $f
    } &
done <<<"$(components nodes)"
wait

## Add neo4j compliant headers
header="${key}Id:ID($key)"
## NOTE: key features are ignored for now, see above.
# while IFS=': ' read key_feature value; do
#     header+="\t${key_feature}"
# done <<<"$(components key_features)"

cat <(echo -e $header) $dest_dir/${key}_nodes.tsv >tmp &&
    mv tmp $dest_dir/${key}_nodes.tsv

while IFS=': ' read node value; do
    header="${node}Id:ID(${node})"
    IFS=','
    for v in $value; do
        header+="\t${v}"
    done

    cat <(echo -e $header) $dest_dir/${node}_nodes.tsv >tmp &&
        mv tmp $dest_dir/${node}_nodes.tsv
done <<<"$(components nodes)"

while IFS=': ' read node value; do
    header=":START_ID(${key})"
    header+="\t:END_ID(${node})"

    cat <(echo -e $header) $dest_dir/${key}_${node}_edges.tsv >tmp &&
        mv tmp $dest_dir/${key}_${node}_edges.tsv
done <<<"$(components nodes)"

while IFS=': ' read edge value; do
    header=":START_ID($key)\t:END_ID($key)"
    cat <(echo -e $header) $dest_dir/${key}_${edge}_edges.tsv >tmp &&
        mv tmp $dest_dir/${key}_${edge}_edges.tsv
done <<<"$(components edges)"
