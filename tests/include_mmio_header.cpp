#include "mmio_header.h"

int mmio_header_header_compiles() {
  const auto header =
      mmio::Header::Parse("%%MatrixMarket matrix coordinate real general");
  return header.is_sparse() ? 0 : 1;
}
