#!/usr/bin/env bash

# Example usage:
#     download_pubmed_data.sh 1110
#     download_pubmed_data.sh {1110..1114}
#
# Find pubmed files:
#     ftp://ftp.ncbi.nlm.nih.gov/pubmed/{baseline,updatefiles}

base_url="ftp://ftp.ncbi.nlm.nih.gov/pubmed"
name_prefix="pubmed22n"

usage() {
	cat <<-_EOF_
		Useage: $(basename $0) OPTION... FILE[S]
		Downloads publication data files from pubmed and checks against md5s.

		The FILE argument can be the number of a file, a range of file numbers (i.e. {0001..0010}), or file numbers from stdin. Using stdin allows for piping in values obtained by running with the list option or cating a file with a list of files.

		  -s    source directory, which pubmed directory to get data from (baseline|updatefiles).
		  -d    destination directory, where to save files to (defaults to \$PWD).
		  -l    list files in source directory, if source directory is unset, show both.
		  -h    Show this help.

		TODO:
		- Should accept baseline/updatefiles/all flags to
		download all files in either baseline, updatefiles
		or both directories.
		- Should accept a directory to save the files to.
		- Should check the save directory to look for files
		that are already present and skip those.
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

src_dir="baseline"
dest_dir=$PWD
list_flag=false
while getopts "d:s:lh" arg; do
	case $arg in
	s) src_dir="$OPTARG" ;;
	d) dest_dir="$OPTARG" ;;
	l) list_flag=true ;;
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

if [[ "$#" -eq 0 ]]; then
	declare -a files
	i=0
	while read num; do
		i+=1
		files[$i]=$num
	done

	if [[ $i -eq 0 ]]; then
		usage >&2
		exit 1
	fi
else
	files="$@"
fi

[ -d $dest_dir ] || mkdir -p $dest_dir
cd $dest_dir

# Potentially make smarter check to see if the requested files already
# exist.
if [ $(ls $PWD | wc -w) -eq 0 ]; then
	echo "Downloading files..."
	download_files $src_dir "$files"
	echo "Finished downloading files."
fi
