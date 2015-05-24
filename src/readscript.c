#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poketools.h"
#include "script_pp.h"
#include "formats/script.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    fprintf(stderr, "Couldn't open '%s' for reading.", argv[1]);
    return 2;
  }

  struct code_block *code = NULL;
  struct debug_block *debug = NULL;

  //-- Read sections
  while (1) {
    long section_start = ftell(f);

    u32 size, magic;
    fread(&size,  sizeof(u32), 1, f);
    fread(&magic, sizeof(u32), 1, f);

    if (feof(f) || ferror(f)) break;

    fseek(f, -8L, SEEK_CUR);
    switch (magic) {
      case 0x0A0AF1E0: code = read_code_block(f); break;
      case 0x0A0AF1EF: debug = read_debug_block(f); break;
      default:
        fprintf(stderr, "Bad section magic number at position $%04lx: %08x\n",
                        ftell(f), magic);
        return 2;
    }

    // Check if `read_*_block` read the entire section properly.
    long section_end = ftell(f);

    if (section_end != section_start + size) {
      fprintf(stderr, "\x1B[33mwarning: section not read properly (size delta is %ld)\x1B[m\n", 
              section_end - (section_start + size));
    }

    fseek(f, section_start + size, SEEK_SET);
  }

  //-- Print code (or debug if only debug info)
  switch ((code != NULL) << 1 | (debug != NULL)) {
    case 3: print_debug_code(code, debug); break;
    case 2: print_code(code); break;
    case 1: print_debug(debug); break;
    default:
      fprintf(stderr, "No blocks read!\n");
  }
}
