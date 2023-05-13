import os
from typing import List

import appdirs

from pubmedparser import __name__ as pkg_name

__all__ = [
    "default_cache_dir",
    "default_data_dir",
    "clear_cache",
    "clear_data",
    "list_cache",
    "list_data",
]

_APPAUTHOR = "net_synergy"


def default_cache_dir(path: str = "") -> str:
    """Find the default location to save cache files.

    If the directory does not exist it is created.

    Cache files are specifically files that can be easily reproduced,
    i.e. those that can be downloaded from the internet.
    """

    cache_dir = appdirs.user_cache_dir(pkg_name, _APPAUTHOR)
    cache_dir = os.path.join(cache_dir, path)
    if not os.path.exists(cache_dir):
        os.makedirs(cache_dir, mode=0o755)

    return cache_dir


def default_data_dir(path: str = "") -> str:
    """Find the default location to save data files.

    If the directory does not exist it is created.

    Data files are files created by a user. It's possible they can be
    reproduced by rerunning the script that produced them but there is
    no gurentee they can be perfectly reproduced.
    """

    data_dir = appdirs.user_data_dir(pkg_name, _APPAUTHOR)
    data_dir = os.path.join(data_dir, path)
    if not os.path.exists(data_dir):
        os.makedirs(data_dir, mode=0o755)

    return data_dir


def _dir_exists(path: str) -> bool:
    """default directory commands create a directory so test if the directory
    is empty"""

    try:
        os.rmdir(path)
    except OSError:
        return True

    return False


def _clear_dir(path: str) -> None:
    if not _dir_exists(path):
        raise NotADirectoryError("Path does not exist")

    for f_name in os.listdir(path):
        f_path = os.path.join(path, f_name)
        if os.path.isdir(f_path):
            _clear_dir(f_path)
        else:
            os.unlink(f_path)

    os.rmdir(path)


def clear_cache(path: str = "") -> None:
    """Clear a cache directory

    By default clears all data cached by this package.

    Parameters
    ----------
    path : str
        If not an empty string (default), clear only the directory PATH
        relative to the default cache.
    """

    _clear_dir(default_cache_dir(path))


def clear_data(path: str = "") -> None:
    """Clear a data directory

    By default clears all data saved by this package.

    Parameters
    ----------
    path : str
        If not an empty string (default), clear only the directory PATH
        relative to the default data directory.
    """

    _clear_dir(default_data_dir(path))


def list_cache(path: str = "") -> List[str]:
    """Show the contents of the default cache directory.

    Parameters
    ----------
    path : str
        If a path is provided, list the contents of the directory below the
        default cache.
    """

    if not path:
        return os.listdir(default_cache_dir(path))

    path = default_cache_dir(path)
    if _dir_exists(path):
        return os.listdir(path)

    raise NotADirectoryError(
        "Path is not a directory relative to " + default_cache_dir()
    )


def list_data(path: str = "") -> List[str]:
    """Show the contents of the default data directory.

    Parameters
    ----------
    path : str
        If a path is provided, list the contents of the directory below the
        default data.
    """

    if not path:
        return os.listdir(default_data_dir(path))

    path = default_data_dir(path)
    if _dir_exists(path):
        return os.listdir(path)

    raise NotADirectoryError(
        "Path is not a directory relative to " + default_data_dir()
    )
