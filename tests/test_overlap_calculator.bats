#! /usr/bin/env bats

setup() {
    load 'bats_helpers'
    _common_setup
}

setup_file() {
    load 'bats_helpers'
    _common_file_setup

    cat <<-EOF > $cache_dir/normal_edges.tsv
NODE1	NODE2
1	1
1	2
1	3
2	1
2	2
3	1
3	3
4	3
EOF

    cat <<-EOF > $cache_dir/disordered_edges.tsv
NODE1	NODE2
2	1
2	2
1	1
1	2
1	3
3	1
3	3
4	3
EOF
}

teardown_file() {
    load 'bats_helpers'
    _common_file_teardown
}

@test "Test overlap calculator" {
    overlap $cache_dir/normal_edges.tsv > $cache_dir/overlap.tsv
    diff $cache_dir/overlap.tsv <(cat<<EOF
1	2	2
1	3	2
1	4	1
2	3	1
3	4	1
EOF
				 )
}

@test "Test unsorted primary column causes error" {
    run overlap $cache_dir/disordered_edges.tsv

    [ "$status" -eq 1 ]
    [ "$output" = "Error: primary column is not sorted.
	First unsorted edge on line 2." ]
}

@test "Test setting primary column" {
    head -n1 $cache_dir/normal_edges.tsv > $cache_dir/overlap_resorted.tsv
    tail -n+2 $cache_dir/normal_edges.tsv | sort -k2 >> $cache_dir/overlap_resorted.tsv

    overlap -k2 $cache_dir/overlap_resorted.tsv > $cache_dir/overlap.tsv

    diff $cache_dir/overlap.tsv <(cat<<EOF
1	2	2
1	3	2
2	3	1
EOF
				 )
}

@test "Test setting delimiter" {
    tr '\t' ',' < $cache_dir/normal_edges.tsv > $cache_dir/normal_edges.csv
    overlap $cache_dir/normal_edges.tsv > $cache_dir/overlap.tsv
    overlap -d, $cache_dir/normal_edges.csv > $cache_dir/overlap_comma.tsv

    diff $cache_dir/overlap.tsv $cache_dir/overlap_comma.tsv
}
