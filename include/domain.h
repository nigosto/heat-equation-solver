#ifndef DOMAIN_H
#define DOMAIN_H

#include "pgm/grid.h"
#include "pgm/header.h"

typedef struct {
  Header header;
  Grid grid;
  int coords[2];
} Domain;

void update_temperature(Domain* domain, float r);
void free_domain(Domain* domain);

#endif