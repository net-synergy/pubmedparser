import os

from ._readxml import from_structure_dictionary as _read_xml_from_dictionary
from ._readxml import from_structure_file as _read_xml_from_structure_file
from .storage import default_cache_dir


def _unprocessed_files(files: list[str], processed_files: str) -> list:
    """Filter that returns a list of files that have not been processed yet."""
    print(processed_files)
    print(os.path.exists(processed_files))
    if not os.path.exists(processed_files):
        return files

    file_set = set(files)
    with open(processed_files, "r") as read_files:
        for f in read_files.readlines():
            try:
                file_set.remove(f.strip("\n"))
            except KeyError:
                continue

    return list(file_set)


def read_xml(
    files: list[str],
    path_structure: dict | str,
    cache_dir: str,
    relative_to_default_cache: bool = True,
    progress_file: str = "processed.txt",
    n_threads=-1,
) -> None:
    """ """

    if relative_to_default_cache:
        cache_dir = default_cache_dir(cache_dir)

    files = _unprocessed_files(
        files, processed_files=os.path.join(cache_dir, progress_file)
    )

    if isinstance(path_structure, str):
        assert os.path.exists(
            path_structure
        ), "File for path structure not found."
        _read_xml_from_structure_file(
            files,
            path_structure,
            cache_dir,
            progress_file,
            n_threads,
        )
    else:
        raise NotImplementedError()
        _read_xml_from_dictionary(
            files,
            path_structure,
            cache_dir,
            progress_file,
            n_threads,
        )
