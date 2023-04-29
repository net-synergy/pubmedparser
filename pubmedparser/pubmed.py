import os
import re
import subprocess
from ftplib import FTP
from typing import Iterable, List

from .storage import default_cache_dir

BASE_URL = "ftp.ncbi.nlm.nih.gov"
NAME_PREFIX = "pubmed23n"  # Update yearly
NAME_REGEX_TEMPLATE = r".*{}({})\.xml\.gz$".format(NAME_PREFIX, "{}")


def _download_files(src_dir: str, args: List[str], cache_dir: str):
    file_names = [f"{NAME_PREFIX}{arg}.xml.gz" for arg in args]
    file_urls = [
        f"{BASE_URL}/{src_dir}/{file_name}" for file_name in file_names
    ]

    subprocess.run(["wget", *file_urls])
    subprocess.run(["wget", *[f"{f}.md5" for f in file_urls]])

    md5_file_names = [f"{f}.md5" for f in file_names]
    checks = subprocess.run(
        ["md5sum", "-c", *md5_file_names], capture_output=True, text=True
    ).stdout
    for line in checks.split("\n"):
        if "OK" in line:
            os.remove(line.split()[0])
        elif "FAILED" in line:
            failed_file = line.split(":")[0]
            print(f"{failed_file} failed md5sum check, deleting")
            os.remove(failed_file)
            os.remove(f"{failed_file}.md5")


def _list_local_pubmed_files(path: str) -> List[str]:
    files = os.listdir(path)
    regex = re.compile(NAME_REGEX_TEMPLATE.format("[0-9]{4}"))
    return [f for f in files if regex.match(f)]


def list_files(src_dir: str) -> List[str]:
    files: List[str] = []
    with FTP(BASE_URL) as ftp:
        ftp.login()
        ftp.cwd("pubmed/" + src_dir)
        ftp.retrlines("NLST", files.append)
    return [f for f in files if f.endswith(".xml.gz")]


def _missing_files(desired_files: List[str], cache_dir: str) -> List[str]:
    local_files = sorted(_list_local_pubmed_files(cache_dir))
    intersect = sorted(list(set(desired_files) & set(local_files)))
    return sorted(list(set(desired_files) - set(intersect)))


def _filter_to_file_numbers(files: List[str], numbers: Iterable[int]):
    number_pattern = "|".join([f"{n}" for n in numbers])
    print(number_pattern)
    regex = re.compile(NAME_REGEX_TEMPLATE.format(number_pattern))
    return [f for f in files if regex.match(f)]


def download_pubmed_data(
    files: str | int | Iterable[int] = "all",
    source: str = "updatefiles",
    cache_dir: str = default_cache_dir(NAME_PREFIX),
):
    if isinstance(files, str) and files != "all":
        raise TypeError('Files is not of type int or "all".')

    if isinstance(files, int):
        files = [files]

    remote_files = list_files(source)
    if not isinstance(files, str):
        remote_files = _filter_to_file_numbers(remote_files, files)

    missing_files = _missing_files(remote_files, cache_dir)

    if not os.path.exists(cache_dir):
        os.makedirs(cache_dir)
    os.chdir(cache_dir)

    if missing_files:
        print("Downloading files...")
        _download_files(source, missing_files, cache_dir)
        print("Finished downloading files.")
