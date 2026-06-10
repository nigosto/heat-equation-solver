#include <assert.h>
#include <mpi.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define GRID_SIZE 120

void init_grid() {
  FILE* file = fopen("input.bin", "wb");

  float value = 100.0;
  for (int i = 0; i < GRID_SIZE; ++i) {
    fwrite(&value, sizeof(float), 1, file);
  }

  for (int i = 1; i < GRID_SIZE; ++i) {
    value = 50.0;
    fwrite(&value, sizeof(float), 1, file);

    value = 0.0;
    for (int j = 1; j < GRID_SIZE - 1; ++j) {
      fwrite(&value, sizeof(float), 1, file);
    }
    
    value = 50.0;
    fwrite(&value, sizeof(float), 1, file);
  }

  fclose(file);
}

void update_temperature(const float* grid, int rows, int columns, float r, int coords[2], int dims[2], float* updated_grid) {
  bool is_first_row = coords[0] == 0;
  bool is_first_column = coords[1] == 0;
  bool is_last_row = coords[0] == dims[0] - 1;
  bool is_last_column = coords[1] == dims[1] - 1;

  for (int i = is_first_row ? 2 : 1; i < rows - (is_last_row ? 2 : 1); ++i) {
    for (int j = is_first_column ? 2 : 1; j < columns - (is_last_column ? 2 : 1); ++j) {
      updated_grid[i * columns + j] = grid[i * columns + j] + r * (
        grid[i * columns + j + 1] + 
        grid[i * columns + j - 1] +
        grid[(i - 1) * columns + j] +
        grid[(i + 1) * columns + j] -
        4 * grid[i * columns + j]
      );
    }
  }
}

int main(int argc, char** argv) {  
  float alpha = 0.01, dh = 1.0, dt = 0.1;
  assert(dt < dh * dh / (4 * alpha));

  float r = alpha * dt / (dh * dh);
  
  MPI_Init(&argc, &argv);
  
  int rank, size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    init_grid();
  }
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Comm cartesian_comm;
  int ndims = 2;
  int dims[2] = {0, 0}, periods[2] = {false, false};
  MPI_Dims_create(size, ndims, dims);
  MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, true, &cartesian_comm);
  MPI_Comm_rank(cartesian_comm, &rank);

  MPI_File input_file;
  MPI_File_open(cartesian_comm, "input.bin", MPI_MODE_RDWR, MPI_INFO_NULL, &input_file);
  
  int lsizes[2], starts[2], coords[2];

  lsizes[0] = GRID_SIZE / dims[0];
  lsizes[1] = GRID_SIZE / dims[1];
  
  MPI_Cart_coords(cartesian_comm, rank, ndims, coords);
  starts[0] = lsizes[0] * coords[0];
  starts[1] = lsizes[1] * coords[1];

  MPI_Datatype file_view_type;
  int gsizes[2] = {GRID_SIZE, GRID_SIZE};
  MPI_Type_create_subarray(ndims, gsizes, lsizes, starts, MPI_ORDER_C, MPI_FLOAT, &file_view_type);
  MPI_Type_commit(&file_view_type);
  MPI_File_set_view(input_file, 0, MPI_FLOAT, file_view_type, "native", MPI_INFO_NULL);
  
  int local_block_size = (lsizes[0] + 2) * (lsizes[1] + 2);
  float* local_block = calloc(local_block_size, sizeof(float));
  float* updated_local_block = malloc(local_block_size * sizeof(float));

  gsizes[0] = lsizes[0] + 2;
  gsizes[1] = lsizes[1] + 2;
  starts[0] = 1;
  starts[1] = 1;
  
  MPI_Datatype block_type;
  MPI_Type_create_subarray(ndims, gsizes, lsizes, starts, MPI_ORDER_C, MPI_FLOAT, &block_type);
  MPI_Type_commit(&block_type);

  MPI_Status status;
  MPI_File_read_all(input_file, local_block, 1, block_type, &status);

  memcpy(updated_local_block, local_block, local_block_size * sizeof(float));

  MPI_Datatype block_row_type, block_column_type_raw;
  MPI_Type_contiguous(lsizes[1], MPI_FLOAT, &block_row_type);
  MPI_Type_commit(&block_row_type);
  
  MPI_Type_vector(lsizes[0], 1, gsizes[1], MPI_FLOAT, &block_column_type_raw);
  MPI_Aint lb, extent;
  MPI_Type_get_extent(MPI_FLOAT, &lb, &extent);
  MPI_Datatype block_column_type;
  MPI_Type_create_resized(block_column_type_raw, 0, extent, &block_column_type);
  MPI_Type_commit(&block_column_type);

  int sendcounts[4];
  MPI_Aint sdispls[4];
  MPI_Datatype sendtypes[4];
  int recvcounts[4];
  MPI_Aint rdispls[4];
  MPI_Datatype recvtypes[4];

  sendcounts[0] = 1;
  sdispls[0] = (gsizes[1] + 1) * sizeof(float);
  sendtypes[0] = block_row_type;

  sendcounts[1] = 1;
  sdispls[1] = (lsizes[0] * gsizes[1] + 1) * sizeof(float);
  sendtypes[1] = block_row_type;

  sendcounts[2] = 1;
  sdispls[2] = (gsizes[1] + 1) * sizeof(float);
  sendtypes[2] = block_column_type;

  sendcounts[3] = 1;
  sdispls[3] = (gsizes[1] + lsizes[1]) * sizeof(float);
  sendtypes[3] = block_column_type;
  
  recvcounts[0] = 1;
  rdispls[0] = sizeof(float);
  recvtypes[0] = block_row_type;

  recvcounts[1] = 1;
  rdispls[1] = ((lsizes[0] + 1) * gsizes[1] + 1) * sizeof(float);
  recvtypes[1] = block_row_type;

  recvcounts[2] = 1;
  rdispls[2] = gsizes[1] * sizeof(float);
  recvtypes[2] = block_column_type;

  recvcounts[3] = 1;
  rdispls[3] = (gsizes[1] + lsizes[1] + 1) * sizeof(float);
  recvtypes[3] = block_column_type;

  MPI_Request requests[2];
  MPI_Neighbor_alltoallw_init(local_block, sendcounts, sdispls, sendtypes, local_block, recvcounts, rdispls, recvtypes, cartesian_comm, MPI_INFO_NULL, &requests[0]);
  MPI_Neighbor_alltoallw_init(updated_local_block, sendcounts, sdispls, sendtypes, updated_local_block, recvcounts, rdispls, recvtypes, cartesian_comm, MPI_INFO_NULL, &requests[1]);
  
  for (int i = 0; i < 100000; i += 2) {
    MPI_Start(&requests[0]);
    MPI_Wait(&requests[0], &status);
    update_temperature(local_block, gsizes[0], gsizes[1], r, coords, dims, updated_local_block);

    MPI_Start(&requests[1]);
    MPI_Wait(&requests[1], &status);
    update_temperature(updated_local_block, gsizes[0], gsizes[1], r, coords, dims, local_block);
  }

  MPI_Request_free(&requests[0]);
  MPI_Request_free(&requests[1]);

  MPI_File_seek(input_file, 0, MPI_SEEK_SET);
  MPI_File_write_all(input_file, local_block, 1, block_type, &status);

  MPI_Type_free(&block_column_type);
  MPI_Type_free(&block_column_type_raw);
  MPI_Type_free(&block_row_type);

  free(local_block);
  free(updated_local_block);
  MPI_Type_free(&block_type);
  MPI_Type_free(&file_view_type);
  MPI_File_close(&input_file);
  MPI_Comm_free(&cartesian_comm);
  MPI_Finalize();

  return 0;
}