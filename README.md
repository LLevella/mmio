# mmio

Header-only C++17 utilities for reading, writing, and converting Matrix Market
matrices.

## Features

- Matrix Market header parsing and validation.
- Coordinate (`coordinate`) and dense array (`array`) readers.
- Real, integer, unsigned integer, complex, and pattern fields.
- General, symmetric, Hermitian, and skew-symmetric expansion while reading.
- COO, CSR, CSC, and dense in-memory matrix structures.
- Direct COO, CSR, CSC, and dense Matrix Market read/write helpers.
- COO/CSR/CSC/dense conversion with duplicate summation by default.
- Optional preserve-order sparse conversion for large inputs when duplicates
  should be kept.
- Two-pass streamed file readers for direct CSR/CSC loading from coordinate
  Matrix Market files.
- Triangular output for symmetric, Hermitian, and skew-symmetric sparse
  matrices.
- CMake target, install/export package, benchmark target, CTest coverage, and
  CI/CD.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The optional benchmark target is built by default:

```sh
./build/mmio_benchmark 1024 100000
```

## Install and Package

```sh
cmake --install build --prefix install
cmake --build build --target package
```

Installed consumers can use:

```cmake
find_package(mmio REQUIRED)
target_link_libraries(app PRIVATE mmio::mmio)
```

Release archives are created by GitHub Actions when a tag matching `v*` is
pushed, for example:

```sh
git tag v0.3.0
git push origin v0.3.0
```

The repository also includes `vcpkg.json` and `conanfile.py` metadata for
package-manager based distribution.

## Example

```cpp
#include "mmio.h"

#include <sstream>

int main() {
  std::istringstream input(
      "%%MatrixMarket matrix coordinate real general\n"
      "3 3 2\n"
      "1 1 10\n"
      "3 2 -5\n");

  mmio::Header header;
  auto csr = mmio::ReadCSRMatrixMarket<double>(input, &header);

  return csr.nnz() == 2 ? 0 : 1;
}
```

## Matrix Types

### COO

Coordinate matrices are stored as:

- `values`: non-zero element values.
- `row_indices`: zero-based row indices.
- `col_indices`: zero-based column indices.
- `nnz()`: number of stored elements.

Matrix Market files use one-based indices; `mmio` converts them to zero-based
indices on read and back to one-based indices on write.

### CSR

Compressed Sparse Row matrices are stored as:

- `values`: non-zero element values.
- `row_ptr`: row offsets, size `rows + 1`.
- `col_indices`: zero-based column indices.

### CSC

Compressed Sparse Column matrices are stored as:

- `values`: non-zero element values.
- `col_ptr`: column offsets, size `cols + 1`.
- `row_indices`: zero-based row indices.

### Dense

Dense matrices are stored in row-major order in memory. Matrix Market `array`
input and output is column-major, as required by the format.

## API Notes

- `ReadMatrixMarket<T>(stream)` returns `CooMatrix<T>`.
- `ReadCSRMatrixMarket<T>(stream)` returns `CsrMatrix<T>`.
- `ReadCSCMatrixMarket<T>(stream)` returns `CscMatrix<T>`.
- `ReadCSRMatrixMarketFileStreamed<T>(path)` and
  `ReadCSCMatrixMarketFileStreamed<T>(path)` read coordinate files directly
  into CSR/CSC with a two-pass path. Set `ReadOptions::duplicate_policy` to
  `DuplicatePolicy::keep` to avoid the fallback duplicate-summing conversion.
- `ReadDenseMatrixMarket<T>(stream)` returns `DenseMatrix<T>`.
- `WriteMatrixMarket(stream, matrix)` writes COO, CSR, CSC, or dense matrices.
- `ToCSR`, `ToCSC`, `ToDense`, and `ToCOO` convert between supported formats.
- Duplicate COO entries are summed by default in sparse conversions. Pass
  `mmio::DuplicatePolicy::keep` to preserve duplicates.
- `SparseOrdering::preserve` avoids sorting during CSR/CSC conversion when
  duplicates are kept.
- `ReadOptions` controls symmetry expansion, duplicate handling, and sparse
  conversion ordering.
- `WriteOptions` controls headers, duplicate handling, symmetry validation, and
  triangular output.
