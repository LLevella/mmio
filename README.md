# mmio
Library for Matrix Market reading, writing, analyzing, converting in popular sparse formats

## COO 

Coordinate matrices format is described by three arrays:
* mtx - elements values, 
* imtx - elements' rows' indexes, 
* jmtx - elements column's indexes,
* nnz - number of non-zero elements in the matrix.

## CSR 
Compressed Sparse Row format is specified by 4 arrays:
* mtx - elements values, 
* imtx - indexes of the row's first element in the a, 
* jmtx - elements column's numbers,
* nnz - number of non-zero elements in the matrix.
