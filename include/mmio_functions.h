namespace mmio {
static inline void LTrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                  [](int c) { return !std::isspace(c); }));
}

static inline void RTrim(std::string &s) {
  s.erase(
      std::find_if(s.rbegin(), s.rend(), [](int c) { return !std::isspace(c); })
          .base(),
      s.end());
}
static inline void Trim(std::string &s) {
  LTrim(s);
  RTrim(s);
}
static inline void ToLower(std::string &s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}
static inline void ToLower(std::vector<std::string> &vs) {
  for (auto &cs : vs) ToLower(cs);
}

static std::vector<std::string> Split(const std::string &s,
                                      const unsigned char d) {
  
  find_if(s.begin(), s.end(), [d](unsigned char c) { return (c == d); });
}

static std::vector<std::string> Split(const std::string &s) {
  find_if(s.begin(), s.end(),
          [](unsigned char c) { return std::isspace(c); });
}

static std::string ReadLine(std::istream &fin,
                            std::function<bool(std::string &)> fline) {}
}  // namespace mmio