#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "script.h"
#include "../poketools.h"

#define SEXT(x,b) ((!((x) >> (b)) - 1) << (b) | (x))


//-- Helpers --------------------------------------------------------
/** Duplicates memory. */
void *memdup(void *p, int nbytes) {
  void *res = malloc(nbytes);
  memcpy(res, p, nbytes);
  return res;
}

/** Reads a null-terminated string into `buf` (buffer of length `n`). */
void read_string(char *buf, int n, FILE *f) {
  int ch, i;

  for (i = 0; i < n; i++) {
    ch = fgetc(f);
    if (ch == 0x00) break;
    buf[i] = ch;
  }
  buf[i] = 0;

  return;
}


//-- Code section ---------------------------------------------------
/** Reads a (newly-allocated) code section from `f` and returns it. */
struct code_block *read_code_block(FILE *f) {
  long section_start = ftell(f);

  //-- Read header
  struct code_header hd_code;
  fread(&hd_code, sizeof(struct code_header), 1, f);
  assert(hd_code.magic == 0x0A0AF1E0);

  // Read "extra"/unknown bytes
  int nextra = (hd_code.header_size - 0x20) / sizeof(u32);
  u32 *extra = malloc(nextra * sizeof(u32));
  fread(extra, sizeof(u32), nextra, f);

  //-- Read code
  fseek(f, section_start + hd_code.header_size, SEEK_SET);
  long code_start = ftell(f);

  // Extract
  int extracted_length = (hd_code.extracted_size - hd_code.header_size) / sizeof(u32),
      code_length      = (hd_code.extracted_code_size - hd_code.header_size) / sizeof(u32);

  u32 *extracted = malloc(extracted_length * sizeof(u32));

  // Read & decompress the instructions
  u32 i = 0, j = 0, x = 0;
  while (i < extracted_length) {
    int byte = fgetc(f),
        v = byte & 0x7F,
        final = (byte & 0x80) == 0;
    if (++j == 1) x = SEXT(v, 6);
    else x = x << 7 | v;
    if (final) {
      extracted[i++] = x;
      j = 0;
    }
  }

  //-- Return section struct
  struct code_block *res = malloc(sizeof(struct code_block));
  res->header = memdup(&hd_code, sizeof(struct code_header));
  res->nextra = nextra;
  res->extra = extra;
  res->ninstrs = code_length;
  res->instrs = extracted;
  res->nmovement = extracted_length - code_length;
  res->movement = extracted + code_length;

  return res;
}


//-- Debug section --------------------------------------------------
struct debug_raw_symbol {
  u32 id;
  u16 unk1;
  u32 start;
  u32 end;
  u32 type;
} __attribute__((packed));

/** Reads a (newly-allocated) debug section from `f` and returns it. */
struct debug_block *read_debug_block(FILE *f) {
  //-- Read header
  struct debug_header hd;
  fread(&hd, sizeof(struct debug_header), 1, f);
  assert(hd.magic == 0x0A0AF1EF);
  assert(hd.count_unk1 == 0); // Not yet supported--haven't seen this yet

  char buf[BUFSIZ];
  int n;

  struct debug_file   *files   = malloc(sizeof(struct debug_file)   * hd.count_files);
  struct debug_lineno *linenos = malloc(sizeof(struct debug_lineno) * hd.count_linenos);
  struct debug_symbol *symbols = malloc(sizeof(struct debug_symbol) * hd.count_symbols);
  struct debug_type   *types   = malloc(sizeof(struct debug_type)   * hd.count_types);

  // Files
  for (int i = 0; i < hd.count_files; i++) {
    u32 start;
    fread(&start, sizeof(u32), 1, f);
    read_string(buf, BUFSIZ, f);
    files[i] = (struct debug_file) { start, strdup(buf) };
  }

  // LineNos
  for (int i = 0; i < hd.count_linenos; i++) {
    u32 start, lineno;
    fread(&start, sizeof(u32), 1, f);
    fread(&lineno, sizeof(u32), 1, f);
    linenos[i] = (struct debug_lineno) { start, lineno };
  }

  // Symbols
  for (int i = 0; i < hd.count_symbols; i++) {
    struct debug_raw_symbol entry;
    fread(&entry, sizeof(struct debug_raw_symbol), 1, f);
    read_string(buf, BUFSIZ, f);
    symbols[i] = (struct debug_symbol) {
                   entry.id, entry.unk1, entry.start, entry.end,
                   entry.type, strdup(buf) };
  }

  // Types
  for (int i = 0; i < hd.count_types; i++) {
    u16 id;
    fread(&id, sizeof(u16), 1, f);
    read_string(buf, BUFSIZ, f);
    types[i] = (struct debug_type) { id, strdup(buf) };
  }

  // Padding
  for (int i = 0; i < 7; i++) {
    u8 pad = fgetc(f);
    assert(pad == 0);
  }

  //-- Return debug struct
  struct debug_block *res = malloc(sizeof(struct debug_block));
  res->header = memdup(&hd, sizeof(struct debug_header));
  res->nfiles = hd.count_files;
  res->files = files;
  res->nlinenos = hd.count_linenos;
  res->linenos = linenos;
  res->nsymbols = hd.count_symbols;
  res->symbols = symbols;
  res->ntypes = hd.count_types;
  res->types = types;

  return res;
}


/** Comparator for symbols.  Compares primarily by type (asc), secondarily by
 *  start position (asc), and finally by ID (asc).  */
int symbols_comparator(const void *sym1_, const void *sym2_) {
  const struct debug_symbol *sym1 = sym1_,
                            *sym2 = sym2_;

  if      (sym1->type < sym2->type) return -1;
  else if (sym1->type > sym2->type) return +1;

  if      (sym1->start < sym2->start) return -1;
  else if (sym1->start > sym2->start) return +1;

  if      (sym1->id < sym2->id) return -1;
  else if (sym1->id > sym2->id) return +1;

  return 0;
}


/** Look up a symbol from a debug section `debug`.  Finds the symbol with the
 *  given `id`, of the given `type`, that is defined at position `pos`
 *  (ignored for functions).
 */
const struct debug_symbol *lookup_sym(struct debug_block *debug,
                                      int id, int type, int pos) {
  for (int i = 0; i < debug->nsymbols; i++) {
    struct debug_symbol *sym = &debug->symbols[i];
 // printf("sym={id=%04x type=%04x start=%04x end=%04x} id=%04x type=%04x pos=%04x\n",
 //        sym->id, sym->type, sym->start, sym->end, id, type, pos);
    if (sym->id == id && sym->type == type
        && (type == 0x0009 || (sym->start <= pos && pos < sym->end))) {
      return sym;
    }
  }
  return NULL;
}
