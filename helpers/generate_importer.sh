#!/usr/bin/env bash
# Create a neo4j importer for files in \$import_dir, defined in
# env.sh.

usage() {
    cat <<-_EOF_
Useage: $(basename $0) OPTION...
Generate an importer for creating a neo4j DB with graphs produced by pubmedparser.

Importer is sent to stdout so should be redirected to the desired file.

    -s, --source          source directory containing the graphs (should be destination of convert2graph.sh).
    -f, --structure-file  a yaml file with the xml paths to collect (same as used by read_xml).
    -n, --db-name         the name of the neo4j DB (defaults to neo4j).
    -h, --help            show this help.
_EOF_
}

bin_dir=$(dirname $0)/../bin
components() {
    local name=$1
    PATH="$bin_dir:$PATH" yaml_get_key_component \
        --structure-file=$structure_file $name
}

src_dir=$PWD
structure_file=structure.yml
db_name=neo4j
clean_flag=false

for arg in "$@"; do
    shift
    case "$arg" in
    '--source') set -- "$@" '-s' ;;
    '--structure-file') set -- "$@" '-f' ;;
    '--db-name') set -- "$@" '-n' ;;
    '--help') set -- "$@" '-h' ;;
    *) set -- "$@" "$arg" ;;
    esac
done

while getopts 's:f:n:ch' arg; do
    case "$arg" in
    s) src_dir="$OPTARG" ;;
    f) structure_file="$OPTARG" ;;
    n) db_name="$OPTARG" ;;
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

cat <<_EOF_
#!/usr/bin/env bash

import_dir=$src_dir
name=$db_name

neo4j-admin import \\
    --database=\$name \\
    --delimiter="\\t" \\
    --quote="\\"" \\
    --skip-bad-relationships=true \\
    --trim-strings=true \\
    --id-type=STRING \\
_EOF_

read -d ":" key <<<$(components key)
while IFS=': ' read node value; do
    echo "    --nodes=${node}=\$import_dir/${node}_nodes.tsv \\"
    echo "    --relationships=${key}-${node}=\$import_dir/${key}_${node}_edges.tsv \\"
done <<<"$(components nodes)"

while IFS=': ' read edge value; do
    echo "    --relationships=${key}-${edge}=\$import_dir/${key}_${edge}_edges.tsv \\"
done <<<"$(components edges)"

echo "    --nodes=${key}=\$import_dir/${key}_nodes.tsv"
