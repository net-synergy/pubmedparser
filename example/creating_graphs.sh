top_dir=$(dirname $0)/..
cd $top_dir

xml_dir=data
cache_dir=cache
import_dir=import
structure_file=example/structure.yml

[ -d $cache_dir ] && rm -r $cache_dir
[ -d $import_dir ] && rm -r $import_dir
[ -d $xml_dir ] && rm -r $xml_dir

helpers/download_pubmed_data --destination $xml_dir {0001..0003}
result/bin/read_xml --structure-file=$structure_file --cache=$cache_dir $xml_dir/*

helpers/convert2graph.sh \
    --structure-file $structure_file \
    --source $cache_dir \
    --destination $import_dir

helpers/generate_importer.sh \
    --structure-file $structure_file \
    --import-dir $import_dir >importpubmed.sh
chmod +x importpubmed.sh

./importpubmed.sh
