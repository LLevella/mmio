#ifndef MMIO_FUNCTIONS_H
#define MMIO_FUNCTIONS_H

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

namespace mmio {

inline bool IsSpace(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}

inline void LTrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                  [](char c) { return !IsSpace(c); }));
}

inline void RTrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](char c) { return !IsSpace(c); })
              .base(),
          s.end());
}

inline void Trim(std::string &s) {
  LTrim(s);
  RTrim(s);
}

inline std::string TrimCopy(std::string s) {
  Trim(s);
  return s;
}

inline void ToLower(std::string &s) {
  std::transform(s.begin(), s.end(), s.begin(), [](char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  });
}

inline void ToLower(std::vector<std::string> &vs) {
  for (auto &cs : vs) ToLower(cs);
}

inline std::string ToLowerCopy(std::string s) {
  ToLower(s);
  return s;
}

inline std::vector<std::string> Split(const std::string &s,
                                      const unsigned char d) {
  std::vector<std::string> tokens;
  if (s.empty()) return tokens;

  const char delimiter = static_cast<char>(d);
  std::string::size_type start = 0;
  while (start <= s.size()) {
    const auto end = s.find(delimiter, start);
    if (end == std::string::npos) {
      tokens.emplace_back(s.substr(start));
      break;
    }
    tokens.emplace_back(s.substr(start, end - start));
    start = end + 1;
  }
  return tokens;
}

inline std::vector<std::string> Split(const std::string &s) {
  std::istringstream input(s);
  std::vector<std::string> tokens;
  std::string token;
  while (input >> token) tokens.emplace_back(token);
  return tokens;
}

inline bool IsBlankLine(const std::string &line) {
  return TrimCopy(line).empty();
}

inline bool IsCommentLine(const std::string &line) {
  const auto trimmed = TrimCopy(line);
  return !trimmed.empty() && trimmed.front() == '%';
}

template <typename Predicate>
inline std::string ReadLine(std::istream &fin, Predicate predicate) {
  std::string line;
  while (std::getline(fin, line)) {
    if (predicate(line)) return line;
  }
  return {};
}

inline std::string ReadDataLine(std::istream &fin,
                                std::size_t *line_number = nullptr) {
  std::string line;
  while (std::getline(fin, line)) {
    if (line_number != nullptr) ++(*line_number);
    auto trimmed = TrimCopy(line);
    if (!trimmed.empty() && trimmed.front() != '%') return trimmed;
  }
  return {};
}

}  // namespace mmio

#endif
