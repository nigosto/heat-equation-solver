#ifndef BLOCK_COLUMN_DATATYPE_H
#define BLOCK_COLUMN_DATATYPE_H

#include "mpi.h"
#include <stddef.h>

static MPI_Datatype block_column_type = MPI_DATATYPE_NULL;

void init_block_column_datatype(size_t column_size, size_t row_size);
void free_block_column_datatype();

#endif