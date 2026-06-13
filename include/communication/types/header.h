#ifndef HEADER_DATATYPE_H
#define HEADER_DATATYPE_H

#include "mpi.h"

static MPI_Datatype header_type = MPI_DATATYPE_NULL;

void init_header_datatype();
void free_header_datatype();

#endif