#ifndef PGM_HEADER_H
#define PGM_HEADER_H

#include <stddef.h>

typedef struct {
  size_t width;
  size_t height;
  size_t length;
  int max_value;
} Header;

Header parse_header(const char* raw_str);
size_t stringify_header(const Header* header, char* str);

#endif