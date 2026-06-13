#include "communication/types/block_column.h"

void init_block_column_datatype(size_t column_size, size_t row_size) {
  if (block_column_type != MPI_DATATYPE_NULL) {
    return;
  }

  MPI_Type_vector(column_size, 1, row_size + 2, MPI_FLOAT, &block_column_type);
  MPI_Type_commit(&block_column_type);
}

void free_block_column_datatype() {
  if (block_column_type == MPI_DATATYPE_NULL) {
    return;
  }

  MPI_Type_free(&block_column_type);
}