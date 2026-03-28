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
enum class SparseOrdering { sorted, preserve };

struct ReadOptions {
  bool expand_symmetry = true;
  DuplicatePolicy duplicate_policy = DuplicatePolicy::sum;
  SparseOrdering sparse_ordering = SparseOrdering::sorted;
};

struct WriteOptions {
  Header header;
  DuplicatePolicy duplicate_policy = DuplicatePolicy::sum;
  bool validate_symmetry = true;
  bool triangular_output = true;
};

inline std::size_t CheckedProduct(std::size_t lhs, std::size_t rhs,
                                  const std::string &context) {
  if (lhs != 0 && rhs > std::numeric_limits<std::size_t>::max() / lhs) {
    throw MMException(MMErrorList::invalidSize, context);
  }
  return lhs * rhs;
}

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
    if (!row_ptr.empty() && row_ptr.front() != 0) {
      throw MMException(MMErrorList::invalidSize,
                        "CSR row_ptr.front() must be zero");
    }
    for (std::size_t row = 1; row < row_ptr.size(); ++row) {
      if (row_ptr[row] < row_ptr[row - 1]) {
        throw MMException(MMErrorList::invalidSize,
                          "CSR row_ptr must be monotonic");
      }
      if (row_ptr[row] > values.size()) {
        throw MMException(MMErrorList::invalidSize,
                          "CSR row_ptr entry is outside values");
      }
    }
    for (const auto col : col_indices) {
      if (col >= cols) {
        throw MMException(MMErrorList::invalidEntry,
                          "CSR column index is outside matrix dimensions");
      }
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
    if (!col_ptr.empty() && col_ptr.front() != 0) {
      throw MMException(MMErrorList::invalidSize,
                        "CSC col_ptr.front() must be zero");
    }
    for (std::size_t col = 1; col < col_ptr.size(); ++col) {
      if (col_ptr[col] < col_ptr[col - 1]) {
        throw MMException(MMErrorList::invalidSize,
                          "CSC col_ptr must be monotonic");
      }
      if (col_ptr[col] > values.size()) {
        throw MMException(MMErrorList::invalidSize,
                          "CSC col_ptr entry is outside values");
      }
    }
    for (const auto row : row_indices) {
      if (row >= rows) {
        throw MMException(MMErrorList::invalidEntry,
                          "CSC row index is outside matrix dimensions");
      }
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
      : rows(row_count),
        cols(col_count),
        values(CheckedProduct(row_count, col_count,
                              "Dense matrix dimensions overflow")) {}

  T &operator()(std::size_t row, std::size_t col) {
    return values[row * cols + col];
  }

  const T &operator()(std::size_t row, std::size_t col) const {
    return values[row * cols + col];
  }

  void validate() const {
    if (values.size() != CheckedProduct(rows, cols,
                                        "Dense matrix dimensions overflow")) {
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
inline bool IsSameValue(const T &lhs, const T &rhs) {
  return lhs == rhs;
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
inline std::vector<CooEntry<T>> MatrixEntriesInInputOrder(
    const CooMatrix<T> &matrix) {
  matrix.validate();

  std::vector<CooEntry<T>> entries;
  entries.reserve(matrix.nnz());
  for (std::size_t i = 0; i < matrix.nnz(); ++i) {
    entries.push_back(
        {matrix.row_indices[i], matrix.col_indices[i], matrix.values[i]});
  }
  return entries;
}

template <typename T>
inline std::vector<CooEntry<T>> SortedEntriesByRowColumn(
    const CooMatrix<T> &matrix, DuplicatePolicy policy) {
  return SortedEntries(
      matrix,
      [](const CooEntry<T> &lhs, const CooEntry<T> &rhs) {
        return std::tie(lhs.row, lhs.col) < std::tie(rhs.row, rhs.col);
      },
      policy);
}

template <typename T>
inline CsrMatrix<T> ToCSRPreserve(const CooMatrix<T> &matrix) {
  matrix.validate();

  CsrMatrix<T> result;
  result.rows = matrix.rows;
  result.cols = matrix.cols;
  result.row_ptr.assign(result.rows + 1, 0);
  result.col_indices.resize(matrix.nnz());
  result.values.resize(matrix.nnz());

  for (const auto row : matrix.row_indices) ++result.row_ptr[row + 1];
  std::partial_sum(result.row_ptr.begin(), result.row_ptr.end(),
                   result.row_ptr.begin());

  auto cursor = result.row_ptr;
  for (std::size_t entry = 0; entry < matrix.nnz(); ++entry) {
    const auto position = cursor[matrix.row_indices[entry]]++;
    result.col_indices[position] = matrix.col_indices[entry];
    result.values[position] = matrix.values[entry];
  }

  return result;
}

template <typename T>
inline CsrMatrix<T> ToCSR(const CooMatrix<T> &matrix,
                          DuplicatePolicy policy = DuplicatePolicy::sum,
                          SparseOrdering ordering = SparseOrdering::sorted) {
  if (ordering == SparseOrdering::preserve &&
      policy == DuplicatePolicy::keep) {
    return ToCSRPreserve(matrix);
  }

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
                          DuplicatePolicy policy = DuplicatePolicy::sum,
                          SparseOrdering ordering = SparseOrdering::sorted) {
  return ToCSR(matrix, policy, ordering);
}

template <typename T>
inline CscMatrix<T> ToCSCPreserve(const CooMatrix<T> &matrix) {
  matrix.validate();

  CscMatrix<T> result;
  result.rows = matrix.rows;
  result.cols = matrix.cols;
  result.col_ptr.assign(result.cols + 1, 0);
  result.row_indices.resize(matrix.nnz());
  result.values.resize(matrix.nnz());

  for (const auto col : matrix.col_indices) ++result.col_ptr[col + 1];
  std::partial_sum(result.col_ptr.begin(), result.col_ptr.end(),
                   result.col_ptr.begin());

  auto cursor = result.col_ptr;
  for (std::size_t entry = 0; entry < matrix.nnz(); ++entry) {
    const auto position = cursor[matrix.col_indices[entry]]++;
    result.row_indices[position] = matrix.row_indices[entry];
    result.values[position] = matrix.values[entry];
  }

  return result;
}

template <typename T>
inline CscMatrix<T> ToCSC(const CooMatrix<T> &matrix,
                          DuplicatePolicy policy = DuplicatePolicy::sum,
                          SparseOrdering ordering = SparseOrdering::sorted) {
  if (ordering == SparseOrdering::preserve &&
      policy == DuplicatePolicy::keep) {
    return ToCSCPreserve(matrix);
  }

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
                          DuplicatePolicy policy = DuplicatePolicy::sum,
                          SparseOrdering ordering = SparseOrdering::sorted) {
  return ToCSC(matrix, policy, ordering);
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

template <typename T>
inline CooMatrix<T> ToCOO(const CsrMatrix<T> &matrix) {
  matrix.validate();

  CooMatrix<T> coo(matrix.rows, matrix.cols);
  coo.reserve(matrix.nnz());
  for (std::size_t row = 0; row < matrix.rows; ++row) {
    for (std::size_t position = matrix.row_ptr[row];
         position < matrix.row_ptr[row + 1]; ++position) {
      coo.push_back(row, matrix.col_indices[position], matrix.values[position]);
    }
  }
  return coo;
}

template <typename T>
inline CooMatrix<T> ToCOO(const CscMatrix<T> &matrix) {
  matrix.validate();

  CooMatrix<T> coo(matrix.rows, matrix.cols);
  coo.reserve(matrix.nnz());
  for (std::size_t col = 0; col < matrix.cols; ++col) {
    for (std::size_t position = matrix.col_ptr[col];
         position < matrix.col_ptr[col + 1]; ++position) {
      coo.push_back(matrix.row_indices[position], col, matrix.values[position]);
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

inline bool ShouldExpandSymmetry(MatrixTypes matrix_type, bool expand_symmetry,
                                 std::size_t row, std::size_t col) {
  return expand_symmetry && row != col &&
         matrix_type != MatrixTypes::general &&
         matrix_type != MatrixTypes::undefined;
}

template <typename T>
inline T CounterpartValue(const T &value, MatrixTypes matrix_type) {
  switch (matrix_type) {
    case MatrixTypes::symmetric:
      return value;
    case MatrixTypes::hermitian:
      return ConjugatedValue(value);
    case MatrixTypes::skewSymmetric:
      return NegatedValue(value);
    case MatrixTypes::general:
    case MatrixTypes::undefined:
      return value;
  }
  return value;
}

template <typename T>
inline const CooEntry<T> *FindSortedEntry(
    const std::vector<CooEntry<T>> &entries, std::size_t row, std::size_t col) {
  const CooEntry<T> needle{row, col, T{}};
  const auto iter = std::lower_bound(
      entries.begin(), entries.end(), needle,
      [](const CooEntry<T> &lhs, const CooEntry<T> &rhs) {
        return std::tie(lhs.row, lhs.col) < std::tie(rhs.row, rhs.col);
      });
  if (iter == entries.end() || iter->row != row || iter->col != col) {
    return nullptr;
  }
  return &(*iter);
}

template <typename T>
inline std::vector<CooEntry<T>> EntriesForCoordinateWrite(
    const CooMatrix<T> &matrix, const Header &header,
    const WriteOptions &options) {
  if (header.matrix_type() == MatrixTypes::general ||
      header.matrix_type() == MatrixTypes::undefined ||
      !options.triangular_output) {
    return MatrixEntriesInInputOrder(matrix);
  }

  if (matrix.rows != matrix.cols) {
    throw MMException(MMErrorList::unsupportedSymmetry,
                      "symmetric sparse matrices must be square");
  }

  const auto entries =
      SortedEntriesByRowColumn(matrix, options.duplicate_policy);
  std::vector<CooEntry<T>> writable;
  writable.reserve(entries.size());

  for (const auto &entry : entries) {
    if (entry.row == entry.col) {
      if (header.matrix_type() == MatrixTypes::skewSymmetric) {
        if (!IsZeroValue(entry.value)) {
          throw MMException(MMErrorList::invalidEntry,
                            "skew-symmetric diagonal values must be zero");
        }
        continue;
      }
      writable.push_back(entry);
      continue;
    }

    const auto counterpart =
        FindSortedEntry(entries, entry.col, entry.row);
    if (options.validate_symmetry && counterpart != nullptr) {
      const auto expected = CounterpartValue(entry.value, header.matrix_type());
      if (!IsSameValue(counterpart->value, expected)) {
        throw MMException(MMErrorList::invalidEntry,
                          "matrix values do not match requested symmetry");
      }
    }

    if (entry.row > entry.col) {
      writable.push_back(entry);
      continue;
    }

    if (counterpart == nullptr) {
      writable.push_back(
          {entry.col, entry.row,
           CounterpartValue(entry.value, header.matrix_type())});
    }
  }

  return writable;
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

inline std::size_t ExpectedCoordinateTokens(DataTypes data_type) {
  if (data_type == DataTypes::pattern) return 2;
  if (data_type == DataTypes::complex) return 4;
  return 3;
}

struct SparseMatrixSize {
  std::size_t rows = 0;
  std::size_t cols = 0;
  std::size_t entries = 0;
};

inline SparseMatrixSize ReadSparseSizeLine(std::istream &input,
                                           std::size_t *line_number) {
  const auto size_line = ReadDataLine(input, line_number);
  if (size_line.empty()) throw MMException(MMErrorList::unexpectedEof);

  const auto size_tokens = Split(size_line);
  if (size_tokens.size() != 3) {
    throw MMException(MMErrorList::invalidSize, size_line);
  }

  SparseMatrixSize size;
  size.rows = ParseSizeToken(size_tokens[0], "sparse row count");
  size.cols = ParseSizeToken(size_tokens[1], "sparse column count");
  size.entries = ParseSizeToken(size_tokens[2], "sparse nnz");
  return size;
}

template <typename T>
inline CooEntry<T> ParseCoordinateEntryLine(const std::string &line,
                                            const Header &header,
                                            std::size_t rows,
                                            std::size_t cols) {
  const auto tokens = Split(line);
  if (tokens.size() != ExpectedCoordinateTokens(header.data_type())) {
    throw MMException(MMErrorList::invalidEntry, line);
  }

  const auto row = ParseCoordinateIndex(tokens[0], rows, "row index");
  const auto col = ParseCoordinateIndex(tokens[1], cols, "column index");
  const auto value =
      ParseMatrixValue<T>(tokens, 2, header.data_type(), "coordinate value");
  return {row, col, value};
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

  const auto size = ReadSparseSizeLine(input, line_number);
  CooMatrix<T> result(size.rows, size.cols);
  result.reserve(CheckedProduct(size.entries, expand_symmetry ? 2 : 1,
                                "sparse nnz reserve overflow"));

  for (std::size_t entry = 0; entry < size.entries; ++entry) {
    const auto line = ReadDataLine(input, line_number);
    if (line.empty()) throw MMException(MMErrorList::unexpectedEof);

    const auto parsed =
        ParseCoordinateEntryLine<T>(line, header, size.rows, size.cols);
    AddSymmetricEntry(result, parsed.row, parsed.col, parsed.value,
                      header.matrix_type(),
                      expand_symmetry);
  }

  return result;
}

template <typename T>
inline CooMatrix<T> ReadMatrixMarket(std::istream &input,
                                     Header *header_out = nullptr,
                                     ReadOptions options = ReadOptions()) {
  Header header;
  input >> header;
  if (header_out != nullptr) *header_out = header;

  std::size_t line_number = 1;
  if (header.is_sparse()) {
    return ReadSparseBody<T>(input, header, &line_number,
                             options.expand_symmetry);
  }

  return ToCOO(ReadDenseBody<T>(input, header, &line_number));
}

template <typename T>
inline CooMatrix<T> ReadMatrixMarket(std::istream &input, Header *header_out,
                                     bool expand_symmetry) {
  ReadOptions options;
  options.expand_symmetry = expand_symmetry;
  return ReadMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline DenseMatrix<T> ReadDenseMatrixMarket(std::istream &input,
                                            Header *header_out = nullptr,
                                            ReadOptions options =
                                                ReadOptions()) {
  Header header;
  input >> header;
  if (header_out != nullptr) *header_out = header;

  std::size_t line_number = 1;
  if (header.is_dense()) {
    return ReadDenseBody<T>(input, header, &line_number);
  }

  return ToDense(ReadSparseBody<T>(input, header, &line_number,
                                   options.expand_symmetry),
                 options.duplicate_policy);
}

template <typename T>
inline DenseMatrix<T> ReadDenseMatrixMarket(std::istream &input,
                                            Header *header_out,
                                            bool expand_symmetry) {
  ReadOptions options;
  options.expand_symmetry = expand_symmetry;
  return ReadDenseMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline CsrMatrix<T> ReadCSRMatrixMarket(std::istream &input,
                                        Header *header_out = nullptr,
                                        ReadOptions options = ReadOptions()) {
  return ToCSR(ReadMatrixMarket<T>(input, header_out, options),
               options.duplicate_policy, options.sparse_ordering);
}

template <typename T>
inline CsrMatrix<T> ReadCsrMatrixMarket(std::istream &input,
                                        Header *header_out = nullptr,
                                        ReadOptions options = ReadOptions()) {
  return ReadCSRMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline CscMatrix<T> ReadCSCMatrixMarket(std::istream &input,
                                        Header *header_out = nullptr,
                                        ReadOptions options = ReadOptions()) {
  return ToCSC(ReadMatrixMarket<T>(input, header_out, options),
               options.duplicate_policy, options.sparse_ordering);
}

template <typename T>
inline CscMatrix<T> ReadCscMatrixMarket(std::istream &input,
                                        Header *header_out = nullptr,
                                        ReadOptions options = ReadOptions()) {
  return ReadCSCMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline CooMatrix<T> ReadMatrixMarketFile(const std::string &path,
                                         Header *header_out = nullptr,
                                         ReadOptions options = ReadOptions()) {
  std::ifstream input(path);
  if (!input) throw MMException(MMErrorList::readFileError, path);
  return ReadMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline CooMatrix<T> ReadMatrixMarketFile(const std::string &path,
                                         Header *header_out,
                                         bool expand_symmetry) {
  ReadOptions options;
  options.expand_symmetry = expand_symmetry;
  return ReadMatrixMarketFile<T>(path, header_out, options);
}

template <typename T>
inline DenseMatrix<T> ReadDenseMatrixMarketFile(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  std::ifstream input(path);
  if (!input) throw MMException(MMErrorList::readFileError, path);
  return ReadDenseMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline DenseMatrix<T> ReadDenseMatrixMarketFile(const std::string &path,
                                                Header *header_out,
                                                bool expand_symmetry) {
  ReadOptions options;
  options.expand_symmetry = expand_symmetry;
  return ReadDenseMatrixMarketFile<T>(path, header_out, options);
}

template <typename T>
inline CsrMatrix<T> ReadCSRMatrixMarketFile(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  std::ifstream input(path);
  if (!input) throw MMException(MMErrorList::readFileError, path);
  return ReadCSRMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline CsrMatrix<T> ReadCsrMatrixMarketFile(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  return ReadCSRMatrixMarketFile<T>(path, header_out, options);
}

template <typename T>
inline CscMatrix<T> ReadCSCMatrixMarketFile(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  std::ifstream input(path);
  if (!input) throw MMException(MMErrorList::readFileError, path);
  return ReadCSCMatrixMarket<T>(input, header_out, options);
}

template <typename T>
inline CscMatrix<T> ReadCscMatrixMarketFile(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  return ReadCSCMatrixMarketFile<T>(path, header_out, options);
}

template <typename T>
inline CsrMatrix<T> ReadCSRMatrixMarketFileStreamed(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  if (options.duplicate_policy != DuplicatePolicy::keep) {
    return ReadCSRMatrixMarketFile<T>(path, header_out, options);
  }

  std::ifstream count_input(path);
  if (!count_input) throw MMException(MMErrorList::readFileError, path);

  Header header;
  count_input >> header;
  if (header_out != nullptr) *header_out = header;
  if (!header.is_sparse()) {
    return ReadCSRMatrixMarketFile<T>(path, header_out, options);
  }

  std::size_t line_number = 1;
  const auto size = ReadSparseSizeLine(count_input, &line_number);

  CsrMatrix<T> result;
  result.rows = size.rows;
  result.cols = size.cols;
  result.row_ptr.assign(result.rows + 1, 0);

  for (std::size_t entry = 0; entry < size.entries; ++entry) {
    const auto line = ReadDataLine(count_input, &line_number);
    if (line.empty()) throw MMException(MMErrorList::unexpectedEof);
    const auto parsed =
        ParseCoordinateEntryLine<T>(line, header, size.rows, size.cols);
    ++result.row_ptr[parsed.row + 1];
    if (ShouldExpandSymmetry(header.matrix_type(), options.expand_symmetry,
                             parsed.row, parsed.col)) {
      ++result.row_ptr[parsed.col + 1];
    }
  }

  std::partial_sum(result.row_ptr.begin(), result.row_ptr.end(),
                   result.row_ptr.begin());
  result.col_indices.resize(result.row_ptr.back());
  result.values.resize(result.row_ptr.back());

  std::ifstream fill_input(path);
  if (!fill_input) throw MMException(MMErrorList::readFileError, path);

  Header fill_header;
  fill_input >> fill_header;
  (void)fill_header;
  line_number = 1;
  (void)ReadSparseSizeLine(fill_input, &line_number);

  auto cursor = result.row_ptr;
  const auto store = [&result, &cursor](std::size_t row, std::size_t col,
                                        const T &value) {
    const auto position = cursor[row]++;
    result.col_indices[position] = col;
    result.values[position] = value;
  };

  for (std::size_t entry = 0; entry < size.entries; ++entry) {
    const auto line = ReadDataLine(fill_input, &line_number);
    if (line.empty()) throw MMException(MMErrorList::unexpectedEof);
    const auto parsed =
        ParseCoordinateEntryLine<T>(line, header, size.rows, size.cols);
    store(parsed.row, parsed.col, parsed.value);
    if (ShouldExpandSymmetry(header.matrix_type(), options.expand_symmetry,
                             parsed.row, parsed.col)) {
      store(parsed.col, parsed.row,
            CounterpartValue(parsed.value, header.matrix_type()));
    }
  }

  return result;
}

template <typename T>
inline CsrMatrix<T> ReadCsrMatrixMarketFileStreamed(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  return ReadCSRMatrixMarketFileStreamed<T>(path, header_out, options);
}

template <typename T>
inline CscMatrix<T> ReadCSCMatrixMarketFileStreamed(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  if (options.duplicate_policy != DuplicatePolicy::keep) {
    return ReadCSCMatrixMarketFile<T>(path, header_out, options);
  }

  std::ifstream count_input(path);
  if (!count_input) throw MMException(MMErrorList::readFileError, path);

  Header header;
  count_input >> header;
  if (header_out != nullptr) *header_out = header;
  if (!header.is_sparse()) {
    return ReadCSCMatrixMarketFile<T>(path, header_out, options);
  }

  std::size_t line_number = 1;
  const auto size = ReadSparseSizeLine(count_input, &line_number);

  CscMatrix<T> result;
  result.rows = size.rows;
  result.cols = size.cols;
  result.col_ptr.assign(result.cols + 1, 0);

  for (std::size_t entry = 0; entry < size.entries; ++entry) {
    const auto line = ReadDataLine(count_input, &line_number);
    if (line.empty()) throw MMException(MMErrorList::unexpectedEof);
    const auto parsed =
        ParseCoordinateEntryLine<T>(line, header, size.rows, size.cols);
    ++result.col_ptr[parsed.col + 1];
    if (ShouldExpandSymmetry(header.matrix_type(), options.expand_symmetry,
                             parsed.row, parsed.col)) {
      ++result.col_ptr[parsed.row + 1];
    }
  }

  std::partial_sum(result.col_ptr.begin(), result.col_ptr.end(),
                   result.col_ptr.begin());
  result.row_indices.resize(result.col_ptr.back());
  result.values.resize(result.col_ptr.back());

  std::ifstream fill_input(path);
  if (!fill_input) throw MMException(MMErrorList::readFileError, path);

  Header fill_header;
  fill_input >> fill_header;
  (void)fill_header;
  line_number = 1;
  (void)ReadSparseSizeLine(fill_input, &line_number);

  auto cursor = result.col_ptr;
  const auto store = [&result, &cursor](std::size_t row, std::size_t col,
                                        const T &value) {
    const auto position = cursor[col]++;
    result.row_indices[position] = row;
    result.values[position] = value;
  };

  for (std::size_t entry = 0; entry < size.entries; ++entry) {
    const auto line = ReadDataLine(fill_input, &line_number);
    if (line.empty()) throw MMException(MMErrorList::unexpectedEof);
    const auto parsed =
        ParseCoordinateEntryLine<T>(line, header, size.rows, size.cols);
    store(parsed.row, parsed.col, parsed.value);
    if (ShouldExpandSymmetry(header.matrix_type(), options.expand_symmetry,
                             parsed.row, parsed.col)) {
      store(parsed.col, parsed.row,
            CounterpartValue(parsed.value, header.matrix_type()));
    }
  }

  return result;
}

template <typename T>
inline CscMatrix<T> ReadCscMatrixMarketFileStreamed(
    const std::string &path, Header *header_out = nullptr,
    ReadOptions options = ReadOptions()) {
  return ReadCSCMatrixMarketFileStreamed<T>(path, header_out, options);
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const CooMatrix<T> &matrix,
                              WriteOptions options) {
  matrix.validate();
  const auto header =
      ResolveWriteHeader<T>(StorageTypes::sparse, options.header);
  const auto entries = EntriesForCoordinateWrite(matrix, header, options);

  out << header << '\n';
  out << matrix.rows << ' ' << matrix.cols << ' ' << entries.size() << '\n';
  for (const auto &entry : entries) {
    out << entry.row + 1 << ' ' << entry.col + 1;
    if (header.data_type() != DataTypes::pattern) {
      out << ' ';
      WriteMatrixValue(out, entry.value, header.data_type());
    }
    out << '\n';
  }
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const CooMatrix<T> &matrix,
                              Header header = Header()) {
  WriteOptions options;
  options.header = header;
  WriteMatrixMarket(out, matrix, options);
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
                              WriteOptions options) {
  matrix.validate();
  const auto header =
      ResolveWriteHeader<T>(StorageTypes::dense, options.header);
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
inline void WriteMatrixMarket(std::ostream &out, const DenseMatrix<T> &matrix,
                              Header header = Header()) {
  WriteOptions options;
  options.header = header;
  WriteMatrixMarket(out, matrix, options);
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const CsrMatrix<T> &matrix,
                              WriteOptions options) {
  WriteMatrixMarket(out, ToCOO(matrix), options);
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const CsrMatrix<T> &matrix,
                              Header header = Header()) {
  WriteOptions options;
  options.header = header;
  WriteMatrixMarket(out, matrix, options);
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const CscMatrix<T> &matrix,
                              WriteOptions options) {
  WriteMatrixMarket(out, ToCOO(matrix), options);
}

template <typename T>
inline void WriteMatrixMarket(std::ostream &out, const CscMatrix<T> &matrix,
                              Header header = Header()) {
  WriteOptions options;
  options.header = header;
  WriteMatrixMarket(out, matrix, options);
}

template <typename T>
inline void WriteMatrixMarketFile(const std::string &path,
                                  const CooMatrix<T> &matrix,
                                  WriteOptions options) {
  std::ofstream output(path);
  if (!output) throw MMException(MMErrorList::readFileError, path);
  WriteMatrixMarket(output, matrix, options);
}

template <typename T>
inline void WriteMatrixMarketFile(const std::string &path,
                                  const CooMatrix<T> &matrix,
                                  Header header = Header()) {
  WriteOptions options;
  options.header = header;
  WriteMatrixMarketFile(path, matrix, options);
}

template <typename T>
inline void WriteDenseMatrixMarketFile(const std::string &path,
                                       const DenseMatrix<T> &matrix,
                                       WriteOptions options) {
  std::ofstream output(path);
  if (!output) throw MMException(MMErrorList::readFileError, path);
  WriteMatrixMarket(output, matrix, options);
}

template <typename T>
inline void WriteDenseMatrixMarketFile(const std::string &path,
                                       const DenseMatrix<T> &matrix,
                                       Header header = Header()) {
  WriteOptions options;
  options.header = header;
  WriteDenseMatrixMarketFile(path, matrix, options);
}

template <typename T>
inline void WriteCSRMatrixMarketFile(const std::string &path,
                                     const CsrMatrix<T> &matrix,
                                     WriteOptions options = WriteOptions()) {
  std::ofstream output(path);
  if (!output) throw MMException(MMErrorList::readFileError, path);
  WriteMatrixMarket(output, matrix, options);
}

template <typename T>
inline void WriteCsrMatrixMarketFile(const std::string &path,
                                     const CsrMatrix<T> &matrix,
                                     WriteOptions options = WriteOptions()) {
  WriteCSRMatrixMarketFile(path, matrix, options);
}

template <typename T>
inline void WriteCSCMatrixMarketFile(const std::string &path,
                                     const CscMatrix<T> &matrix,
                                     WriteOptions options = WriteOptions()) {
  std::ofstream output(path);
  if (!output) throw MMException(MMErrorList::readFileError, path);
  WriteMatrixMarket(output, matrix, options);
}

template <typename T>
inline void WriteCscMatrixMarketFile(const std::string &path,
                                     const CscMatrix<T> &matrix,
                                     WriteOptions options = WriteOptions()) {
  WriteCSCMatrixMarketFile(path, matrix, options);
}

}  // namespace mmio

#endif
