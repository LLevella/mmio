#ifndef MMIO_EXCEPTION
#define MMIO_EXCEPTION

#include <exception>
#include <string>

namespace mmio {
enum class MMErrorList {
  ok,
  readFileError,
  invalidHeader,
  unsupportedObject,
  unsupportedFormat,
  unsupportedField,
  unsupportedSymmetry,
  invalidSize,
  invalidEntry,
  unexpectedEof,
  typeMismatch
};

inline const char *ErrorMessage(MMErrorList error) noexcept {
  switch (error) {
    case MMErrorList::ok:
      return "Passed";
    case MMErrorList::readFileError:
      return "Couldn't read file";
    case MMErrorList::invalidHeader:
      return "Invalid Matrix Market header";
    case MMErrorList::unsupportedObject:
      return "Unsupported Matrix Market object";
    case MMErrorList::unsupportedFormat:
      return "Unsupported Matrix Market format";
    case MMErrorList::unsupportedField:
      return "Unsupported Matrix Market field";
    case MMErrorList::unsupportedSymmetry:
      return "Unsupported Matrix Market symmetry";
    case MMErrorList::invalidSize:
      return "Invalid Matrix Market size line";
    case MMErrorList::invalidEntry:
      return "Invalid Matrix Market entry";
    case MMErrorList::unexpectedEof:
      return "Unexpected end of Matrix Market input";
    case MMErrorList::typeMismatch:
      return "Matrix Market value type does not match requested C++ type";
  }
  return "Unknown Matrix Market error";
}

class MMException : public std::exception {
  std::string m_error;
  MMErrorList m_error_code;

 public:
  explicit MMException(MMErrorList error, const std::string &detail = {})
      : m_error_code(error) {
    const auto ierror = static_cast<int>(error);
    m_error = "[Error number " + std::to_string(ierror) + "]: MMIO ";
    m_error += ErrorMessage(error);
    if (!detail.empty()) m_error += ": " + detail;
  }

  const char *what() const noexcept override { return m_error.c_str(); }

  MMErrorList error_code() const noexcept { return m_error_code; }
};
}  // namespace mmio

#endif
