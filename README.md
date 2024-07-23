# mmio
Library for Matrix Market reading, writing, analyzing, converting in popular sparse formats

## COO 

Coordinate matrices format is described by three arrays:
* a - elements values, 
* ia - elements' rows' numbers, 
* ja - elements column's numbers,
* nnz - number of non-zero elements in the matrix.

## CSR 
Compressed Sparse Row format is specified by 4 arrays:
* a - elements values, 
* ia - numbers of the row's first element in the a, 
* ja - elements column's numbers,
* nnz - number of non-zero elements in the matrix.
