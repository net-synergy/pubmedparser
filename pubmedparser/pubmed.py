import os
import re
import subprocess
from ftplib import FTP
from typing import List

BASE_URL = "ftp.ncbi.nlm.nih.gov"
NAME_PREFIX = "pubmed23n"  # Update yearly


def _download_files(src_dir: str, args: List[str]):
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


def _find_file_numbers(filenames: List[str]) -> List[str]:
    pattern = r".*{}([0-9]+)\.xml\.gz$".format(NAME_PREFIX)
    match = re.match(pattern, filename)
    if match:
        return match.group(1)
    else:
        return None


def list_files(src_dir: str) -> List[str]:
    files: List[str] = []
    with FTP(BASE_URL) as ftp:
        ftp.login()
        ftp.cwd("pubmed/" + src_dir)
        ftp.retrlines("NLST", files.append)
    return [f for f in files if f.endswith(".xml.gz")]


def _missing_files(desired_files: List[str]) -> List[str]:
    local_files = sorted(_find_file_numbers(os.listdir(os.getcwd())))
    intersect = sorted(list(set(desired_files) & set(local_files)))
    return sorted(list(set(desired_files) - set(intersect)))


def download_pubmed_data(dest_dir: str, source: str):
    file_names = list_files(source)

    desired_file = _missing_files(file_names)

    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)
    os.chdir(dest_dir)

    if len(desired_file) > 0:
        print("Downloading files...")
        _download_files(source, desired_file)
        print("Finished downloading files.")
