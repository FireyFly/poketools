#include <ctype.h>
#include <stdio.h>

#include "poketools.h"

#define FMT_END "\x1B[m"

/** SGR string to format the byte `v`. */
const char *format_of(u32 v) {
  return v ==  0x00? "\x1B[38;5;238m"
       : v ==  0xFF? "\x1B[38;5;167m"
       : v <   0x20? "\x1B[38;5;150m"
       : v >=  0x7F? "\x1B[38;5;141m"
       :             FMT_END;
}

/** Hexdump `n` bytes at `p`. */
void hexdump(void *p_, int n) {
  u8 *p = p_;
  int cols = 16, i;

  for (i = 0; i < n; i += cols) {
    // Offset
    printf("    %04x ", i);

    // Read line
    int line[cols];
    for (int j = 0; j < cols; j++) {
      line[j] = i + j < n? *p++ : -1;
    }

    // Print hex area
    for (int j = 0; j < cols; j++) {
      int v = line[j];
      if (v >= 0) printf(" %s%02x" FMT_END, format_of(v), v);
      else printf("   ");
      if (j % 8 == 7) printf(" ");
    }
    printf(" ");

    // Print character area
    for (int j = 0; j < cols; j++) {
      int v = line[j];
      if (v >= 0) printf("%s%c" FMT_END, format_of(v), isprint(v)? v : '.');
      else printf(" ");
      if (j % 8 == 7) putchar(" \n"[j == cols - 1]);
    }
  }

  if (i % cols != 0) putchar('\n');
}

