#include <assert.h>
#include <mpi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define true 1 == 1
#define false 0 == 1

#define GRID_SIZE 120

void init_grid() {
  size_t i, j;
  float value;
  FILE* file;

  file = fopen("input.bin", "wb");

  value = 100.0;
  for (i = 0; i < GRID_SIZE; ++i) {
    fwrite(&value, sizeof(float), 1, file);
  }

  for (i = 1; i < GRID_SIZE; ++i) {
    value = 50.0;
    fwrite(&value, sizeof(float), 1, file);

    value = 0.0;
    for (j = 1; j < GRID_SIZE - 1; ++j) {
      fwrite(&value, sizeof(float), 1, file);
    }
    
    value = 50.0;
    fwrite(&value, sizeof(float), 1, file);
  }

  fclose(file);
}

int main(int argc, char** argv) {
  int rank, size, i, j, local_block_size, ndims = 2;
  int dims[2] = {0, 0}, periods[2] = {false, false}, gsizes[2] = {GRID_SIZE, GRID_SIZE};
  int lsizes[2], starts[2], coords[2];
  float *local_block;
  
  MPI_Comm cartesian_comm;
  MPI_Datatype block_type;
  MPI_File input_file;
  MPI_Info info;
  MPI_Status status;

  float alpha = 0.01, dh = 1.0, dt = 0.1;
  
  assert(dt < dh * dh / (4 * alpha));
  
  MPI_Init(&argc, &argv);
  
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    init_grid();
  }
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Dims_create(size, ndims, dims);
  MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, true, &cartesian_comm);
  MPI_Comm_rank(cartesian_comm, &rank);

  MPI_Info_create(&info);
  MPI_File_open(cartesian_comm, "input.bin", MPI_MODE_RDONLY, info, &input_file);
  
  lsizes[0] = GRID_SIZE / dims[0];
  lsizes[1] = GRID_SIZE / dims[1];
  
  MPI_Cart_coords(cartesian_comm, rank, ndims, coords);
  starts[0] = lsizes[0] * coords[0];
  starts[1] = lsizes[1] * coords[1];

  MPI_Type_create_subarray(ndims, gsizes, lsizes, starts, MPI_ORDER_C, MPI_FLOAT, &block_type);
  MPI_Type_commit(&block_type);
  MPI_File_set_view(input_file, 0, MPI_FLOAT, block_type, "native", info);
  
  local_block_size = lsizes[0] * lsizes[1];
  local_block = malloc(sizeof(float) * local_block_size);
  MPI_File_read_all(input_file, local_block, local_block_size, MPI_FLOAT, &status);

  free(local_block);
  MPI_Type_free(&block_type);
  MPI_File_close(&input_file);
  MPI_Info_free(&info);
  MPI_Comm_free(&cartesian_comm);
  MPI_Finalize();

  return 0;
}