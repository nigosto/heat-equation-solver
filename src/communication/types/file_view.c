#include "communication/types/file_view.h"
#include "constants.h"

void init_file_view_datatype(const Header* header, int lsizes[2], int coords[2]) {
  if (file_view_type != MPI_DATATYPE_NULL) {
    return;
  }

  int starts[2], gsizes[2] = {header->height, header->width};
  starts[0] = lsizes[0] * coords[0];
  starts[1] = lsizes[1] * coords[1];

  MPI_Type_create_subarray(NDIMS, gsizes, lsizes, starts, MPI_ORDER_C, MPI_BYTE, &file_view_type);
  MPI_Type_commit(&file_view_type);
}

void free_file_view_datatype() {
  if (file_view_type == MPI_DATATYPE_NULL) {
    return;
  }

  MPI_Type_free(&file_view_type);
}