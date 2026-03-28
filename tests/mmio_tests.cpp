#include "mmio.h"

#include <complex>
#include <filesystem>
#include <fstream>
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

  RequireMMThrow(
      [] {
        mmio::Header::Parse("%%MatrixMarket vector coordinate real general");
      },
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

  const auto csr_coo = mmio::ToCOO(csr);
  RequireVectorEqual(csr_coo.row_indices, std::vector<std::size_t>{0, 1, 2},
                     "CSR to COO rows failed");
  RequireVectorEqual(csr_coo.col_indices, std::vector<std::size_t>{2, 0, 1},
                     "CSR to COO columns failed");

  const auto csc_coo = mmio::ToCOO(csc);
  RequireVectorEqual(csc_coo.row_indices, std::vector<std::size_t>{1, 2, 0},
                     "CSC to COO rows failed");
  RequireVectorEqual(csc_coo.col_indices, std::vector<std::size_t>{0, 1, 2},
                     "CSC to COO columns failed");

  const auto preserved_csr =
      mmio::ToCSR(matrix, mmio::DuplicatePolicy::keep,
                  mmio::SparseOrdering::preserve);
  RequireVectorEqual(preserved_csr.row_ptr,
                     std::vector<std::size_t>{0, 2, 3, 4},
                     "preserved CSR row_ptr failed");
  RequireVectorEqual(preserved_csr.col_indices,
                     std::vector<std::size_t>{2, 2, 0, 1},
                     "preserved CSR columns failed");
  RequireVectorEqual(preserved_csr.values, std::vector<int>{1, 2, 4, 5},
                     "preserved CSR values failed");
}

void TestDirectCSRCSCReadWrite() {
  std::istringstream input(
      "%%MatrixMarket matrix coordinate integer general\n"
      "3 3 4\n"
      "1 3 1\n"
      "1 3 2\n"
      "2 1 4\n"
      "3 2 5\n");

  const auto csr = mmio::ReadCSRMatrixMarket<int>(input);
  RequireVectorEqual(csr.row_ptr, std::vector<std::size_t>{0, 1, 2, 3},
                     "direct CSR read row_ptr failed");
  RequireVectorEqual(csr.col_indices, std::vector<std::size_t>{2, 0, 1},
                     "direct CSR read columns failed");
  RequireVectorEqual(csr.values, std::vector<int>{3, 4, 5},
                     "direct CSR read values failed");

  std::stringstream csr_output;
  mmio::WriteMatrixMarket(csr_output, csr);
  const auto csr_roundtrip = mmio::ReadMatrixMarket<int>(csr_output);
  RequireVectorEqual(csr_roundtrip.values, std::vector<int>{3, 4, 5},
                     "CSR writer roundtrip failed");

  std::istringstream input_again(
      "%%MatrixMarket matrix coordinate integer general\n"
      "3 3 3\n"
      "1 3 3\n"
      "2 1 4\n"
      "3 2 5\n");
  const auto csc = mmio::ReadCSCMatrixMarket<int>(input_again);
  RequireVectorEqual(csc.col_ptr, std::vector<std::size_t>{0, 1, 2, 3},
                     "direct CSC read col_ptr failed");
  RequireVectorEqual(csc.row_indices, std::vector<std::size_t>{1, 2, 0},
                     "direct CSC read rows failed");
  RequireVectorEqual(csc.values, std::vector<int>{4, 5, 3},
                     "direct CSC read values failed");
}

void TestSparseValidation() {
  mmio::CsrMatrix<int> invalid_csr;
  invalid_csr.rows = 2;
  invalid_csr.cols = 2;
  invalid_csr.row_ptr = {0, 2, 1};
  invalid_csr.col_indices = {0};
  invalid_csr.values = {1};
  RequireMMThrow([&] { invalid_csr.validate(); },
                 "invalid CSR row_ptr was accepted");

  mmio::CscMatrix<int> invalid_csc;
  invalid_csc.rows = 2;
  invalid_csc.cols = 2;
  invalid_csc.col_ptr = {0, 1, 1};
  invalid_csc.row_indices = {2};
  invalid_csc.values = {1};
  RequireMMThrow([&] { invalid_csc.validate(); },
                 "invalid CSC row index was accepted");
}

void TestStreamedSparseFileReads() {
  const auto path =
      std::filesystem::temp_directory_path() / "mmio_streamed_sparse_test.mtx";
  {
    std::ofstream file(path);
    file << "%%MatrixMarket matrix coordinate integer symmetric\n"
         << "3 3 3\n"
         << "1 1 10\n"
         << "3 1 7\n"
         << "2 2 5\n";
  }

  mmio::ReadOptions options;
  options.duplicate_policy = mmio::DuplicatePolicy::keep;
  options.sparse_ordering = mmio::SparseOrdering::preserve;

  const auto csr = mmio::ReadCSRMatrixMarketFileStreamed<int>(
      path.string(), nullptr, options);
  RequireVectorEqual(csr.row_ptr, std::vector<std::size_t>{0, 2, 3, 4},
                     "streamed CSR row_ptr failed");
  RequireVectorEqual(csr.col_indices, std::vector<std::size_t>{0, 2, 1, 0},
                     "streamed CSR columns failed");
  RequireVectorEqual(csr.values, std::vector<int>{10, 7, 5, 7},
                     "streamed CSR values failed");

  const auto csc = mmio::ReadCSCMatrixMarketFileStreamed<int>(
      path.string(), nullptr, options);
  RequireVectorEqual(csc.col_ptr, std::vector<std::size_t>{0, 2, 3, 4},
                     "streamed CSC col_ptr failed");
  RequireVectorEqual(csc.row_indices, std::vector<std::size_t>{0, 2, 1, 0},
                     "streamed CSC rows failed");
  RequireVectorEqual(csc.values, std::vector<int>{10, 7, 5, 7},
                     "streamed CSC values failed");

  std::filesystem::remove(path);
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

void TestSymmetricWriters() {
  mmio::CooMatrix<int> symmetric(2, 2);
  symmetric.push_back(0, 0, 1);
  symmetric.push_back(0, 1, 5);
  symmetric.push_back(1, 0, 5);
  symmetric.push_back(1, 1, 2);

  mmio::WriteOptions options;
  options.header =
      mmio::Header(mmio::StorageTypes::sparse, mmio::DataTypes::integer,
                   mmio::MatrixTypes::symmetric);

  std::stringstream output;
  mmio::WriteMatrixMarket(output, symmetric, options);
  const auto serialized = output.str();
  Require(serialized.find("2 2 3\n") != std::string::npos,
          "symmetric writer did not emit triangular nnz");
  Require(serialized.find("1 2 5\n") == std::string::npos,
          "symmetric writer emitted upper triangle");
  Require(serialized.find("2 1 5\n") != std::string::npos,
          "symmetric writer did not emit lower triangle");

  mmio::CooMatrix<int> skew(2, 2);
  skew.push_back(0, 0, 1);
  mmio::WriteOptions skew_options;
  skew_options.header =
      mmio::Header(mmio::StorageTypes::sparse, mmio::DataTypes::integer,
                   mmio::MatrixTypes::skewSymmetric);
  RequireMMThrow(
      [&] {
        std::stringstream skew_output;
        mmio::WriteMatrixMarket(skew_output, skew, skew_options);
      },
      "skew-symmetric diagonal value was accepted");
}

void TestMalformedInputs() {
  const std::vector<std::string> malformed = {
      "",
      "%%MatrixMarket matrix coordinate real general\n1 1\n",
      "%%MatrixMarket matrix coordinate real general\n1 1 1\n0 1 1\n",
      "%%MatrixMarket matrix coordinate real general\n1 1 1\n1 2 1\n",
      "%%MatrixMarket matrix coordinate complex general\n1 1 1\n1 1 1\n",
      "%%MatrixMarket matrix array pattern general\n1 1\n1\n",
      "not a matrix market file\n"};

  for (const auto &content : malformed) {
    RequireMMThrow(
        [&] {
          std::istringstream input(content);
          (void)mmio::ReadMatrixMarket<double>(input);
        },
        "malformed input was accepted: " + content);
  }

  for (int seed = 0; seed < 64; ++seed) {
    std::ostringstream generated;
    generated << "%%MatrixMarket matrix coordinate real general\n";
    generated << "3 3 2\n";
    generated << ((seed % 5) - 1) << ' ' << ((seed % 7) - 2) << " value\n";
    generated << "1 1 " << seed << "\n";
    RequireMMThrow(
        [&] {
          std::istringstream input(generated.str());
          (void)mmio::ReadMatrixMarket<double>(input);
        },
        "fuzz-style malformed coordinate was accepted");
  }
}

}  // namespace

int main() {
  TestStringHelpers();
  TestHeaderParsing();
  TestReadCoordinateReal();
  TestPatternSymmetricExpansion();
  TestComplexHermitianExpansion();
  TestSparseConversions();
  TestDirectCSRCSCReadWrite();
  TestSparseValidation();
  TestStreamedSparseFileReads();
  TestRoundTripCoordinate();
  TestDenseArrayRead();
  TestDenseSymmetricRead();
  TestSymmetricWriters();
  TestMalformedInputs();
  return 0;
}
