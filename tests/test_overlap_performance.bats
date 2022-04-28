#! /usr/bin/env bats

setup() {
    load 'bats_helpers'
    _common_setup

    export node_types=(Author Chemical)
}

setup_file() {
    load 'bats_helpers'
    _common_file_setup
}

run_over_test_dir() {
    for n in "${node_types[@]}"; do
	f="$import_dir/Publication_${n}_edges.tsv"
	overlap $f > /dev/null
    done
}

@test "Performance overlap" {
    { time run_over_test_dir; } 2>&3
}
