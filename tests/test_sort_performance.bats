#! /usr/bin/env bats

setup() {
    load 'bats_helpers'
    _common_setup
}

setup_file() {
    load 'bats_helpers'
    _common_file_setup

    export nthreads=4
    export data_files=("$HOME/data/pubmed/pubmed21n00**.xml.gz")

    if ![[ -d $cache_dir ]]; then
	OMP_NUM_THREADS=nthreads read_xml --cache-dir=$cache_dir \
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
    { time sort -k1 $cache_dir/Author_*.tsv > $tmp_dir/Author.tsv; } 2>&3
}

cat_sort() {
    cat $cache_dir/Author_*.tsv > $tmp_dir/Author.tsv
    sort -k1 $tmp_dir/Author.tsv > $tmp_dir/tmp.tsv && \
	mv $tmp_dir/tmp.tsv $tmp_dir/Author.tsv
}

@test "Performance concat then sort" {
    { time cat_sort; } 2>&3
}

merge_sort_1() {
    for f in $cache_dir/Author_*.tsv; do
	sort -k1 $f > $tmp_dir/$(basename $f) &
    done
    wait

    sort -m $tmp_dir/Author_*.tsv > $tmp_dir/Author.tsv
    rm $tmp_dir/Author_*.tsv
}

@test "Performance & sort and merge" {
    { time merge_sort_1; } 2>&3
}

merge_sort_2() {
    seq -f "Author_%g.tsv" 0 $((nthreads - 1)) | \
	xargs -I {} --max-procs=$nthreads --max-args=1 \
	sh -c 'sort -k1 $cache_dir/$1 > $tmp_dir/$1' sh {}

    sort -m $tmp_dir/Author_*.tsv > $tmp_dir/Author.tsv
    rm $tmp_dir/Author_*.tsv
}

@test "Performance xargs sort and merge" {
    { time merge_sort_2; } 2>&3
}
