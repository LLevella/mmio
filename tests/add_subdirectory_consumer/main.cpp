#include "mmio.h"

#include <sstream>

int main() {
  std::istringstream input(
      "%%MatrixMarket matrix coordinate integer general\n"
      "2 2 1\n"
      "1 2 7\n");

  const auto matrix = mmio::ReadMatrixMarket<int>(input);
  return matrix.nnz() == 1 && matrix.col_indices[0] == 1 ? 0 : 1;
}
