#include "mmio_functions.h"

int mmio_functions_header_compiles() {
  std::string value = " value ";
  mmio::Trim(value);
  return value == "value" ? 0 : 1;
}
