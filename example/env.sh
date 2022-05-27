#! /usr/bin/env bash
# Setup variables for file paths and running code related to
# generating neo4j db.

export top_dir=../$(dirname $0)
export bin_dir=$top_dir/bin
export cache_dir=~/Documents/david/data/synergy/cache
export data_dir=~/Documents/david/data/pubmed
export structure_file=$top_dir/example/structure.yml
export import_dir=~/Documents/david/data/synergy/import
export delete_cache=true # If true clear cache.
export nthreads=64

components() {
    local name=$1
    PATH="$bin_dir:$PATH" yaml_get_key_component \
        --structure-file=$structure_file $name
}

key_value=$(components key)
export key=${key_value%%:*}
