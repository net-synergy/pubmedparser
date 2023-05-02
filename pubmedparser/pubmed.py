import hashlib
import os
import re
from ftplib import FTP
from typing import Iterable, List

from .storage import default_cache_dir

__all__ = ["download", "list_files"]

BASE_URL = "ftp.ncbi.nlm.nih.gov"
NAME_PREFIX = "pubmed23n"  # Update yearly
NAME_REGEX_TEMPLATE = r".*{}({})\.xml\.gz$".format(NAME_PREFIX, "{}")
KNOWN_PUBMED_DIRECTORIES = ("baseline", "updatefiles")


def _download_files(remote_dir: str, file_names: List[str], cache_dir: str):
    def in_cache(f):
        return os.path.join(cache_dir, f)

    with FTP(BASE_URL) as ftp:
        ftp.login()
        ftp.cwd("pubmed/" + remote_dir)
        for file_name in file_names:
            print(f"Downloading {file_name}")
            with open(in_cache(file_name), "wb") as fw:
                ftp.retrbinary(f"RETR {file_name}", fw.write)

            with open(in_cache(f"{file_name}.md5"), "wb") as fw:
                ftp.retrbinary(f"RETR {file_name}.md5", fw.write)

    md5_file_names = [f"{f}.md5" for f in file_names]
    for file_name, md5_file_name in zip(file_names, md5_file_names):
        with open(in_cache(md5_file_name), "r") as fr:
            expected_md5 = fr.read().split()[1]

        with open(in_cache(file_name), "rb") as fr:
            actual_md5 = hashlib.md5(fr.read()).hexdigest()

        if actual_md5 != expected_md5:
            print(f"{file_name} failed md5sum check, deleting")
            os.unlink(file_name)

        os.unlink(in_cache(md5_file_name))


def _list_local_pubmed_files(path: str) -> List[str]:
    files = os.listdir(path)
    regex = re.compile(NAME_REGEX_TEMPLATE.format("[0-9]{4}"))
    return [f for f in files if regex.match(f)]


def list_files(remote_dir: str = "all") -> List[str]:
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


def _missing_files(desired_files: List[str], cache_dir: str) -> List[str]:
    local_files = _list_local_pubmed_files(cache_dir)
    unique_desired_files = set(desired_files)
    intersect = unique_desired_files & set(local_files)
    return sorted(list(unique_desired_files - intersect))


def _filter_to_file_numbers(files: List[str], numbers: Iterable[int]):
    number_pattern = "|".join([f"{n}" for n in numbers])
    regex = re.compile(NAME_REGEX_TEMPLATE.format(number_pattern))
    return [f for f in files if regex.match(f)]


def download(
    file_numbers: str | int | Iterable[int] = "all",
    cache_dir: str = default_cache_dir(NAME_PREFIX),
) -> str:

    if isinstance(file_numbers, str) and file_numbers != "all":
        raise TypeError('Files is not of type int or "all".')

    if isinstance(file_numbers, int):
        file_numbers = [file_numbers]

    remote_files = {
        "baseline": list_files("baseline"),
        "updatefiles": list_files("updatefiles"),
    }
    if not isinstance(file_numbers, str):
        remote_files = {
            k: _filter_to_file_numbers(remote_files[k], file_numbers)
            for k in remote_files.keys()
        }

    missing_files = {
        k: _missing_files(remote_files[k], cache_dir)
        for k in remote_files.keys()
    }

    if not os.path.exists(cache_dir):
        os.makedirs(cache_dir)
    os.chdir(cache_dir)

    if missing_files["baseline"] or missing_files["updatefiles"]:
        print("Downloading files...")
        for remote_dir in missing_files.keys():
            _download_files(remote_dir, missing_files[remote_dir], cache_dir)
        print("Finished downloading files.")

    return cache_dir
