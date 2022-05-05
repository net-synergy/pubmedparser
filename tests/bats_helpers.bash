#! /usr/bin/env bash

_common_file_setup() {
    export BATS_DIR=$(dirname $BATS_TEST_FILENAME)
    export PATH="$BATS_DIR/../bin:$PATH"
    export cache_dir=$BATS_DIR/cache
    export structure_file=$BATS_DIR/data/test_xml_reader_structure.yml
    export import_dir=$HOME/data/synergy/import
    export OMP_NUM_THREADS=4
    mkdir -p $cache_dir
}

_common_setup() {
    cd $BATS_DIR
}

_common_file_teardown() {
    rm -r $cache_dir
}
