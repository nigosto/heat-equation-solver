#ifndef CARTESIAN_COMMUNICATOR_H
#define CARTESIAN_COMMUNICATOR_H

#include "mpi.h"

typedef struct {
  MPI_Comm comm;
  int dims[2];
} CartesianCommunicator;

CartesianCommunicator init_cartesian_comm(int size);
void free_cartesian_comm(CartesianCommunicator* cartesian_comm);

#endif