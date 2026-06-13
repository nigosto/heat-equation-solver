#include "communication/types/block.h"
#include "constants.h"

MPI_Datatype block_type = MPI_DATATYPE_NULL;

void init_block_datatype(int lsizes[2]) {
  if (block_type != MPI_DATATYPE_NULL) {
    return;
  }

  int gsizes[] = {
    lsizes[0] + 2,
    lsizes[1] + 2
  };
  int starts[] = {1, 1};

  MPI_Type_create_subarray(NDIMS, gsizes, lsizes, starts, MPI_ORDER_C, MPI_BYTE, &block_type);
  MPI_Type_commit(&block_type);
}

void free_block_datatype() {
  if (block_type == MPI_DATATYPE_NULL) {
    return;
  }
  
  MPI_Type_free(&block_type);
}