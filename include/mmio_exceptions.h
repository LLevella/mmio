#ifndef MMIO_EXCEPTION
#define MMIO_EXCEPTION

#include <exception>

namespace mmio {
enum class MMErrorList { ok, readFileError };

class MMException : public std::exception {
  std::string m_error;
  static const std::map<MMErrorList, std::string> msg;

 public:
  MMException(MMErrorList error) {
    int ierror = static_cast<int>(error);
    this->m_error = "[Error number " + std::to_string(ierror) + "]";
    auto m_iter = this->msg.find(error);
    if (m_iter != this->msg.end())
      this->m_error += ": MMIO " + (*m_iter).second;
  };
  const char* what() const noexcept { return m_error.c_str(); };
};

const std::map<MMErrorList, std::string> MMException::msg = {
    {MMErrorList::ok, "Passed"},
    {MMErrorList::readFileError, "Couldn't read file"}};
}  // namespace mmio

#endif