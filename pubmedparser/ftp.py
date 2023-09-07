import hashlib
import os
import re
from ftplib import FTP
from typing import Iterable, List

from .storage import default_cache_dir

__all__ = ["download", "list_files"]

BASE_URL = "ftp.ncbi.nlm.nih.gov"
KNOWN_PUBMED_DIRECTORIES = ("baseline", "updatefiles")


def _download_files(
    remote_dir: str,
    file_names: List[str],
    cache_dir: str,
    attempt: int = 1,
    max_tries: int = 3,
) -> None:
    def in_cache(f):
        return os.path.join(cache_dir, f)

    with FTP(BASE_URL) as ftp:
        ftp.login()
        ftp.cwd("pubmed/" + remote_dir)
        for file_name in file_names:
            print(f"Downloading {file_name}")
            with open(in_cache(file_name), "wb") as f_wb:
                ftp.retrbinary(f"RETR {file_name}", f_wb.write)

            with open(in_cache(f"{file_name}.md5"), "wb") as f_wb:
                ftp.retrbinary(f"RETR {file_name}.md5", f_wb.write)

    md5_file_names = [f"{f}.md5" for f in file_names]
    for file_name, md5_file_name in zip(file_names, md5_file_names):
        with open(in_cache(md5_file_name), "r") as f_r:
            expected_md5 = f_r.read().split()[1]

        with open(in_cache(file_name), "rb") as f_rb:
            actual_md5 = hashlib.md5(f_rb.read()).hexdigest()

        if actual_md5 != expected_md5:
            if attempt <= max_tries:
                print(
                    f"{file_name} failed md5sum check, trying"
                    f" {max_tries - attempt} more times..."
                )
                _download_files(
                    remote_dir, [file_name], cache_dir, attempt + 1, max_tries
                )
            else:
                print(f"{file_name} failed md5sum check max tries, deleting")
                os.unlink(in_cache(file_name))

        os.unlink(in_cache(md5_file_name))


def _list_local_pubmed_files(path: str, name_regex_template) -> List[str]:
    files = os.listdir(path)
    regex = re.compile(name_regex_template.format("\d{4}"))
    return [f for f in files if regex.match(f)]


def list_files(remote_dir: str = "all") -> List[str]:
    """
    List the files on pubmed's ftp server

    Parameters
    ----------
    remote_dir : {"all", "baseline", "updatefiles"}
        The directory to list. If "all" (default) concatenate files from all
        directories.

    Returns
    -------
    files : list
        A list of all files in the requested directories.
    """

    assert remote_dir == "all" or remote_dir in KNOWN_PUBMED_DIRECTORIES
    files: List[str] = []
    if remote_dir == "all":
        for remote in KNOWN_PUBMED_DIRECTORIES:
            files += list_files(remote)
        return files

    with FTP(BASE_URL) as ftp:
        ftp.login()
        ftp.cwd("pubmed/" + remote_dir)
        ftp.retrlines("NLST", files.append)
    return [f for f in files if f.endswith(".xml.gz")]


def _missing_files(
    desired_files: List[str], name_regex_template: str, cache_dir: str
) -> List[str]:
    local_files = _list_local_pubmed_files(cache_dir, name_regex_template)
    unique_desired_files = set(desired_files)
    intersect = unique_desired_files & set(local_files)
    return sorted(list(unique_desired_files - intersect))


def _filter_to_file_numbers(
    files: List[str], numbers: Iterable[int], name_regex_template: str
) -> List[str]:
    number_pattern = "|".join([f"{n:0>4}" for n in numbers])
    regex = re.compile(name_regex_template.format(number_pattern))
    return [f for f in files if regex.match(f)]


def _find_file_prefix(file_name: str, regex: str) -> str:
    m = re.match(regex, file_name)
    if not m:
        raise NameError("Could not find file prefix. Please report bug.")

    return m.groups()[0]


def download(
    file_numbers: str | int | Iterable[int] = "all",
    cache_dir: str | None = None,
) -> List[str]:
    """
    Download XML files from pubmed's ftp server

    Files are saved locally to a cache directory. Only files that are not in
    the cache directory will be download. As such, once the full dataset as
    been downloaded it can be rerun using "all" to download only recently
    uploaded files.

    All downloaded files are validated against an md5 hash. Any files whose
    hash does not match the value provided by pubmed will be deleted.

    Parameters
    ----------
    file_numbers : str, int, list-like
        Which files to download. If "all" (default) downloads all available
        files. Otherwise, identify files by their index. Can provide a list of
        files or a generator.
    cache_dir : str, None
        Where to save the files. If None (default) use a subdirectory named
        after the file prefix (i.e. f"pubmed{year}n") under the default cache
        directory. This prefix prevents different years files from interfering
        which each other.

    Returns
    -------
    files : list
       List of the files asked for which can be passed directly to
       `pubmedparser.read_xml`.

    See also
    --------
    `pubmedparser.storage.default_cache_dir`.

    Examples
    --------
    >>> from pubmedparser import ftp
    >>> # Download a subset of files.
    >>> files = ftp.download(range(1300, 1310))
    >>> # Download all available files.
    >>> files = ftp.download()
    >>> # Call above periodically to check for and download new files.
    """

    if isinstance(file_numbers, str) and file_numbers != "all":
        raise TypeError('Files is not of type int or "all".')

    if isinstance(file_numbers, int):
        file_numbers = [file_numbers]

    name_regex_template = r"^{}{}\.xml\.gz$"
    remote_files = {k: list_files(k) for k in KNOWN_PUBMED_DIRECTORIES}
    prefix = _find_file_prefix(
        remote_files["baseline"][0],
        name_regex_template.format(r"(pubmed\d{2}n)", r"\d{4}"),
    )
    name_regex_template = name_regex_template.format(prefix, "({})")
    if not cache_dir:
        cache_dir = default_cache_dir(prefix)

    if not isinstance(file_numbers, str):
        remote_files = {
            k: _filter_to_file_numbers(
                remote_files[k], file_numbers, name_regex_template
            )
            for k in remote_files.keys()
        }

    missing_files = {
        k: _missing_files(remote_files[k], name_regex_template, cache_dir)
        for k in remote_files.keys()
    }

    if not os.path.exists(cache_dir):
        os.makedirs(cache_dir)

    if missing_files["baseline"] or missing_files["updatefiles"]:
        print("Downloading files...")
        for remote_dir in missing_files.keys():
            _download_files(remote_dir, missing_files[remote_dir], cache_dir)
        print("Finished downloading files.")

    if isinstance(file_numbers, str):
        requested_files = [
            os.path.join(cache_dir, f)
            for k in remote_files.keys()
            for f in remote_files[k]
        ]
    else:
        requested_files = [
            os.path.join(cache_dir, f"{prefix}{n:0>4}.xml.gz")
            for n in file_numbers
        ]
    cached_files = [f for f in requested_files if os.path.exists(f)]
    not_downloaded_files = set(requested_files) - set(cached_files)
    if not_downloaded_files:
        names_not_downloaded = [
            os.path.split(f)[-1] for f in not_downloaded_files
        ]
        print("Failed to collect:\n\t" + "\n\t".join(names_not_downloaded))
        print(
            "\nThese files may not exist (check ftp.list_files), or they may"
            " have been corrupted."
        )

    return cached_files
