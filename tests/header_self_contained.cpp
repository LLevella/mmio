#include "mmio.h"

int mmio_exceptions_header_compiles();
int mmio_functions_header_compiles();
int mmio_header_header_compiles();
int mmio_matrix_header_compiles();

int main() {
  return mmio_exceptions_header_compiles() +
         mmio_functions_header_compiles() + mmio_header_header_compiles() +
         mmio_matrix_header_compiles();
}
