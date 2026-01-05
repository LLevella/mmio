#include "mmio_exceptions.h"

int mmio_exceptions_header_compiles() {
  const mmio::MMException error(mmio::MMErrorList::ok);
  return error.what()[0] == '\0' ? 1 : 0;
}
