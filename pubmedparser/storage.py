import os

import appdirs

from pubmedparser import __name__ as pkg_name

__all__ = ["default_cache_dir", "default_data_dir"]

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
