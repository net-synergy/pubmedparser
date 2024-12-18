# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.2] 2024-11-04

### Fixed

- Memory error caused by taking the length of the wrong string.
- Handle case where tag's value is empty.
- Handle tag mismatch causing a dynamic path to not get unwound by end of parsing.

## [2.1.1] 2024-10-31

### Fixed

- Build ext from compiling shared lib with library dependencies out of order (leading to missing runtime dependencies and a "missing symbol" error).

## [2.1.0] 2024-10-29

### Added

- Support for multi-threading using pthreads.
- Ability to interrupt.
- Workflow for building and publishing wheels.

### Changed

- Move error handling to externally defined error handlers (i.e. so the python interface can define a python friendly error handler).

### Fixed

- Protect against trying to delete non-existent md5 file.
- Allow the program to continue after a parse error by removing data stored from the malformed input file.
- Memory leaks using valgrind.
- Ensure the reprocess all flag works even if no files have been processed yet.

## [2.0.6] 2024-01-20

### Fixed

- Handle failed ftp downloads.
- Passing a directory to `read_xml`

## [2.0.5] 2023-11-14

### Fixed

- Removed extra '\t' added to condensed group's output.

### Changed

- Python make target to use poetry build instead of python setup build.

## [2.0.4] 2023-11-07

### Fixed

- Missing -fPIC flag when compiling the library.
- Update README with current installation istructions.
- Ensure C and header files are included so package can be built without wheel.

## [2.0.3] 2023-10-17

### Changed

- Replace all whitespace characters with a regular space in XML values.

## [2.0.2] 2023-10-16

### Changed

- Use components for column names instead of the node name. In some cases the node name doesn't represent the column's content well.

## [2.0.1] 2023-10-12

### Fixed

- Prevent printing extra characters in condensed groups ID field (https://gitlab.com/net-synergy/pubmedparser/-/issues/9)

### Changed

- Modify optional arguments in python `readxml` functions.

## [2.0.0] 2023-10-09

### Added

- Changelog

### Fixed

- Typo in example's structure dictionary.

### Changed

- Reorder printing so attributes appear before associated values (https://gitlab.com/net-synergy/pubmedparser/-/issues/10)
- Auto index now uses Index postfix instead of ID in header.
- Python dev tools replaced with primarily ruff.
