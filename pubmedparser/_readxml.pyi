def from_structure_file(
    files: list[str],
    path_structure: str,
    cache_dir: str,
    progress_file: str,
    n_threads: int,
    overwrite_cache: bool,
) -> None: ...
def from_structure_dictionary(
    files: list[str],
    path_structure: dict[str, dict | str],
    cache_dir: str,
    progress_file: str,
    n_threads: int,
    overwrite_cache: bool,
) -> None: ...
