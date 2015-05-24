#ifndef SCRIPT_H
#define SCRIPT_H

#include "../poketools.h"

#include <stdio.h>


//-- Types ----------------------------------------------------------
//-- Code section
struct code_header {
  u32 section_size;
  u32 magic; // 0x0A0AF1E0
  u16 unk1;
  u16 unk2;
  u32 header_size; // 0x60
  u32 unk4;
  u32 extracted_size;
  u32 unk5;
  u32 unk6;
} __attribute__((packed));

struct code_block {
  struct code_header *header;
  int ninstrs;
  u32 *instrs;
};


//-- Debug section
struct debug_header {
  u32 section_size;
  u32 magic; // 0x0A0AF1EF
  u16 count_unk1;
  u16 count_files;
  u16 count_linenos;
  u16 count_symbols;
  u16 count_types;
  u32 unk1;
} __attribute__((packed));

struct debug_file {
  u32 start;
  char *name;
};

struct debug_lineno {
  u32 start;
  u32 lineno;
} __attribute__((packed));

struct debug_symbol {
  u32 id;
  u16 unk1;
  u32 start;
  u32 end;
  u32 type;
  char *name;
};

struct debug_type {
  u32 id;
  char *name;
};

struct debug_block {
  struct debug_header *header;
  int nfiles;
  struct debug_file *files;
  int nlinenos;
  struct debug_lineno *linenos;
  int nsymbols;
  struct debug_symbol *symbols;
  int ntypes;
  struct debug_type *types;
};


//-- Functions ------------------------------------------------------
/** Reads a (newly-allocated) code section from `f` and returns it. */
struct code_block *read_code_block(FILE *f);

/** Reads a (newly-allocated) debug section from `f` and returns it. */
struct debug_block *read_debug_block(FILE *f);

/** Comparator for symbols.  Compares primarily by type (asc), secondarily by
 *  start position (asc), and finally by ID (asc).  */
int symbols_comparator(const void *sym1, const void *sym2);

/** Look up a symbol from a debug section `debug`.  Finds the symbol with the
 *  given `id`, of the given `type`, that is defined at position `pos`
 *  (ignored for functions).
 */
const struct debug_symbol *lookup_sym(struct debug_block *debug,
                                      int id, int type, int pos);

#endif
