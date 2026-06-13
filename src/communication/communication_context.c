#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "communication/communication_context.h"
#include "communication/communicators/cartesian.h"
#include "communication/types/file_view.h"
#include "communication/types/header.h"
#include "communication/types/block.h"
#include "communication/types/block_row.h"
#include "communication/types/block_column.h"
#include "mpi.h"
#include "pgm/grid.h"
#include "pgm/header.h"
#include "constants.h"

static MPI_Status status;

struct CommunicationContext {
  int rank, size;
  CartesianCommunicator cartesian_comm;
  MPI_Request request;
};

void init_communication_context(int* argc, char*** argv, CommunicationContext* ctx) {
  MPI_Init(argc, argv);

  struct CommunicationContext* local_ctx = malloc(sizeof(struct CommunicationContext));

  MPI_Comm_size(MPI_COMM_WORLD, &local_ctx->size);

  CartesianCommunicator cartesian_comm = init_cartesian_comm(local_ctx->size);
  MPI_Comm_rank(cartesian_comm.comm, &local_ctx->rank);

  local_ctx->cartesian_comm = cartesian_comm;

  *ctx = local_ctx;
}

static Header read_header_from_file(CommunicationContext ctx, MPI_File input_file) {
  Header header;
  if (ctx->rank == 0) {
    char buffer[BUFFER_SIZE];
    
    int error = MPI_File_read(input_file, buffer, BUFFER_SIZE, MPI_CHAR, &status);
    if (error != MPI_SUCCESS) {
      fprintf(stderr, "Couldn't read from file\n");
      MPI_Abort(ctx->cartesian_comm.comm, error);
    }

    header = parse_header(buffer);
  }

  init_header_datatype();
  MPI_Bcast(&header, 1, header_type, 0, ctx->cartesian_comm.comm);
  free_header_datatype();

  return header;
}

static Grid read_grid_from_file(CommunicationContext ctx, MPI_File input_file, const Header* header, int lsizes[2]) {
  MPI_File_set_view(input_file, header->length, MPI_BYTE, file_view_type, "native", MPI_INFO_NULL);
  
  size_t rows = lsizes[0] + 2, columns = lsizes[1] + 2;
  uint8_t* bytes = calloc(rows * columns, sizeof(uint8_t));

  int error = MPI_File_read_all(input_file, bytes, 1, block_type, &status);
  if (error != MPI_SUCCESS) {
    fprintf(stderr, "Couldn't read from file\n");
    MPI_Abort(ctx->cartesian_comm.comm, error);
  }

  Grid grid = from_raw_bytes(bytes, rows, columns, header->max_value);
  free(bytes);

  return grid;
}

Domain init_domain_from_file(CommunicationContext ctx, const char* filename) {  
  MPI_File input_file;
  int error = MPI_File_open(ctx->cartesian_comm.comm, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &input_file);
  if (error != MPI_SUCCESS) {
    fprintf(stderr, "Unable to open file %s\n", filename);
    MPI_Abort(ctx->cartesian_comm.comm, error);
  }
  
  Domain domain;
  domain.header = read_header_from_file(ctx, input_file);

  MPI_Cart_coords(ctx->cartesian_comm.comm, ctx->rank, NDIMS, domain.coords);
  init_file_view_datatype(&domain.header, ctx->cartesian_comm.dims, domain.coords);
  
  int lsizes[] = {
    domain.header.height / ctx->cartesian_comm.dims[0],
    domain.header.width / ctx->cartesian_comm.dims[1]
  };
  
  init_block_datatype(lsizes);
  domain.grid = read_grid_from_file(ctx, input_file, &domain.header, lsizes);

  error = MPI_File_close(&input_file);
  if (error != MPI_SUCCESS) {
    fprintf(stderr, "Error while closing file %s\n", filename);
    MPI_Abort(ctx->cartesian_comm.comm, error);
  }

  return domain;
}

void init_halo_exchange(CommunicationContext ctx, const Domain* domain) {
  size_t interior_rows = domain->grid.interior_rows;
  size_t interior_columns = domain->grid.interior_columns;
  size_t total_columns = interior_columns + 2;

  init_block_row_datatype(interior_columns);
  init_block_column_datatype(interior_rows, interior_columns);

  int sendcounts[4], recvcounts[4];
  MPI_Aint sdispls[4], rdispls[4];
  MPI_Datatype sendtypes[4], recvtypes[4];

  sendcounts[0] = 1;
  sdispls[0] = (total_columns + 1) * sizeof(float);
  sendtypes[0] = block_row_type;

  sendcounts[1] = 1;
  sdispls[1] = (interior_rows * interior_columns + 1) * sizeof(float);
  sendtypes[1] = block_row_type;

  sendcounts[2] = 1;
  sdispls[2] = (total_columns + 1) * sizeof(float);
  sendtypes[2] = block_column_type;

  sendcounts[3] = 1;
  sdispls[3] = (total_columns + interior_columns) * sizeof(float);
  sendtypes[3] = block_column_type;
  
  recvcounts[0] = 1;
  rdispls[0] = sizeof(float);
  recvtypes[0] = block_row_type;

  recvcounts[1] = 1;
  rdispls[1] = ((interior_rows + 1) * total_columns + 1) * sizeof(float);
  recvtypes[1] = block_row_type;

  recvcounts[2] = 1;
  rdispls[2] = total_columns * sizeof(float);
  recvtypes[2] = block_column_type;

  recvcounts[3] = 1;
  rdispls[3] = (total_columns + interior_columns + 1) * sizeof(float);
  recvtypes[3] = block_column_type;

  MPI_Neighbor_alltoallw_init(domain->grid.block, sendcounts, sdispls, sendtypes, domain->grid.block, recvcounts, rdispls, recvtypes, ctx->cartesian_comm.comm, MPI_INFO_NULL, &ctx->request);
}

void exchange_halos(CommunicationContext ctx) {
  MPI_Start(&ctx->request);
  MPI_Wait(&ctx->request, &status); 
}

void finalize_halo_exchange(CommunicationContext ctx) {
  MPI_Request_free(&ctx->request);

  free_block_row_datatype();
  free_block_column_datatype();
}

void save_domain_to_file(CommunicationContext ctx, const Domain* domain, const char* filename) {
  MPI_File output_file;
  int error = MPI_File_open(ctx->cartesian_comm.comm, filename, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &output_file);
  if (error != MPI_SUCCESS) {
    fprintf(stderr, "Unable to open file %s\n", filename);
    MPI_Abort(ctx->cartesian_comm.comm, error);
  }

  char output_header[BUFFER_SIZE];
  size_t output_header_length = stringify_header(&domain->header, output_header);
  if (ctx->rank == 0) {
    error = MPI_File_write(output_file, output_header, output_header_length, MPI_BYTE, &status);
    if (error != MPI_SUCCESS) {
      fprintf(stderr, "Couldn't write to file\n");
      MPI_Abort(ctx->cartesian_comm.comm, error);
    }
  }
  
  uint8_t* bytes = to_raw_bytes(&domain->grid, domain->header.max_value);
  
  MPI_File_set_view(output_file, output_header_length, MPI_BYTE, file_view_type, "native", MPI_INFO_NULL);
  error = MPI_File_write_all(output_file, bytes, 1, block_type, &status);
  if (error != MPI_SUCCESS) {
    fprintf(stderr, "Couldn't write to file\n");
    MPI_Abort(ctx->cartesian_comm.comm, error); 
  }
  
  free(bytes);

  error = MPI_File_close(&output_file);
  if (error != MPI_SUCCESS) {
    fprintf(stderr, "Error while closing file %s\n", filename);
    MPI_Abort(ctx->cartesian_comm.comm, error);
  }
}

void free_communication_context(CommunicationContext* ctx) { 
  free_cartesian_comm(&(*ctx)->cartesian_comm);
  free_block_datatype();
  free_file_view_datatype();
  free(*ctx);

  MPI_Finalize();
}