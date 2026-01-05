#include "mmio_matrix.h"

int mmio_matrix_header_compiles() {
  mmio::CooMatrix<int> matrix(1, 1);
  matrix.push_back(0, 0, 1);
  return matrix.nnz() == 1 ? 0 : 1;
}
