# Changelog

## 1.0.0

- Declared the stable C++17 API surface for COO, CSR, CSC, dense matrices,
  Matrix Market readers/writers, options, and conversion helpers.
- Added the `mmio::mmio` build-tree target alias for `add_subdirectory`
  consumers.
- Added an `add_subdirectory` consumer test.
- Added release archive SHA256 checksum generation.
- Added static documentation and a manual GitHub Pages deployment workflow.
- Added citation, contributing, and security metadata.

## 0.3.0

- Added `SparseOrdering::preserve` for non-sorting CSR/CSC conversion when
  duplicates should be kept.
- Added `ReadCSRMatrixMarketFileStreamed` and
  `ReadCSCMatrixMarketFileStreamed` two-pass file readers for large coordinate
  matrices without materializing a COO matrix first.
- Added deterministic fuzz-style parser regression tests.
- Added a lightweight `mmio_benchmark` target and smoke test.

## 0.2.0

- Added `ReadOptions` and `WriteOptions`.
- Added direct CSR/CSC read and write helpers.
- Added CSR/CSC to COO conversion helpers.
- Strengthened COO/CSR/CSC/dense validation.
- Added triangular Matrix Market output for symmetric, Hermitian, and
  skew-symmetric sparse matrices.
- Added install-consumer CMake test.
- Added Windows and sanitizer CI coverage.
- Added MIT license, vcpkg manifest, and Conan recipe.

## 0.1.0

- Added header-only C++17 Matrix Market I/O implementation.
- Added COO, CSR, CSC, and dense matrix structures.
- Added Matrix Market coordinate/array reading and writing.
- Added CMake package install/export and GitHub release packaging.
