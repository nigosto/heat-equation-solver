#include "pgm/grid.h"
#include <stddef.h>
#include <stdlib.h>

Grid from_raw_bytes(const uint8_t* bytes, size_t rows, size_t columns, int max_value) {
  size_t size = rows * columns;
  Grid grid = {
    .interior_rows = rows,
    .interior_columns = columns,
    .block = malloc(size * sizeof(float)),
    .auxiliary_block = malloc(size * sizeof(float))
  };

  for (size_t i = 0; i < size; ++i) {
    grid.block[i] = (float)bytes[i] / max_value * MAX_TEMPERATURE;
  }

  return grid;
}

uint8_t* to_raw_bytes(const Grid* grid, int max_value) {
  size_t size = (grid->interior_rows + 2) * (grid->interior_columns + 2); 
  uint8_t* bytes = malloc(size * sizeof(uint8_t));

  for (size_t i = 0; i < size; ++i) {
    bytes[i] = (uint8_t)((grid->block[i] / MAX_TEMPERATURE) * max_value);
  }

  return bytes;
}
