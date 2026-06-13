#include "communication/types/block_row.h"

MPI_Datatype block_row_type = MPI_DATATYPE_NULL;

void init_block_row_datatype(size_t row_size) {
  if (block_row_type != MPI_DATATYPE_NULL) {
    return;
  }

  MPI_Type_contiguous(row_size, MPI_FLOAT, &block_row_type);
  MPI_Type_commit(&block_row_type);
}

void free_block_row_datatype() {
  if (block_row_type == MPI_DATATYPE_NULL) {
    return;
  }

  MPI_Type_free(&block_row_type);
}