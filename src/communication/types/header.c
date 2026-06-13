#include "communication/types/header.h"
#include "pgm/header.h"

void init_header_datatype() {
  if (header_type != MPI_DATATYPE_NULL) {
    return;
  }

  int blens[] = {1, 1, 1, 1};
  MPI_Aint displs[] = {
    offsetof(Header, width),
    offsetof(Header, height),
    offsetof(Header, length),
    offsetof(Header, max_value),
  };
  MPI_Datatype types[] = {MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_INT};
  MPI_Type_create_struct(4, blens, displs, types, &header_type);
  MPI_Type_commit(&header_type);
}

void free_header_datatype() {
  if (header_type == MPI_DATATYPE_NULL) {
    return;
  }
  
  MPI_Type_free(&header_type);
}