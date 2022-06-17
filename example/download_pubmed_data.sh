#!/usr/bin/env bash

# Example usage:
#     download_pubmed_data.sh 1110
#     download_pubmed_data.sh {1110..1114}
#
# Find pubmed files:
#     ftp://ftp.ncbi.nlm.nih.gov/pubmed/{baseline,updatefiles}

usage() {
	cat <<-_EOF_
		Useage: $(basename $0) download_pubmed_data.sh OPTION... FILE[S]
		Downloads publication data files from pubmed and checks against md5s.

		The FILE argument can be the number of a file or a range of file numbers (i.e. {1..10}).

		TODO:
		- Currently only checks the baseline directory, should
		eventually look in the updatefiles directory.
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
	local n_files="$#"
	local baseline_url="ftp://ftp.ncbi.nlm.nih.gov/pubmed/baseline"
	local file_names=$(expand_file_names pubmed22n%s.xml.gz "$@")
	local file_urls=$(expand_file_names "${baseline_url}/%s" "$file_names")
	cd $cache_dir

	wget $file_urls
	wget $(expand_file_names %s.md5 "$file_urls")

	checks=$(md5sum -c *.md5 2>/dev/null)
	echo -e "${checks}" | grep "OK" | grep -o "pubmed22n[0-9]\+.xml.gz" |
		xargs -I {} rm {}.md5

	echo -e "${checks}" | grep "FAILED" | grep -o "pubmed22n[0-9]\+.xml.gz" |
		xargs -I {} sh -c "echo {} failed md5sum check, deleting && rm {}* >&2"
}

[ -d $data_dir ] || mkdir -p $data_dir

# Potentially make smarter check to see if the requested files already
# exist.
if [ $(ls $data_dir | wc -w) -eq 0 ]; then
	echo "Downloading files..."
	download_files "$@"
	echo "Finished downloading files."
fi
