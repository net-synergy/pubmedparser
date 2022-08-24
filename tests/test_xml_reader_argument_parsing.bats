#! /usr/bin/env bats

setup() {
    load 'bats_helpers'
    _common_setup
}

setup_file() {
    load 'bats_helpers'
    _common_file_setup

    export data_dir=$BATS_DIR/../data
}

teardown_file() {
    load 'bats_helpers'
    _common_file_teardown
}

@test "Test passing xml file" {
    read_xml --cache-dir=$cache_dir \
        --structure-file=$structure_file \
        $data_dir/pubmed21n1000.xml.gz
}

@test "Test passing multiple files using globs" {
    read_xml --cache-dir=$cache_dir \
        --structure-file=$structure_file \
        $data_dir/*.xml.gz
}

@test "Test xml from stdin" {
    read_xml --cache-dir=$cache_dir \
        --structure-file=$structure_file \
        <$data_dir/pubmed21n1000.xml.gz
}

@test "Test can use minimal structure file" {
    minimal_structure_file=$BATS_DIR/data/test_minimal_structure.yml
    read_xml --cache-dir=$cache_dir \
        --structure-file=$minimal_structure_file \
        $data_dir/*.xml.gz
}
