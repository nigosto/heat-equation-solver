#ifndef BLOCK_DATATYPE_H
#define BLOCK_DATATYPE_H

#include "mpi.h"

extern MPI_Datatype block_type;

void init_block_datatype(int lsizes[2]);
void free_block_datatype();

#endif