#!/usr/bin/env bash
# Create a neo4j importer for files in \$import_dir, defined in
# env.sh.

source ./env.sh

import_file=example/importpubmed.sh
cat > $import_file <<_EOF_
#!/usr/bin/env bash

database_dir=\$XDG_DATA_HOME/neo4j/data
import_dir=$import_dir
name=neo4j

[ -d \$database_dir ] && rm -r \$database_dir

neo4j-admin import \\
    --database=\$name \\
    --delimiter="\\t" \\
    --quote="\\"" \\
    --skip-bad-relationships=true \\
    --trim-strings=true \\
    --id-type=STRING \\
_EOF_

while IFS=': ' read node value; do
    [[ $node == "Reference" ]] && continue
    echo "    --nodes=${node}=\$import_dir/${node}_nodes.tsv \\" >> $import_file
    echo "    --relationships=${key}-${node}=\$import_dir/${key}_${node}_edges.tsv \\" >> $import_file
done <<< "$(components nodes)"

echo "    --nodes=${key}=\$import_dir/${key}_nodes.tsv \\" >> $import_file
echo "    --relationships=${key}-${key}=\$import_dir/${key}_${key}_edges.tsv \\" >> $import_file

chmod +x $import_file
