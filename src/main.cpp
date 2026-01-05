#include <iostream>
#include <sstream>

#include "mmio.h"

int main() {
  std::istringstream input(
      "%%MatrixMarket matrix coordinate real general\n"
      "3 3 2\n"
      "1 1 10\n"
      "3 2 -5\n");

  const auto matrix = mmio::ReadMatrixMarket<double>(input);
  const auto csr = mmio::ToCSR(matrix);

  std::cout << matrix.rows << "x" << matrix.cols << " nnz=" << csr.nnz()
            << '\n';
}
