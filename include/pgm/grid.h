#ifndef GRID_H
#define GRID_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_TEMPERATURE 100.0f

typedef struct {
  float *block, *auxiliary_block;
  size_t interior_rows, interior_columns;
} Grid;

Grid from_raw_bytes(const uint8_t* bytes, size_t rows, size_t columns, int max_value);
uint8_t* to_raw_bytes(const Grid* grid, int max_value);

#endif