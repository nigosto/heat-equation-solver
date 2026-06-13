#include "pgm/header.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WHITESPACE " \n\t\r"

static char* skip_comments(char* content) {
  while (*content == '#') {
    content = strpbrk(content, WHITESPACE) + 1;
  }
  return content;
}

Header parse_header(const char* raw_str) {
  Header header;

  char* current = strpbrk(raw_str, WHITESPACE) + 1;
  current = skip_comments(current);
  header.width = strtol(current, &current, 10);
  current = skip_comments(current);
  header.height = strtol(current, &current, 10);
  current = skip_comments(current);
  header.max_value = strtol(current, &current, 10);
  ++current;

  header.length = current - raw_str;
  return header;
}

size_t stringify_header(const Header* header, char* str) {
  sprintf(str, "P5\n%ld %ld\n%d\n", header->width, header->height, header->max_value);  
  return strlen(str);
}
