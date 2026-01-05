#ifndef MMIO_HEADER_H
#define MMIO_HEADER_H

#include <istream>
#include <ostream>
#include <string>
#include <vector>

#include "mmio_exceptions.h"
#include "mmio_functions.h"

namespace mmio {

enum class StorageTypes { undefined, dense, sparse };
enum class DataTypes { undefined, real, integer, uinteger, complex, pattern };
enum class MatrixTypes {
  undefined,
  general,
  symmetric,
  hermitian,
  skewSymmetric
};
enum class TriangleTypes { undefined, lower, upper, both, diagonal };

inline std::string ToString(StorageTypes type) {
  switch (type) {
    case StorageTypes::dense:
      return "array";
    case StorageTypes::sparse:
      return "coordinate";
    case StorageTypes::undefined:
      return "undefined";
  }
  return "undefined";
}

inline std::string ToString(DataTypes type) {
  switch (type) {
    case DataTypes::real:
      return "real";
    case DataTypes::integer:
      return "integer";
    case DataTypes::uinteger:
      return "unsigned-integer";
    case DataTypes::complex:
      return "complex";
    case DataTypes::pattern:
      return "pattern";
    case DataTypes::undefined:
      return "undefined";
  }
  return "undefined";
}

inline std::string ToString(MatrixTypes type) {
  switch (type) {
    case MatrixTypes::general:
      return "general";
    case MatrixTypes::symmetric:
      return "symmetric";
    case MatrixTypes::hermitian:
      return "hermitian";
    case MatrixTypes::skewSymmetric:
      return "skew-symmetric";
    case MatrixTypes::undefined:
      return "undefined";
  }
  return "undefined";
}

inline StorageTypes ParseStorageType(const std::string &token) {
  if (token == "array") return StorageTypes::dense;
  if (token == "coordinate") return StorageTypes::sparse;
  throw MMException(MMErrorList::unsupportedFormat, token);
}

inline DataTypes ParseDataType(const std::string &token) {
  if (token == "real") return DataTypes::real;
  if (token == "integer") return DataTypes::integer;
  if (token == "unsigned-integer") return DataTypes::uinteger;
  if (token == "complex") return DataTypes::complex;
  if (token == "pattern") return DataTypes::pattern;
  throw MMException(MMErrorList::unsupportedField, token);
}

inline MatrixTypes ParseMatrixType(const std::string &token) {
  if (token == "general") return MatrixTypes::general;
  if (token == "symmetric") return MatrixTypes::symmetric;
  if (token == "hermitian") return MatrixTypes::hermitian;
  if (token == "skew-symmetric") return MatrixTypes::skewSymmetric;
  throw MMException(MMErrorList::unsupportedSymmetry, token);
}

class Header {
 public:
  inline static constexpr const char *MMstart = "%%matrixmarket";
  inline static constexpr const char *dataDefinition = "matrix";
  inline static constexpr const char *dataDeffinition = dataDefinition;

 private:
  StorageTypes stype;
  DataTypes dtype;
  MatrixTypes mtype;

 public:
  Header()
      : stype(StorageTypes::undefined),
        dtype(DataTypes::undefined),
        mtype(MatrixTypes::undefined) {}

  Header(StorageTypes storage_type, DataTypes data_type,
         MatrixTypes matrix_type = MatrixTypes::general)
      : stype(storage_type), dtype(data_type), mtype(matrix_type) {}

  StorageTypes storage_type() const noexcept { return stype; }
  DataTypes data_type() const noexcept { return dtype; }
  MatrixTypes matrix_type() const noexcept { return mtype; }

  bool is_sparse() const noexcept { return stype == StorageTypes::sparse; }
  bool is_dense() const noexcept { return stype == StorageTypes::dense; }
  bool is_pattern() const noexcept { return dtype == DataTypes::pattern; }
  bool is_complex() const noexcept { return dtype == DataTypes::complex; }

  std::string format_name() const { return ToString(stype); }
  std::string field_name() const { return ToString(dtype); }
  std::string symmetry_name() const { return ToString(mtype); }

  static Header Parse(std::string line) {
    Trim(line);
    ToLower(line);

    const auto tokens = Split(line);
    if (tokens.size() != 5) {
      throw MMException(MMErrorList::invalidHeader, line);
    }
    if (tokens[0] != MMstart) {
      throw MMException(MMErrorList::invalidHeader, tokens[0]);
    }
    if (tokens[1] != dataDefinition) {
      throw MMException(MMErrorList::unsupportedObject, tokens[1]);
    }

    return Header(ParseStorageType(tokens[2]), ParseDataType(tokens[3]),
                  ParseMatrixType(tokens[4]));
  }

  friend std::istream &operator>>(std::istream &fin, Header &h) {
    if (!fin) throw MMException(MMErrorList::readFileError);

    const auto line =
        ReadLine(fin, [](const std::string &candidate) {
          return !IsBlankLine(candidate);
        });
    if (line.empty()) throw MMException(MMErrorList::unexpectedEof);

    h = Header::Parse(line);
    return fin;
  }

  friend std::ostream &operator<<(std::ostream &out, const Header &h) {
    out << "%%MatrixMarket matrix " << h.format_name() << ' '
        << h.field_name() << ' ' << h.symmetry_name();
    return out;
  }
};

}  // namespace mmio

#endif
