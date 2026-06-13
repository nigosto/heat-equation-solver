#ifndef BLOCK_ROW_DATATYPE_H
#define BLOCK_ROW_DATATYPE_H

#include "mpi.h"
#include <stddef.h>

static MPI_Datatype block_row_type = MPI_DATATYPE_NULL;

void init_block_row_datatype(size_t row_size);
void free_block_row_datatype();

#endif