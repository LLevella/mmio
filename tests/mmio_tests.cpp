#include "mmio.h"

#include <complex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Require(bool condition, const std::string &message) {
  if (!condition) throw std::runtime_error(message);
}

template <typename T>
void RequireEqual(const T &actual, const T &expected,
                  const std::string &message) {
  if (!(actual == expected)) throw std::runtime_error(message);
}

template <typename T>
void RequireVectorEqual(const std::vector<T> &actual,
                        const std::vector<T> &expected,
                        const std::string &message) {
  if (actual != expected) throw std::runtime_error(message);
}

template <typename F>
void RequireMMThrow(F &&function, const std::string &message) {
  bool thrown = false;
  try {
    function();
  } catch (const mmio::MMException &) {
    thrown = true;
  }
  if (!thrown) throw std::runtime_error(message);
}

void TestStringHelpers() {
  std::string value = " \tHello Matrix Market\n";
  mmio::Trim(value);
  RequireEqual(value, std::string("Hello Matrix Market"), "Trim failed");

  mmio::ToLower(value);
  RequireEqual(value, std::string("hello matrix market"), "ToLower failed");

  RequireVectorEqual(mmio::Split("a,,b", ','),
                     std::vector<std::string>{"a", "", "b"},
                     "delimiter split failed");
  RequireVectorEqual(mmio::Split(" a \t b  c "),
                     std::vector<std::string>{"a", "b", "c"},
                     "whitespace split failed");
}

void TestHeaderParsing() {
  const auto header =
      mmio::Header::Parse("%%MatrixMarket MATRIX Coordinate Real General");
  Require(header.is_sparse(), "header storage parsing failed");
  RequireEqual(header.data_type(), mmio::DataTypes::real,
               "header data type parsing failed");
  RequireEqual(header.matrix_type(), mmio::MatrixTypes::general,
               "header matrix type parsing failed");

  std::ostringstream output;
  output << header;
  RequireEqual(output.str(),
               std::string("%%MatrixMarket matrix coordinate real general"),
               "header serialization failed");

  RequireMMThrow([] { mmio::Header::Parse("%%MatrixMarket vector coordinate real general"); },
                 "invalid object was accepted");
}

void TestReadCoordinateReal() {
  std::istringstream input(
      "%%MatrixMarket matrix coordinate real general\n"
      "% comment\n"
      "3 3 3\n"
      "1 1 2.5\n"
      "2 3 4\n"
      "3 2 -1\n");

  mmio::Header header;
  const auto matrix = mmio::ReadMatrixMarket<double>(input, &header);

  Require(header.is_sparse(), "read header was not returned");
  RequireEqual(matrix.rows, std::size_t{3}, "row count failed");
  RequireEqual(matrix.cols, std::size_t{3}, "column count failed");
  RequireEqual(matrix.nnz(), std::size_t{3}, "nnz failed");
  RequireVectorEqual(matrix.row_indices, std::vector<std::size_t>{0, 1, 2},
                     "COO rows failed");
  RequireVectorEqual(matrix.col_indices, std::vector<std::size_t>{0, 2, 1},
                     "COO columns failed");
  RequireVectorEqual(matrix.values, std::vector<double>{2.5, 4.0, -1.0},
                     "COO values failed");
}

void TestPatternSymmetricExpansion() {
  std::istringstream input(
      "%%MatrixMarket matrix coordinate pattern symmetric\n"
      "3 3 2\n"
      "1 1\n"
      "3 1\n");

  const auto matrix = mmio::ReadMatrixMarket<double>(input);
  RequireEqual(matrix.nnz(), std::size_t{3}, "pattern expansion nnz failed");
  RequireVectorEqual(matrix.row_indices, std::vector<std::size_t>{0, 2, 0},
                     "pattern expansion rows failed");
  RequireVectorEqual(matrix.col_indices, std::vector<std::size_t>{0, 0, 2},
                     "pattern expansion columns failed");
  RequireVectorEqual(matrix.values, std::vector<double>{1.0, 1.0, 1.0},
                     "pattern expansion values failed");
}

void TestComplexHermitianExpansion() {
  using Complex = std::complex<double>;
  std::istringstream input(
      "%%MatrixMarket matrix coordinate complex hermitian\n"
      "2 2 3\n"
      "1 1 2 0\n"
      "2 1 3 4\n"
      "2 2 5 0\n");

  const auto matrix = mmio::ReadMatrixMarket<Complex>(input);
  RequireEqual(matrix.nnz(), std::size_t{4}, "hermitian expansion nnz failed");
  RequireVectorEqual(matrix.values,
                     std::vector<Complex>{Complex{2, 0}, Complex{3, 4},
                                          Complex{3, -4}, Complex{5, 0}},
                     "hermitian values failed");
}

void TestSparseConversions() {
  mmio::CooMatrix<int> matrix(3, 3);
  matrix.push_back(0, 2, 1);
  matrix.push_back(0, 2, 2);
  matrix.push_back(2, 1, 5);
  matrix.push_back(1, 0, 4);

  const auto csr = mmio::ToCSR(matrix);
  RequireVectorEqual(csr.row_ptr, std::vector<std::size_t>{0, 1, 2, 3},
                     "CSR row_ptr failed");
  RequireVectorEqual(csr.col_indices, std::vector<std::size_t>{2, 0, 1},
                     "CSR columns failed");
  RequireVectorEqual(csr.values, std::vector<int>{3, 4, 5},
                     "CSR values failed");

  const auto csc = mmio::ToCSC(matrix);
  RequireVectorEqual(csc.col_ptr, std::vector<std::size_t>{0, 1, 2, 3},
                     "CSC col_ptr failed");
  RequireVectorEqual(csc.row_indices, std::vector<std::size_t>{1, 2, 0},
                     "CSC rows failed");
  RequireVectorEqual(csc.values, std::vector<int>{4, 5, 3},
                     "CSC values failed");
}

void TestRoundTripCoordinate() {
  mmio::CooMatrix<int> matrix(2, 3);
  matrix.push_back(0, 0, 7);
  matrix.push_back(1, 2, -2);

  std::stringstream buffer;
  mmio::WriteMatrixMarket(buffer, matrix);

  const auto read_back = mmio::ReadMatrixMarket<int>(buffer);
  RequireEqual(read_back.rows, matrix.rows, "roundtrip rows failed");
  RequireEqual(read_back.cols, matrix.cols, "roundtrip columns failed");
  RequireVectorEqual(read_back.row_indices, matrix.row_indices,
                     "roundtrip row indices failed");
  RequireVectorEqual(read_back.col_indices, matrix.col_indices,
                     "roundtrip column indices failed");
  RequireVectorEqual(read_back.values, matrix.values,
                     "roundtrip values failed");
}

void TestDenseArrayRead() {
  std::istringstream input(
      "%%MatrixMarket matrix array real general\n"
      "2 3\n"
      "1\n"
      "2\n"
      "3\n"
      "4\n"
      "0\n"
      "6\n");

  const auto dense = mmio::ReadDenseMatrixMarket<double>(input);
  RequireEqual(dense.rows, std::size_t{2}, "dense rows failed");
  RequireEqual(dense.cols, std::size_t{3}, "dense columns failed");
  RequireEqual(dense(0, 0), 1.0, "dense value 0,0 failed");
  RequireEqual(dense(1, 0), 2.0, "dense value 1,0 failed");
  RequireEqual(dense(0, 1), 3.0, "dense value 0,1 failed");
  RequireEqual(dense(1, 1), 4.0, "dense value 1,1 failed");
  RequireEqual(dense(0, 2), 0.0, "dense value 0,2 failed");
  RequireEqual(dense(1, 2), 6.0, "dense value 1,2 failed");
}

void TestDenseSymmetricRead() {
  std::istringstream input(
      "%%MatrixMarket matrix array real symmetric\n"
      "2 2\n"
      "1\n"
      "2\n"
      "3\n");

  const auto dense = mmio::ReadDenseMatrixMarket<double>(input);
  RequireEqual(dense(0, 0), 1.0, "symmetric dense diagonal failed");
  RequireEqual(dense(1, 0), 2.0, "symmetric dense lower failed");
  RequireEqual(dense(0, 1), 2.0, "symmetric dense upper failed");
  RequireEqual(dense(1, 1), 3.0, "symmetric dense diagonal 2 failed");
}

}  // namespace

int main() {
  TestStringHelpers();
  TestHeaderParsing();
  TestReadCoordinateReal();
  TestPatternSymmetricExpansion();
  TestComplexHermitianExpansion();
  TestSparseConversions();
  TestRoundTripCoordinate();
  TestDenseArrayRead();
  TestDenseSymmetricRead();
  return 0;
}
