#ifndef HEADER_DATATYPE_H
#define HEADER_DATATYPE_H

#include "mpi.h"

extern MPI_Datatype header_type;

void init_header_datatype();
void free_header_datatype();

#endif