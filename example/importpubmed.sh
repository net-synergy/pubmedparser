#!/usr/bin/env bash

database_dir=$XDG_DATA_HOME/neo4j/data
import_dir=/home/voidee/data/synergy/import
name=neo4j

[ -d $database_dir ] && rm -r $database_dir

neo4j-admin import \
    --database=$name \
    --delimiter="\t" \
    --quote="\"" \
    --skip-bad-relationships=true \
    --trim-strings=true \
    --id-type=STRING \
    --nodes=Author=$import_dir/Author_nodes.tsv \
    --relationships=Publication-Author=$import_dir/Publication_Author_edges.tsv \
    --relationships=Author-overlap=$import_dir/Publication_Author_overlap.tsv \
    --nodes=Grant=$import_dir/Grant_nodes.tsv \
    --relationships=Publication-Grant=$import_dir/Publication_Grant_edges.tsv \
    --relationships=Grant-overlap=$import_dir/Publication_Grant_overlap.tsv \
    --nodes=Chemical=$import_dir/Chemical_nodes.tsv \
    --relationships=Publication-Chemical=$import_dir/Publication_Chemical_edges.tsv \
    --relationships=Chemical-overlap=$import_dir/Publication_Chemical_overlap.tsv \
    --nodes=Qualifier=$import_dir/Qualifier_nodes.tsv \
    --relationships=Publication-Qualifier=$import_dir/Publication_Qualifier_edges.tsv \
    --relationships=Qualifier-overlap=$import_dir/Publication_Qualifier_overlap.tsv \
    --nodes=Descriptor=$import_dir/Descriptor_nodes.tsv \
    --relationships=Publication-Descriptor=$import_dir/Publication_Descriptor_edges.tsv \
    --relationships=Descriptor-overlap=$import_dir/Publication_Descriptor_overlap.tsv \
    --nodes=Publication=$import_dir/Publication_nodes.tsv \
    --relationships=Publication-Publication=$import_dir/Publication_Publication_edges.tsv \
    --relationships=Publication-overlap=$import_dir/Publication_Publication_overlap.tsv
