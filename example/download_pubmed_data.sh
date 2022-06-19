#!/usr/bin/env bash

base_url="ftp://ftp.ncbi.nlm.nih.gov/pubmed"
name_prefix="pubmed22n"

usage() {
    cat <<-_EOF_
		Useage: $(basename $0) OPTION... FILE[S]
		Downloads publication data files from pubmed and checks against md5s.

		The FILE argument can be the number of a file, a range of file numbers (i.e. {0001..0010}), or file numbers from stdin. Using stdin allows for piping in values obtained by running with the list option or cating a file with a list of files.

		If some desired FILES already exist in the destination directory, they will be skipped.

		  -s, --source       source directory, which pubmed directory to get data from (baseline|updatefiles).
		  -d, --destination  destination directory, where to save files to (defaults to \$PWD).
		  -l, --list         list files in source directory, if source directory is unset, show both.
		  -a, --all          download all files from the source directory.
		  -h, --help         Show this help.

		  Examples:
		      download_pubmed_data.sh 0001
		      download_pubmed_data.sh -d destination {0001..0005}
		      # Will only download new files.
		      download_pubmed_data.sh -d destination -s updatefiles -a
		      # Equivalent to above
		      download_pubmed_data.sh -s updatefiles -l |
		               download_pubmed_data.sh -d destination -s updatefiles
	_EOF_
}

expand_file_names() {
    local template="$1"
    shift
    local variable="$@"

    for v in $variable; do
        echo ${template/'%s'/$v}
    done
}

download_files() {
    local src_dir="$1"
    shift
    local file_names=$(expand_file_names "${name_prefix}%s.xml.gz" "$@")
    local file_urls=$(expand_file_names "${base_url}/${src_dir}/%s" "$file_names")

    wget $file_urls
    wget $(expand_file_names %s.md5 "$file_urls")

    checks=$(md5sum -c *.md5 2>/dev/null)
    echo -e "${checks}" | grep "OK" | grep -o "${name_prefix}[0-9]\+.xml.gz" |
        xargs -I {} rm {}.md5

    echo -e "${checks}" | grep "FAILED" | grep -o "${name_prefix}[0-9]\+.xml.gz" |
        xargs -I {} sh -c "echo {} failed md5sum check, deleting && rm {}* >&2"
}

find_file_numbers() {
    sed -n s/".*${name_prefix}\([0-9]\+\)\.xml\.gz$"/"\1"/p
}

list_files() {
    local src_dir=$1
    curl --silent "${base_url}/${src_dir}/" | find_file_numbers
}

missing_files() {
    local desired_files=("$@")
    local local_files=($(ls $PWD | find_file_numbers))
    local intersect=($(
        echo ${desired_files[@]} ${local_files[@]} |
            tr ' ' '\n' |
            sort |
            uniq -d |
            tr '\n' ' '
    ))
    echo ${desired_files[@]} ${intersect[@]} |
        tr ' ' '\n' |
        sort |
        uniq -u |
        tr '\n' ' '
}

for arg in "$@"; do
    shift
    case "$arg" in
    '--source') set -- "$@" '-s' ;;
    '--destination') set -- "$@" '-d' ;;
    '--list') set -- "$@" '-l' ;;
    '--all') set -- "$@" '-a' ;;
    '--help') set -- "$@" '-h' ;;
    *) set -- "$@" "$arg" ;;
    esac
done

src_dir="baseline"
dest_dir=$PWD
list_flag=false
all_flag=false
while getopts "d:s:lha" arg; do
    case $arg in
    s) src_dir="$OPTARG" ;;
    d) dest_dir="$OPTARG" ;;
    l) list_flag=true ;;
    a) all_flag=true ;;
    h)
        usage
        exit 0
        ;;
    ?)
        usage >&2
        exit 1
        ;;
    esac
done
shift $(($OPTIND - 1))

if $list_flag; then
    list_files $src_dir
    exit 0
fi

if $all_flag; then
    $0 -s $src_dir -l | $0 -s $src_dir -d $dest_dir
    exit 0
fi

declare -a desired_files
if [[ "$#" -eq 0 ]]; then
    i=0
    while read num; do
        desired_files[$i]=$num
        i=$(($i + 1))
    done

    if [[ $i -eq 0 ]]; then
        usage >&2
        exit 1
    fi
else
    desired_files=("$@")
fi

[ -d $dest_dir ] || mkdir -p $dest_dir
cd $dest_dir

files=($(missing_files ${desired_files[@]}))
if [ ${#files[@]} -gt 0 ]; then
    echo "Downloading files..."
    download_files $src_dir "${files[@]}"
    echo "Finished downloading files."
fi
