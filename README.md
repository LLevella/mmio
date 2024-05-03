# mmio
Library for Matrix Market reading, writing, analyzing, converting in popular sparse formats

##COO 

Coordinate matrices format is described by three arrays:
* a - elements values, 
* ia - elements' rows' numbers, 
* ja - elements column's numbers,
* nnz - number of non-zero elements in the matrix.
