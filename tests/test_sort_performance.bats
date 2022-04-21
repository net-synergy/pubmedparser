#! /usr/bin/env bats

setup() {
    load 'bats_helpers'
    _common_setup
}

setup_file() {
    load 'bats_helpers'
    _common_file_setup

    export data_files=("$HOME/data/pubmed/pubmed21n00**.xml.gz")

    if ![[ -d $cache_dir ]]; then
	read_xml --cache-dir=$cache_dir \
	    --structure-file=$structure_file \
	    $data_files
    fi

    # By using a seperate tmp_dir should not have to delete cache and
    # rerun reader for every test.
    export tmp_dir="$BATS_DIR/tmp"
    mkdir -p $tmp_dir
}

teardown() {
    rm -rf $tmp_dir
    mkdir -p $tmp_dir
}

@test "Performance regular sort" {
    { time sort $cache_dir/Author_*.tsv > $tmp_dir/author.tsv; } 2>&3
}

merge_sort_1() {
    for f in $cache_dir/Author_*.tsv; do
	sort $f > $tmp_dir/$(basename $f) &
    done
    wait

    sort -m $tmp_dir/Author_*.tsv > $tmp_dir/author.tsv
}

@test "Performance & sort and merge" {
    { time merge_sort_1; } 2>&3
}
