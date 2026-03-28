#include "mmio.h"

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>

namespace {

std::size_t ParseArg(char **argv, int index, std::size_t fallback) {
  if (argv[index] == nullptr) return fallback;
  return static_cast<std::size_t>(std::strtoull(argv[index], nullptr, 10));
}

template <typename F>
long long MeasureMicros(F &&function) {
  const auto start = std::chrono::steady_clock::now();
  function();
  const auto stop = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(stop - start)
      .count();
}

mmio::CooMatrix<double> MakeMatrix(std::size_t dimension, std::size_t nnz) {
  mmio::CooMatrix<double> matrix(dimension, dimension);
  matrix.reserve(nnz);
  for (std::size_t entry = 0; entry < nnz; ++entry) {
    const auto row = (entry * 37 + 11) % dimension;
    const auto col = (entry * 97 + 23) % dimension;
    matrix.push_back(row, col, static_cast<double>((entry % 17) + 1));
  }
  return matrix;
}

}  // namespace

int main(int argc, char **argv) {
  const auto dimension = argc > 1 ? ParseArg(argv, 1, 1024) : 1024;
  const auto nnz = argc > 2 ? ParseArg(argv, 2, 100000) : 100000;

  const auto matrix = MakeMatrix(dimension == 0 ? 1 : dimension, nnz);

  std::size_t csr_nnz = 0;
  const auto sorted_csr_us = MeasureMicros([&] {
    const auto csr = mmio::ToCSR(matrix);
    csr_nnz = csr.nnz();
  });

  std::size_t preserve_csr_nnz = 0;
  const auto preserve_csr_us = MeasureMicros([&] {
    const auto csr = mmio::ToCSR(matrix, mmio::DuplicatePolicy::keep,
                                mmio::SparseOrdering::preserve);
    preserve_csr_nnz = csr.nnz();
  });

  std::size_t csc_nnz = 0;
  const auto sorted_csc_us = MeasureMicros([&] {
    const auto csc = mmio::ToCSC(matrix);
    csc_nnz = csc.nnz();
  });

  std::cout << "dimension=" << matrix.rows << " nnz=" << matrix.nnz()
            << " csr_sorted_us=" << sorted_csr_us
            << " csr_preserve_us=" << preserve_csr_us
            << " csc_sorted_us=" << sorted_csc_us
            << " csr_nnz=" << csr_nnz
            << " csr_preserve_nnz=" << preserve_csr_nnz
            << " csc_nnz=" << csc_nnz << '\n';

  return preserve_csr_nnz == matrix.nnz() && csc_nnz <= matrix.nnz() ? 0 : 1;
}
