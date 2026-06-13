#include "domain.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

void update_temperature(Domain* domain, float r) {
  int dims[] = {
    domain->header.height / domain->grid.interior_rows,
    domain->header.width / domain->grid.interior_columns
  };

  bool is_first_row = domain->coords[0] == 0;
  bool is_first_column = domain->coords[1] == 0;
  bool is_last_row = domain->coords[0] == dims[0] - 1;
  bool is_last_column = domain->coords[1] == dims[1] - 1;

  size_t rows = domain->grid.interior_rows + 2;
  size_t columns = domain->grid.interior_columns + 2;
  float *block = domain->grid.block, *auxiliary_block = domain->grid.auxiliary_block;

  memcpy(block, auxiliary_block, rows * columns * sizeof(float));

  #pragma omp parallel for
  for (size_t i = is_first_row ? 2 : 1; i < rows - (is_last_row ? 2 : 1); ++i) {
    #pragma omp simd
    for (size_t j = is_first_column ? 2 : 1; j < columns - (is_last_column ? 2 : 1); ++j) {
      auxiliary_block[i * columns + j] = block[i * columns + j] + r * (
        block[i * columns + j + 1] + 
        block[i * columns + j - 1] +
        block[(i - 1) * columns + j] +
        block[(i + 1) * columns + j] -
        4 * block[i * columns + j]
      );
    }
  }

  memcpy(block, auxiliary_block, rows * columns * sizeof(float));
}

void free_domain(Domain* domain) {
  free(domain->grid.block);
  free(domain->grid.auxiliary_block);
}