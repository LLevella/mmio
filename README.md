# mmio
Library for Matrix Market reading, writing, analyzing, converting in popular sparse formats

## COO 

Coordinate matrices format is described by three arrays:
* mtx - elements values, 
* imtx - elements' rows' indexes, 
* jmtx - elements column's indexes,
* nnz - number of non-zero elements in the matrix.

## CSR 
Compressed Sparse Row format is specified by 3 arrays:
* mtx - elements values, 
* imtx - indexes of the row's first element in the a, 
* jmtx - elements column's indexes,
* n - martices' rows number 
* nnz - number of non-zero elements in the matrix.

## CSC
Compressed sparse column format is similar to the CSR format, but the columns are used instead the rows. 
It is specified by 3 arrays:
* mtx - elements values, 
* jmtx - indexes of the columns's first element in the a, 
* imtx - elements row's indexes,
* n - martices' columns number 
* nnz - number of non-zero elements in the matrix.

## CSM
Compressed sparse mixed format is similar to the CSR and CSC formats, but upper triangle use CSC and lower use CRS formats. 
It is specified by next arrays:
* d - diagonal elements values,
* l - elements values, 
* il - indexes of the row's first element in the a, 
* jl - elements column's indexes,
* u - elements values, 
* ju - indexes of the columns's first element in the a, 
* iu - elements row's indexes,
* n - martices' rows number, 
* m - martices' columns number, 
* nnzl - number of non-zero elements in the low triangle of the matrix,
* nnzu - number of non-zero elements in the upper triangle of the matrix,
