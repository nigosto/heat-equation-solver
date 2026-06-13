#include <stdbool.h>
#include "communication/communicators/cartesian.h"

CartesianCommunicator init_cartesian_comm(int size) {
  CartesianCommunicator cartesian_comm = {
    .dims = {0, 0}
  };
  
  int ndims = 2, periods[2] = {false, false};

  MPI_Dims_create(size, ndims, cartesian_comm.dims);
  MPI_Cart_create(MPI_COMM_WORLD, ndims, cartesian_comm.dims, periods, true, &cartesian_comm.comm);

  return cartesian_comm;
}

void free_cartesian_comm(CartesianCommunicator* cartesian_comm) {
  MPI_Comm_free(&cartesian_comm->comm);
}