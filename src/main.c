#include <assert.h>
#include <mpi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define WHITESPACE " \n\t\r"
#define BUFFER_SIZE 255

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

typedef struct {
  size_t width;
  size_t height;
  size_t length;
  int max_value;
} Header;

char* skip_comments(char* content) {
  while (*content == '#') {
    content = strpbrk(content, WHITESPACE) + 1;
  }
  return content;
}

Header parse_header(MPI_File file, MPI_Status *status) {
  char buffer[BUFFER_SIZE];
  Header header;

  MPI_File_read(file, buffer, BUFFER_SIZE, MPI_CHAR, status);

  char* ptr = strpbrk(buffer, WHITESPACE) + 1;
  ptr = skip_comments(ptr);
  header.width = strtol(ptr, &ptr, 10);
  ptr = skip_comments(ptr);
  header.height = strtol(ptr, &ptr, 10);
  ptr = skip_comments(ptr);
  header.max_value = strtol(ptr, &ptr, 10);
  ++ptr;

  header.length = ptr - buffer;
  return header;
}

void parse_pixels(uint8_t* bytes, size_t size, int max_value, float* block) {
  for (size_t i = 0; i < size; ++i) {
    block[i] = (float)bytes[i] / max_value * 100.0f;
  }
}

void to_pixels(float* block, size_t size, int max_value, uint8_t* bytes) {
  for (size_t i = 0; i < size; ++i) {
    bytes[i] = (uint8_t)((block[i] / 100.0f) * max_value);
  }
}

int main(int argc, char** argv) {  
  float alpha = 0.01, dh = 1.0, dt = 0.1;
  assert(dt < dh * dh / (4 * alpha));

  float r = alpha * dt / (dh * dh);

  if (argc != 3) {
    fprintf(stderr, "Invalid number of arguments. Please specify input and output file\n");
    return 1;
  }
  
  MPI_Init(&argc, &argv);
  
  int rank, size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Comm cartesian_comm;
  int ndims = 2;
  int dims[2] = {0, 0}, periods[2] = {false, false};
  MPI_Dims_create(size, ndims, dims);
  MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, true, &cartesian_comm);
  MPI_Comm_rank(cartesian_comm, &rank);

  MPI_File input_file;
  MPI_Status status;
  MPI_File_open(cartesian_comm, argv[1], MPI_MODE_RDONLY, MPI_INFO_NULL, &input_file);
  
  Header header;
  if (rank == 0) {
    header = parse_header(input_file, &status);
  }

  MPI_Datatype header_type;
  int blens[] = {1, 1, 1, 1};
  MPI_Aint displs[] = {
    offsetof(Header, width),
    offsetof(Header, height),
    offsetof(Header, length),
    offsetof(Header, max_value),
  };
  MPI_Datatype types[] = {MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_INT};
  MPI_Type_create_struct(4, blens, displs, types, &header_type);
  MPI_Type_commit(&header_type);

  MPI_Bcast(&header, 1, header_type, 0, cartesian_comm);

  MPI_Type_free(&header_type);

  int lsizes[2], starts[2], coords[2];

  lsizes[0] = header.height / dims[0];
  lsizes[1] = header.width / dims[1];
  
  MPI_Cart_coords(cartesian_comm, rank, ndims, coords);
  starts[0] = lsizes[0] * coords[0];
  starts[1] = lsizes[1] * coords[1];

  MPI_Datatype file_view_type;
  int gsizes[2] = {header.height, header.width};
  MPI_Type_create_subarray(ndims, gsizes, lsizes, starts, MPI_ORDER_C, MPI_BYTE, &file_view_type);
  MPI_Type_commit(&file_view_type);
  MPI_File_set_view(input_file, header.length, MPI_BYTE, file_view_type, "native", MPI_INFO_NULL);
  
  int local_block_size = (lsizes[0] + 2) * (lsizes[1] + 2);
  uint8_t* bytes = calloc(local_block_size, sizeof(uint8_t));

  gsizes[0] = lsizes[0] + 2;
  gsizes[1] = lsizes[1] + 2;
  starts[0] = 1;
  starts[1] = 1;
  
  MPI_Datatype block_type;
  MPI_Type_create_subarray(ndims, gsizes, lsizes, starts, MPI_ORDER_C, MPI_BYTE, &block_type);
  MPI_Type_commit(&block_type);

  MPI_File_read_all(input_file, bytes, 1, block_type, &status);

  float* local_block = malloc(local_block_size * sizeof(float));
  parse_pixels(bytes, local_block_size, header.max_value, local_block);
  float* updated_local_block = malloc(local_block_size * sizeof(float));
  memcpy(updated_local_block, local_block, local_block_size * sizeof(float));

  MPI_Datatype block_row_type, block_column_type;
  MPI_Type_contiguous(lsizes[1], MPI_FLOAT, &block_row_type);
  MPI_Type_commit(&block_row_type);
  
  MPI_Type_vector(lsizes[0], 1, gsizes[1], MPI_FLOAT, &block_column_type);
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

  MPI_File output_file;
  MPI_File_open(cartesian_comm, argv[2], MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &output_file);
  
  char output_header[BUFFER_SIZE];
  sprintf(output_header, "P5\n%ld %ld\n%d\n", header.width, header.height, header.max_value);
  size_t output_header_length = strlen(output_header);
  if (rank == 0) {
    MPI_File_write(output_file, output_header, output_header_length, MPI_CHAR, &status);
  }
  
  MPI_File_set_view(output_file, output_header_length, MPI_BYTE, file_view_type, "native", MPI_INFO_NULL);

  to_pixels(local_block, local_block_size, header.max_value, bytes);

  MPI_File_write_all(output_file, bytes, 1, block_type, &status);

  MPI_File_close(&output_file);

  MPI_Type_free(&block_column_type);
  MPI_Type_free(&block_row_type);

  free(bytes);
  free(local_block);
  free(updated_local_block);
  MPI_Type_free(&block_type);
  MPI_Type_free(&file_view_type);
  MPI_File_close(&input_file);
  MPI_Comm_free(&cartesian_comm);
  MPI_Finalize();

  return 0;
}