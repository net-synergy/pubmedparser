# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
