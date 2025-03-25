#ifndef MMIO_EXCEPTION
#define MMIO_EXCEPTION

#include <exception>
#include <string>
#include <map>

namespace mmio{
  enum class MMErrorList{
  ok
  };

  class MMException: public std::exception {
    std::string m_error;
    std::map<MMErrorList, std::string> const msg = {
      {MMErrorList::ok, "Passed"}
    };

  public:
    MMException(MMErrorList error){
      int ierror = static_cast<int>(error);
      this->m_error = "[Error number " + std::to_string(ierror) + "]";
      auto m_iter = this->msg.find(error);
      if(m_iter != this->msg.end()) this->m_error += ": MMIO " + (*m_iter).second;
    };
    const char* what() const noexcept {return m_error.c_str();};
  };
}

#endif