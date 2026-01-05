#ifndef MMIO_MATRIX_H
#define MMIO_MATRIX_H

#include <algorithm>
#include <complex>
#include <cstddef>
#include <fstream>
#include <istream>
#include <limits>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <tuple>
#include <utility>
#include <vector>

#include "mmio_exceptions.h"
#include "mmio_functions.h"
#include "mmio_header.h"

namespace mmio {

enum class DuplicatePolicy { keep, sum };

template <typename T>
struct CooMatrix {
  std::size_t rows = 0;
  std::size_t cols = 0;
  std::vector<std::size_t> row_indices;
  std::vector<std::size_t> col_indices;
  std::vector<T> values;

  CooMatrix() = default;
  CooMatrix(std::size_t row_count, std::size_t col_count)
      : rows(row_count), cols(col_count) {}

  std::size_t nnz() const noexcept { return values.size(); }

  void reserve(std::size_t count) {
    row_indices.reserve(count);
    col_indices.reserve(count);
    values.reserve(count);
  }

  void push_back(std::size_t row, std::size_t col, const T &value) {
    row_indices.emplace_back(row);
    col_indices.emplace_back(col);
    values.emplace_back(value);
  }

  void validate() const {
    if (row_indices.size() != values.size() ||
        col_indices.size() != values.size()) {
      throw MMException(MMErrorList::invalidSize,
                        "COO arrays must have the same length");
    }
    for (std::size_t i = 0; i < values.size(); ++i) {
      if (row_indices[i] >= rows || col_indices[i] >= cols) {
        throw MMException(MMErrorList::invalidEntry,
                          "COO index is outside matrix dimensions");
      }
    }
  }
};

template <typename T>
struct CsrMatrix {
  std::size_t rows = 0;
  std::size_t cols = 0;
  std::vector<std::size_t> row_ptr;
  std::vector<std::size_t> col_indices;
  std::vector<T> values;

  std::size_t nnz() const noexcept { return values.size(); }

  void validate() const {
    if (row_ptr.size() != rows + 1) {
      throw MMException(MMErrorList::invalidSize,
                        "CSR row_ptr size must be rows + 1");
    }
    if (col_indices.size() != values.size()) {
      throw MMException(MMErrorList::invalidSize,
                        "CSR arrays must have the same length");
    }
    if (!row_ptr.empty() && row_ptr.back() != values.size()) {
      throw MMException(MMErrorList::invalidSize,
                        "CSR row_ptr.back() must equal nnz");
    }
  }
};

template <typename T>
struct CscMatrix {
  std::size_t rows = 0;
  std::size_t cols = 0;
  std::vector<std::size_t> col_ptr;
  std::vector<std::size_t> row_indices;
  std::vector<T> values;

  std::size_t nnz() const noexcept { return values.size(); }

  void validate() const {
    if (col_ptr.size() != cols + 1) {
      throw MMException(MMErrorList::invalidSize,
                        "CSC col_ptr size must be cols + 1");
    }
    if (row_indices.size() != values.size()) {
      throw MMException(MMErrorList::invalidSize,
                        "CSC arrays must have the same length");
    }
    if (!col_ptr.empty() && col_ptr.back() != values.size()) {
      throw MMException(MMErrorList::invalidSize,
                        "CSC col_ptr.back() must equal nnz");
    }
  }
};

template <typename T>
struct DenseMatrix {
  std::size_t rows = 0;
  std::size_t cols = 0;
  std::vector<T> values;

  DenseMatrix() = default;
  DenseMatrix(std::size_t row_count, std::size_t col_count)
      : rows(row_count), cols(col_count), values(row_count * col_count) {}

  T &operator()(std::size_t row, std::size_t col) {
    return values[row * cols + col];
  }

  const T &operator()(std::size_t row, std::size_t col) const {
    return values[row * cols + col];
  }

  void validate() const {
    if (values.size() != rows * cols) {
      throw MMException(MMErrorList::invalidSize,
                        "Dense values size must be rows * cols");
    }
  }
};

template <typename T>
struct IsComplex : std::false_type {};

template <typename T>
struct IsComplex<std::complex<T>> : std::true_type {};

template <typename T>
inline constexpr bool IsComplexValue = IsComplex<T>::value;

template <typename T>
struct RealValueType {
  using type = T;
};

template <typename T>
struct RealValueType<std::complex<T>> {
  using type = T;
};

template <typename T>
inline constexpr DataTypes DeducedDataType() {
  if constexpr (IsComplexValue<T>) {
    return DataTypes::complex;
  } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
    return DataTypes::uinteger;
  } else if constexpr (std::is_integral_v<T>) {
    return DataTypes::integer;
  } else {
    return DataTypes::real;
  }
}

template <typename T>
inline T OneValue() {
  if constexpr (IsComplexValue<T>) {
    using Scalar = typename RealValueType<T>::type;
    return T{Scalar{1}, Scalar{0}};
  } else {
    return static_cast<T>(1);
  }
}

template <typename T>
inline T NegatedValue(const T &value) {
  return T{} - value;
}

template <typename T>
inline T ConjugatedValue(const T &value) {
  return value;
}

template <typename T>
inline std::complex<T> ConjugatedValue(const std::complex<T> &value) {
  return std::conj(value);
}

template <typename T>
inline T ParseStrict(const std::string &token, MMErrorList error,
                     const std::string &context) {
  if constexpr (std::is_unsigned_v<T>) {
    if (!token.empty() && token.front() == '-') {
      throw MMException(error, context + ": " + token);
    }
  }

  std::istringstream input(token);
  T value{};
  if (!(input >> value)) {
    throw MMException(error, context + ": " + token);
  }
  input >> std::ws;
  if (!input.eof()) {
    throw MMException(error, context + ": " + token);
  }
  return value;
}

inline std::size_t ParseSizeToken(const std::string &token,
                                  const std::string &context) {
  const auto value =
      ParseStrict<unsigned long long>(token, MMErrorList::invalidSize, context);
  if (value > static_cast<unsigned long long>(
                  std::numeric_limits<std::size_t>::max())) {
    throw MMException(MMErrorList::invalidSize, context + ": " + token);
  }
  return static_cast<std::size_t>(value);
}

inline std::size_t ParseCoordinateIndex(const std::string &token,
                                        std::size_t limit,
                                        const std::string &context) {
  const auto value =
      ParseStrict<unsigned long long>(token, MMErrorList::invalidEntry, context);
  if (value == 0 || value > static_cast<unsigned long long>(limit)) {
    throw MMException(MMErrorList::invalidEntry, context + ": " + token);
  }
  return static_cast<std::size_t>(value - 1);
}

template <typename T>
inline T ParseMatrixValue(const std::vector<std::string> &tokens,
                          std::size_t offset, DataTypes data_type,
                          const std::string &context) {
  if (data_type == DataTypes::pattern) return OneValue<T>();

  if (data_type == DataTypes::complex) {
    if (tokens.size() < offset + 2) {
      throw MMException(MMErrorList::invalidEntry, context);
    }
    if constexpr (!IsComplexValue<T>) {
      throw MMException(MMErrorList::typeMismatch, context);
    } else {
      using Scalar = typename RealValueType<T>::type;
      const auto real =
          ParseStrict<Scalar>(tokens[offset], MMErrorList::invalidEntry,
                              context + " real part");
      const auto imag =
          ParseStrict<Scalar>(tokens[offset + 1], MMErrorList::invalidEntry,
                              context + " imaginary part");
      return T{real, imag};
    }
  }

  if (tokens.size() < offset + 1) {
    throw MMException(MMErrorList::invalidEntry, context);
  }

  using Scalar = typename RealValueType<T>::type;
  const auto value = ParseStrict<Scalar>(tokens[offset],
                                         MMErrorList::invalidEntry, context);
  if constexpr (IsComplexValue<T>) {
    return T{value, Scalar{}};
  } else {
    return static_cast<T>(value);
  }
}

template <typename T>
inline void WriteMatrixValue(std::ostream &out, const T &value,
                             DataTypes data_type) {
  if (data_type == DataTypes::pattern) return;

  if (data_type == DataTypes::complex) {
    if constexpr (IsComplexValue<T>) {
      out << value.real() << ' ' << value.imag();
    } else {
      out << value << " 0";
    }
    return;
  }

  if constexpr (IsComplexValue<T>) {
    out << value.real();
  } else {
    out << value;
  }
}

template <typename T>
inline bool IsZeroValue(const T &value) {
  return value == T{};
}

template <typename T>
struct CooEntry {
  std::size_t row = 0;
  std::size_t col = 0;
  T value{};
};

template <typename T, typename Less>
inline std::vector<CooEntry<T>> SortedEntries(const CooMatrix<T> &matrix,
                                              Less less,
                                              DuplicatePolicy policy) {
  matrix.validate();

  std::vector<CooEntry<T>> entries;
  entries.reserve(matrix.nnz());
  for (std::size_t i = 0; i < matrix.nnz(); ++i) {
    entries.push_back(
        {matrix.row_indices[i], matrix.col_indices[i], matrix.values[i]});
  }

  std::sort(entries.begin(), entries.end(), less);
  if (policy == DuplicatePolicy::keep || entries.empty()) return entries;

  std::vector<CooEntry<T>> combined;
  combined.reserve(entries.size());
  for (const auto &entry : entries) {
    if (!combined.empty() && combined.back().row == entry.row &&
        combined.back().col == entry.col) {
      combined.back().value += entry.value;
    } else {
      combined.push_back(entry);
    }
  }
  return combined;
}

template <typename T>
inline CsrMatrix<T> ToCSR(const CooMatrix<T> &matrix,
                          DuplicatePolicy policy = DuplicatePolicy::sum) {
  const auto entries = SortedEntries(
      matrix,
      [](const CooEntry<T> &lhs, const CooEntry<T> &rhs) {
        return std::tie(lhs.row, lhs.col) < std::tie(rhs.row, rhs.col);
      },
      policy);

  CsrMatrix<T> result;
  result.rows = matrix.rows;
  result.cols = matrix.cols;
  result.row_ptr.assign(result.rows + 1, 0);
  result.col_indices.resize(entries.size());
  result.values.resize(entries.size());

  for (const auto &entry : entries) ++result.row_ptr[entry.row + 1];
  std::partial_sum(result.row_ptr.begin(), result.row_ptr.end(),
                   result.row_ptr.begin());

  auto cursor = result.row_ptr;
  for (const auto &entry : entries) {
    const auto position = cursor[entry.row]++;
    result.col_indices[position] = entry.col;
    result.values[position] = entry.value;
  }

  return result;
}

template <typename T>
inline CsrMatrix<T> ToCsr(const CooMatrix<T> &matrix,
                          DuplicatePolicy policy = DuplicatePolicy::sum) {
  return ToCSR(matrix, policy);
}

template <typename T>
inline CscMatrix<T> ToCSC(const CooMatrix<T> &matrix,
                          DuplicatePolicy policy = DuplicatePolicy::sum) {
  const auto entries = SortedEntries(
      matrix,
      [](const CooEntry<T> &lhs, const CooEntry<T> &rhs) {
        return std::tie(lhs.col, lhs.row) < std::tie(rhs.col, rhs.row);
      },
      policy);

  CscMatrix<T> result;
  result.rows = matrix.rows;
  result.cols = matrix.cols;
  result.col_ptr.assign(result.cols + 1, 0);
  result.row_indices.resize(entries.size());
  result.values.resize(entries.size());

  for (const auto &entry : entries) ++result.col_ptr[entry.col + 1];
  std::partial_sum(result.col_ptr.begin(), result.col_ptr.end(),
                   result.col_ptr.begin());

  auto cursor = result.col_ptr;
  for (const auto &entry : entries) {
    const auto position = cursor[entry.col]++;
    result.row_indices[position] = entry.row;
    result.values[position] = entry.value;
  }

  return result;
}

template <typename T>
inline CscMatrix<T> ToCsc(const CooMatrix<T> &matrix,
                          DuplicatePolicy policy = DuplicatePolicy::sum) {
  return ToCSC(matrix, policy);
}

template <typename T>
inline DenseMatrix<T> ToDense(
    const CooMatrix<T> &matrix,
    DuplicatePolicy policy = DuplicatePolicy::sum) {
  DenseMatrix<T> dense(matrix.rows, matrix.cols);
  const auto entries = SortedEntries(
      matrix,
      [](const CooEntry<T> &lhs, const CooEntry<T> &rhs) {
        return std::tie(lhs.row, lhs.col) < std::tie(rhs.row, rhs.col);
      },
      policy);

  for (const auto &entry : entries) dense(entry.row, entry.col) += entry.value;
  return dense;
}

template <typename T>
inline CooMatrix<T> ToCOO(const DenseMatrix<T> &matrix) {
  matrix.validate();

  CooMatrix<T> coo(matrix.rows, matrix.cols);
  for (std::size_t row = 0; row < matrix.rows; ++row) {
    for (std::size_t col = 0; col < matrix.cols; ++col) {
      const auto &value = matrix(row, col);
      if (!IsZeroValue(value)) coo.push_back(row, col, value);
    }
  }
  return coo;
}

inline Header MakeWriteHeader(StorageTypes storage_type, DataTypes data_type,
                              MatrixTypes matrix_type) {
  return Header(storage_type, data_type == DataTypes::undefined
                                  ? DataTypes::real
                                  : data_type,
                matrix_type == MatrixTypes::undefined
                    ? MatrixTypes::general
                    : matrix_type);
}

template <typename T>
inline Header ResolveWriteHeader(StorageTypes storage_type,
                                 const Header &requested) {
  const auto requested_storage = requested.storage_type();
  if (requested_storage != StorageTypes::undefined &&
      requested_storage != storage_type) {
    throw MMException(MMErrorList::unsupportedFormat,
                      "requested header format does not match matrix type");
  }

  const auto data_type = requested.data_type() == DataTypes::undefined
                             ? DeducedDataType<T>()
                             : requested.data_type();
  const auto matrix_type = requested.matrix_type() == MatrixTypes::undefined
                               ? MatrixTypes::general
                               : requested.matrix_type();
  return MakeWriteHeader(storage_type, data_type, matrix_type);
}

template <typename T>
inline void AddSymmetricEntry(CooMatrix<T> &matrix, std::size_t row,
                              std::size_t col, const T &value,
                              MatrixTypes matrix_type, bool expand_symmetry) {
  matrix.push_back(row, col, value);
  if (!expand_symmetry || row == col) return;

  switch (matrix_type) {
    case MatrixTypes::general:
    case MatrixTypes::undefined:
      return;
    case MatrixTypes::symmetric:
      matrix.push_back(col, row, value);
      return;
    case MatrixTypes::hermitian:
      matrix.push_back(col, row, ConjugatedValue(value));
      return;
    case MatrixTypes::skewSymmetric:
      matrix.push_back(col, row, NegatedValue(value));
      return;
  }
}

class DataTokenReader {
 public:
  DataTokenReader(std::istream &input, std::size_t *line_number)
      : input_(input), line_number_(line_number) {}

  std::string Next(const std::string &context) {
    while (position_ >= tokens_.size()) {
      const auto line = ReadDataLine(input_, line_number_);
      if (line.empty()) throw MMException(MMErrorList::unexpectedEof, context);
      tokens_ = Split(line);
      position_ = 0;
    }
    return tokens_[position_++];
  }

 private:
  std::istream &input_;
  std::size_t *line_number_;
  std::vector<std::string> tokens_;
  std::size_t position_ = 0;
};

template <typename T>
inline T ReadValue(DataTokenReader &reader, DataTypes data_type,
                   const std::string &context) {
  if (data_type == DataTypes::pattern) {
    throw MMException(MMErrorList::unsupportedField,
                      "pattern is only valid for coordinate matrices");
  }

  std::vector<std::string> tokens;
  tokens.emplace_back(reader.Next(context));
  if (data_type == DataTypes::complex) tokens.emplace_back(reader.Next(context));
  return ParseMatrixValue<T>(tokens, 0, data_type, context);
}

template <typename T>
inline DenseMatrix<T> ReadDenseBody(std::istream &input, const Header &header,
                                    std::size_t *line_number) {
  if (!header.is_dense()) {
    throw MMException(MMErrorList::unsupportedFormat, header.format_name());
  }

  const auto size_line = ReadDataLine(input, line_number);
  if (size_line.empty()) throw MMException(MMErrorList::unexpectedEof);

  const auto size_tokens = Split(size_line);
  if (size_tokens.size() != 2) {
    throw MMException(MMErrorList::invalidSize, size_line);
  }

  const auto rows = ParseSizeToken(size_tokens[0], "dense row count");
  const auto cols = ParseSizeToken(size_tokens[1], "dense column count");
  DenseMatrix<T> result(rows, cols);
  DataTokenReader reader(input, line_number);

  if (header.matrix_type() == MatrixTypes::general) {
    for (std::size_t col = 0; col < cols; ++col) {
      for (std::size_t row = 0; row < rows; ++row) {
        result(row, col) =
            ReadValue<T>(reader, header.data_type(), "dense value");
      }
    }
    return result;
  }

  if (rows != cols) {
    throw MMException(MMErrorList::unsupportedSymmetry,
                      "symmetric dense matrices must be square");
  }

  for (std::size_t col = 0; col < cols; ++col) {
    const auto first_row =
        header.matrix_type() == MatrixTypes::skewSymmetric ? col + 1 : col;
    for (std::size_t row = first_row; row < rows; ++row) {
      const auto value =
          ReadValue<T>(reader, header.data_type(), "dense triangular value");
      result(row, col) = value;
      if (row == col) continue;

      if (header.matrix_type() == MatrixTypes::symmetric) {
        result(col, row) = value;
      } else if (header.matrix_type() == MatrixTypes::hermitian) {
        result(col, row) = ConjugatedValue(value);
      } else if (header.matrix_type() == MatrixTypes::skewSymmetric) {
        result(col, row) = NegatedValue(value);
      }
    }
  }

  return result;
}

template <typename T>
inline CooMatrix<T> ReadSparseBody(std::istream &input, const Header &header,
                                   std::size_t *line_number,
                                   bool expand_symmetry) {
  if (!header.is_sparse()) {
    throw MMException(MMErrorList::unsupportedFormat, header.format_name());
  }

  const auto size_line = ReadDataLine(input, line_number);
  if (size_line.empty()) throw MMException(MMErrorList::unexpectedEof);

  const auto size_tokens = Split(size_line);
  if (size_tokens.size() != 3) {
    throw MMException(MMErrorList::invalidSize, size_line);
  }

  const auto rows = ParseSizeToken(size_tokens[0], "sparse row count");
  const auto cols = ParseSizeToken(size_tokens[1], "sparse column count");
  const auto entry_count = ParseSizeToken(size_tokens[2], "sparse nnz");
  CooMatrix<T> result(rows, cols);
  result.reserve(entry_count * (expand_symmetry ? 2 : 1));

  const std::size_t expected_tokens =
      header.data_type() == DataTypes::pattern
          ? 2
          : header.data_type() == DataTypes::complex ? 4 : 3;

  for (std::size_t entry = 0; entry < entry_count; ++entry) {
    const auto line = ReadDataLine(input, line_number);
    if (line.empty()) throw MMException(MMErrorList::unexpectedEof);

    const auto tokens = Split(line);
    if (tokens.size() != expected_tokens) {
      throw MMException(MMErrorList::invalidEntry, line);
    }

    const auto row = ParseCoordinateIndex(tokens[0], rows, "row index");
    const auto col = ParseCoordinateIndex(tokens[1], cols, "column index");
    const auto value =
        ParseMatrixValue<T>(tokens, 2, header.data_type(), "coordinate value");
    AddSymmetricEntry(result, row, col, value, header.matrix_type(),
                      expand_symmetry);
  }

  return result;
}

template <typename T>
inline CooMatrix<T> ReadMatrixMarket(std::istream &input,
                                     Header *header_out = nullptr,
                                     bool expand_symmetry = true) {
  Header header;
  input >> header;
  if (header_out != nullptr) *header_out = header;

  std::size_t line_number = 1;
  if (header.is_sparse()) {
    return ReadSparseBody<T>(input, header, &line_number, expand_symmetry);
  }

  return ToCOO(ReadDenseBody<T>(input, header, &line_number));
}

template <typename T>
inline DenseMatrix<T> ReadDenseMatrixMarket(std::istream &input,
                                            Header *header_out = nullptr,
                                            bool expand_symmetry = true) {
  Header header;
  input >> header;
  if (header_out != nullptr) *header_out = header;

  std::size_t line_number = 1;
  if (header.is_dense()) {
    return ReadDenseBody<T>(input, header, &line_number);
  }

  return ToDense(ReadSparseBody<T>(input, header, &line_number,
                                   expand_symmetry));
}

template <typename T>
inline CooMatrix<T> ReadMatrixMarketFile(const std::string &path,
                                         Header *header_out = nullptr,
                                         bool expand_symmetry = true) {
  std::ifstream input(path);
  if (!input) throw MMException(MMErrorList::readFileError, path);
  return ReadMatrixMarket<T>(input, header_out, expand_symmetry);
}

template <typename T>
inline DenseMatrix<T> ReadDenseMatrixMarketFile(
    const std::string &path, Header *header_out = nullptr,
    bool expand_symmetry = true) {
  std::ifstream input(path);
  if (!input) throw MMException(MMErrorList::readFileError, path);
  return ReadDenseMatrixMarket<T>(input, header_out, expand_symmetry);
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const CooMatrix<T> &matrix,
                              Header header = Header()) {
  matrix.validate();
  header = ResolveWriteHeader<T>(StorageTypes::sparse, header);

  out << header << '\n';
  out << matrix.rows << ' ' << matrix.cols << ' ' << matrix.nnz() << '\n';
  for (std::size_t entry = 0; entry < matrix.nnz(); ++entry) {
    out << matrix.row_indices[entry] + 1 << ' '
        << matrix.col_indices[entry] + 1;
    if (header.data_type() != DataTypes::pattern) {
      out << ' ';
      WriteMatrixValue(out, matrix.values[entry], header.data_type());
    }
    out << '\n';
  }
}

template <typename T>
inline void WriteDenseValue(std::ostream &out, const DenseMatrix<T> &matrix,
                            std::size_t row, std::size_t col,
                            DataTypes data_type) {
  WriteMatrixValue(out, matrix(row, col), data_type);
  out << '\n';
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const DenseMatrix<T> &matrix,
                              Header header = Header()) {
  matrix.validate();
  header = ResolveWriteHeader<T>(StorageTypes::dense, header);
  if (header.data_type() == DataTypes::pattern) {
    throw MMException(MMErrorList::unsupportedField,
                      "pattern is only valid for coordinate matrices");
  }

  out << header << '\n';
  out << matrix.rows << ' ' << matrix.cols << '\n';

  if (header.matrix_type() == MatrixTypes::general) {
    for (std::size_t col = 0; col < matrix.cols; ++col) {
      for (std::size_t row = 0; row < matrix.rows; ++row) {
        WriteDenseValue(out, matrix, row, col, header.data_type());
      }
    }
    return;
  }

  if (matrix.rows != matrix.cols) {
    throw MMException(MMErrorList::unsupportedSymmetry,
                      "symmetric dense matrices must be square");
  }

  for (std::size_t col = 0; col < matrix.cols; ++col) {
    const auto first_row =
        header.matrix_type() == MatrixTypes::skewSymmetric ? col + 1 : col;
    for (std::size_t row = first_row; row < matrix.rows; ++row) {
      WriteDenseValue(out, matrix, row, col, header.data_type());
    }
  }
}

template <typename T>
inline void WriteMatrixMarketFile(const std::string &path,
                                  const CooMatrix<T> &matrix,
                                  Header header = Header()) {
  std::ofstream output(path);
  if (!output) throw MMException(MMErrorList::readFileError, path);
  WriteMatrixMarket(output, matrix, header);
}

template <typename T>
inline void WriteDenseMatrixMarketFile(const std::string &path,
                                       const DenseMatrix<T> &matrix,
                                       Header header = Header()) {
  std::ofstream output(path);
  if (!output) throw MMException(MMErrorList::readFileError, path);
  WriteMatrixMarket(output, matrix, header);
}

}  // namespace mmio

#endif
