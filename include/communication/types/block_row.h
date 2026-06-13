#ifndef BLOCK_ROW_DATATYPE_H
#define BLOCK_ROW_DATATYPE_H

#include "mpi.h"
#include <stddef.h>

extern MPI_Datatype block_row_type;

void init_block_row_datatype(size_t row_size);
void free_block_row_datatype();

#endif