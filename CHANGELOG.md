# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- Protect against trying to delete non-existent md5 file.

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
