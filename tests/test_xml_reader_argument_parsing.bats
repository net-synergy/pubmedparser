#! /usr/bin/env bats

setup() {
    DIR=$(dirname $BATS_TEST_FILENAME)
    cd $DIR
    PATH="$DIR/../bin:$PATH"
    cache_dir=$DIR/cache
    structure_file=$DIR/../example/structure.yml
    data_dir=$DIR/../data
    mkdir -p $cache_dir
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
	< $data_dir/pubmed21n1000.xml.gz
}
