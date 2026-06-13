#ifndef FILE_VIEW_DATATYPE_H
#define FILE_VIEW_DATATYPE_H

#include "mpi.h"
#include "pgm/header.h"

static MPI_Datatype file_view_type = MPI_DATATYPE_NULL;

void init_file_view_datatype(const Header* header, int dims[2], int coords[2]);
void free_file_view_datatype();

#endif