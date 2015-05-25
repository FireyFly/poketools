#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poketools.h"
#include "hexdump.h"
#include "formats/script.h"

#define FMT_COMMENT "\x1B[38;5;243m"
#define FMT_END     "\x1B[m"

/** Prints a (comment) line explaining an instruction operand. */
void print_argument_comment(u16 op, u32 *p, int i, int j, int n, struct debug_block *debug) {
  #define CODE_DELTA(d) ((i - 1)*4 + (int)(d))
  #define PRINT_CODE_DELTA(d) printf("%+4d words (= %04x)", (int)(d)/4, CODE_DELTA(d));
  u32 v = p[i];

  switch (op) {
    case 0x0031: // Relative jumps of various kinds
    case 0x0033:
    case 0x0035:
    case 0x0036:
    case 0x0081:
      printf("  "); PRINT_CODE_DELTA(v);
      break;

    case 0x0082: { // JumpMap
      static int result;
      if (j == 0) {
        printf("  cases = %d", v);
      } else if (j == n - 1 && j % 2 == 1) {
        printf("  else => "); PRINT_CODE_DELTA(v);
      } else if (j % 2 == 1) {
        result = v;
      } else {
        printf("  %4d => ", v); PRINT_CODE_DELTA(result - 4);
      }
    } break;

    default:
      printf("  <?>");
  }

  #undef PRINT_CODE_DELTA
  #undef CODE_DELTA
}

/** Prints a (comment) line explaining an instruction. */
void print_instruction_comment(u32 *p, int i, struct debug_block *debug) {
  u32 v  = p[i];
  u16 vh = v >> 16,
      vl = v & 0xFFFF;

  static u16 curr_op, curr_nargs = 0, curr_remaining = 0;

  printf(FMT_COMMENT "  ; ");
  if (curr_remaining > 0) {
    print_argument_comment(curr_op, p, i, curr_nargs - curr_remaining, curr_nargs, debug);
    curr_remaining--;
    printf(FMT_END);
    return;
  }

  int nargs      =  0,            // #operand ints that follow this instruction
      lookup_var = -1,            // If not -1, lookup a variable of this type and append
      var        = SEXT(vh, 15);  // Variable to lookup (default: high halfword)

  printf(FMT_END);
  switch (vl) { // These are all super WIP
    case 0x002E: printf("Func Begin");                                        break;
    case 0x0030: printf("Return\n");                                          break;
    case 0x0031: printf("Call");              nargs = 1; lookup_var = 0x0009;
                                              var = i*4 + (int) p[i+1];       break;
    case 0x0033: printf("Jump");              nargs = 1;                      break;
    case 0x0035: printf("CondJump");          nargs = 1;                      break;
    case 0x0036: printf("CondJump2");         nargs = 1;                      break;
    case 0x004E: printf("Add?");                                              break;
    case 0x0059: printf("Cleanup?");                                          break;
    case 0x0081: printf("Trampoline");        nargs = 1;                      break;
    case 0x0082: printf("JumpMap");           nargs = p[i+1] * 2 + 2;         break;
    case 0x0087: printf("DoCommand?");        nargs = 2;                      break;
    case 0x0089: printf("LineNo");                                            break;
    case 0x00A2: printf("GetGlobal2 %04hx", vh);         lookup_var = 0x0001; break;
    case 0x00A3: printf("GetGlobal %04hx", vh);          lookup_var = 0x0001; break;
    case 0x00A4: printf("GetLocal %04hx", vh);           lookup_var = 0x0101; break;
    case 0x00AF: printf("SetGlobal %04hx", vh);          lookup_var = 0x0001; break;
    case 0x00B1: printf("SetLocal? %04hx", vh);          lookup_var = 0x0101; break;
    case 0x00BC: printf("PushConst %6hd", vh);                                break;
    case 0x00BE: printf("PushCmdLocal? %04hx", vh);      lookup_var = 0x0101; break;
    case 0x00BF: printf("ResetLocal %04hx", vh);         lookup_var = 0x0101; break;
    case 0x00C8: printf("CmpLocal %04hx", vh);           lookup_var = 0x0101; break;
    case 0x00C9: printf("CmpConst %04hx", vh);                                break;
  }
  printf(FMT_COMMENT);

  if (nargs > 0) {
    curr_op = vl;
    curr_nargs = nargs;
    curr_remaining = nargs;
  }

  if (lookup_var != -1 && debug != NULL) {
    const struct debug_symbol *sym = lookup_sym(debug, var, lookup_var, i*4);
    printf(" (%s)", sym == NULL? "N/A" : sym->name);
  }

  printf(FMT_END);
}

/** Prints the given code section `code` to stdout. */
void print_code(struct code_block *code) {
  struct code_header hd_code;
  memcpy(&hd_code, code->header, sizeof(struct code_header));

//printf("------ \x1B[1mCode\x1B[m ------\n");
//printf("Unknowns: %04x %04x %08x %08x %08x %08x\n",
//       hd_code.unk1, hd_code.unk2, hd_code.extracted_size, hd_code.unk4,
//       hd_code.unk5, hd_code.unk6);

  for (int i = 0; i < code->ninstrs; i++) {
    u32 v  = code->instrs[i];
    u16 vh = v >> 16,
        vl = v & 0xFFFF;
    printf("  %04x: %s%04hx%s%04hx" FMT_END,
           i * 4, format_of(vh), vh, format_of(vl), vl);
    print_instruction_comment(code->instrs, i, NULL);
    printf("\n");
  }
}

/** Prints the given debug section `debug` to stdout. */
void print_debug(struct debug_block *debug) {
  struct debug_header hd_debug;
  memcpy(&hd_debug, debug->header, sizeof(struct debug_header));

  printf("\n------ \x1B[1mDebug\x1B[m ------\n");
  printf("  #unk1: %2d   #files: %2d   #linenos: %2d   #symbols: %2d   #types: %2d\n",
         hd_debug.count_unk1, hd_debug.count_files, hd_debug.count_linenos,
         hd_debug.count_symbols, hd_debug.count_types);
  printf("  Unknowns: %08x\n", hd_debug.unk1);

  // Files
  printf("\nFiles:\n");
  for (int i = 0; i < debug->nfiles; i++) {
    struct debug_file *file = &debug->files[i];
    printf("  [%08x] %s\n", file->start, file->name);
  }

  // LineNos
  printf("\nLineNos: (%d linenos)\n", debug->nlinenos);
//for (int i = 0; i < debug->nlinenos; i++) {
//  struct debug_raw_lineno *lineno = &debug->linenos[i];
//  printf("  [%08x] %d\n", lineno->start, lineno->lineno);
//}

  // Symbols
  printf("\nSymbols:\n");
  for (int i = 0; i < debug->nsymbols; i++) {
    struct debug_symbol *symbol = &debug->symbols[i];
    printf("  [%08x] %04x (%04x..%04x) %04x %s\n",
           symbol->id, symbol->unk1, symbol->start, symbol->end,
           symbol->type, symbol->name);
  }

  // Types
  printf("\nTypes:\n");
  for (int i = 0; i < debug->ntypes; i++) {
    struct debug_type *type = &debug->types[i];
    printf("%4d %s\n", type->id, type->name);
  }
}

/** Prints the given code section `code` to stdout, using debug info from
 *  `debug` as aid for pretty-printing. */
void print_debug_code(struct code_block *code, struct debug_block *debug) {
  // Create a sorted copy of the symbol table
  int sym_bytes = sizeof(struct debug_symbol) * debug->nsymbols;
  struct debug_symbol *symbols = malloc(sym_bytes);
  memcpy(symbols, debug->symbols, sym_bytes);
  qsort(symbols, debug->nsymbols, sizeof(struct debug_symbol), symbols_comparator);

  // Extract the globals, functions and locals from the symbol table.
  struct debug_symbol *sym_globals, *sym_functions, *sym_locals;
  int nglobals, nfunctions, nlocals;

  int k = 0;
  while (k < debug->nsymbols && symbols[k].type == 0x0001) k++;
  nglobals = k;
  while (k < debug->nsymbols && symbols[k].type == 0x0009) k++;
  nfunctions = k - nglobals;
  while (k < debug->nsymbols && symbols[k].type == 0x0101) k++;
  nlocals = k - nfunctions - nglobals;

  assert(nglobals + nfunctions + nlocals == debug->nsymbols);

  sym_globals = symbols;
  sym_functions = symbols + nglobals;
  sym_locals = symbols + nglobals + nfunctions;

  int file_i     = 0,
      line_i     = 0,
      global_i   = 0,
      function_i = 0,
      local_i    = 0;

  for (int i = 0; i < code->ninstrs; i++) {
    // Print file header if new file
    while (file_i < debug->nfiles && debug->files[file_i].start <= 4*i) {
      printf(FMT_COMMENT "\n");
      for (int i = 0; i < 74; i++) putchar(';');
      putchar('\n');
      printf(";;;; " FMT_END "%s" FMT_COMMENT " ",
             debug->files[file_i].name);
      int pad = 74 - (strlen(debug->files[file_i].name) + strlen(";;;;  "));
      if (pad < 0) pad = 0;
      for (int i = 0; i < pad; i++) putchar(';');
      putchar('\n');
      for (int i = 0; i < 74; i++) putchar(';');
      printf("\n" FMT_END);
      file_i++;
    }

    // Print lineno info if new lineno
    while (line_i < debug->nlinenos && debug->linenos[line_i].start <= 4*i) {
      printf(FMT_COMMENT "; LineNo: %d" FMT_END "\n",
             debug->linenos[line_i].lineno);
      line_i++;
    }

    // Print any new globals
    while (global_i < nglobals && sym_globals[global_i].start <= 4*i) {
      struct debug_symbol *sym = &sym_globals[global_i];
      printf(FMT_COMMENT "; Global: (%04hx) %s" FMT_END "\n",
             (u16) sym->id, sym->name);
      global_i++;
    }

    // Print out function header if appropriate
    while (function_i < nfunctions && sym_functions[function_i].start <= 4*i) {
      printf("\n");
      printf(FMT_COMMENT ";;;; %s" FMT_END "\n",
             sym_functions[function_i].name);
      function_i++;
    }

    // Print any new locals
    while (local_i < nlocals && sym_locals[local_i].start <= 4*i) {
      struct debug_symbol *sym = &sym_locals[local_i];
      printf(FMT_COMMENT ";   Local: (%04hx) %s" FMT_END "\n",
             (u16) sym->id, sym->name);
      local_i++;
    }

    // Print the actual instruction
    u32 v  = code->instrs[i];
    u16 vh = v >> 16,
        vl = v & 0xFFFF;
    printf("  %04x: %s%04hx%s%04hx" FMT_END,
           i * 4, format_of(vh), vh, format_of(vl), vl);
    print_instruction_comment(code->instrs, i, debug);
    printf("\n");
  }
}
