#include "mmio.h"

#include <sstream>

int main() {
  std::istringstream input(
      "%%MatrixMarket matrix coordinate real general\n"
      "2 2 2\n"
      "1 1 1\n"
      "2 2 2\n");

  const auto csr = mmio::ReadCSRMatrixMarket<double>(input);
  return csr.nnz() == 2 && csr.row_ptr.size() == 3 ? 0 : 1;
}
