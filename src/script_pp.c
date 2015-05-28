#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poketools.h"
#include "hexdump.h"
#include "formats/script.h"

#define FMT_FUNC    "\x1B[38;5;221m"
#define FMT_LOCAL   "\x1B[38;5;139m"
#define FMT_GLOBAL  "\x1B[38;5;150m"
#define FMT_LABEL   "\x1B[38;5;168m"
#define FMT_UNKNOWN "\x1B[31m"
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
    case 0x002E: printf("Begin");                                        break;
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
  struct debug_header *hd = debug->header;

  printf("\n------ \x1B[1mDebug\x1B[m ------\n");
  printf("  #unk1: %2d   #files: %2d   #linenos: %2d   #symbols: %2d   #types: %2d\n",
         hd->count_unk1, hd->count_files, hd->count_linenos, hd->count_symbols, hd->count_types);
  printf("  Unknowns: %08x\n", hd->unk1);

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


//-- New disassembler implementation --------------------------------
struct instr {
  i32 op;
  i32 high_half;
  int uses_high_half;
  int nargs;
  u32 *args;
};
int decode(struct instr *instr, u32 *code) {
  u32 v  = *code;
  u16 vh = v >> 16,
      vl = v & 0xFFFF;

  *instr = (struct instr) {
    .op             = vl,
    .high_half      = vh,
    .uses_high_half = 0,
    .nargs          = 0,
    .args           = code + 1,
  };

  switch (vl) {
    case 0x0009: instr->uses_high_half = 1;         break;
    case 0x000B: instr->nargs = 1;                  break;
    case 0x000C: instr->nargs = 1;                  break;
    case 0x000E: instr->nargs = 1;                  break;
    case 0x0017: instr->uses_high_half = 1;         break;
    case 0x0020: instr->uses_high_half = 1;         break;
    case 0x0022: instr->uses_high_half = 1;         break;
    case 0x0024: instr->uses_high_half = 1;         break;
    case 0x0025: instr->uses_high_half = 1;         break;
    case 0x0027: instr->nargs = 1;                  break; // PushConst
    case 0x002B: instr->uses_high_half = 1;         break;
    case 0x002E:                                    break; // Begin
    case 0x0030:                                    break; // Return
    case 0x0031: instr->nargs = 1;                  break; // Call
    case 0x0033: instr->nargs = 1;                  break; // Jump
    // Why in the world does this cause issues??
 // case 0x0034: instr->nargs = 1;                  break; // Jump??
    case 0x0035: instr->nargs = 1;                  break; // JumpNE
    case 0x0036: instr->nargs = 1;                  break; // JumpEq
    case 0x0037: instr->nargs = 1;                  break; // Jump??
    case 0x0038: instr->nargs = 1;                  break; // Jump??
    case 0x003D: instr->nargs = 1;                  break; // Jump??
    case 0x003E: instr->nargs = 1;                  break; // Jump??
    case 0x0040: instr->nargs = 1;                  break; // Jump??
    case 0x004E:                                    break; // Add?
    case 0x0051:                                    break; // Cmp?
    case 0x0059:                                    break; // PushFalse
    case 0x0069: instr->nargs = 1;                  break;
    case 0x0081: instr->nargs = 1;                  break; // Trampoline
    case 0x0082: instr->nargs = 2*code[1] + 2;      break; // JumpMap
    case 0x0087: instr->nargs = 2;                  break; // DoCommand?
    case 0x0089:                                    break; // LineNo
    case 0x008A: instr->nargs = 2;                  break;
    case 0x008E: instr->nargs = 3;                  break;
    case 0x0096: instr->nargs = 5;                  break;
    case 0x009B: instr->nargs = 2;                  break; // Copy
    case 0x009D: instr->nargs = 2;                  break;
    case 0x00A2: instr->uses_high_half = 1;         break; // GetGlobal2
    case 0x00A3: instr->uses_high_half = 1;         break; // GetGlobal
    case 0x00A4: instr->uses_high_half = 1;         break; // GetLocal
    case 0x00AB: instr->uses_high_half = 1;         break; // PushTrue
    case 0x00AC: instr->uses_high_half = 1;         break; // CmpConst2
    case 0x00AE: instr->uses_high_half = 1;         break;
    case 0x00AF: instr->uses_high_half = 1;         break; // SetGlobal
    case 0x00B1: instr->uses_high_half = 1;         break; // SetLocal?
    case 0x00B8: instr->uses_high_half = 1;         break;
    case 0x00B9: instr->uses_high_half = 1;         break;
    case 0x00BC: instr->uses_high_half = 1;         break; // PushConst
    case 0x00BD: instr->uses_high_half = 1;         break; // GetGlobal3
    case 0x00BE: instr->uses_high_half = 1;         break; // GetArg
    case 0x00BF: instr->uses_high_half = 1;         break; // ResetLocal
    case 0x00C5: instr->uses_high_half = 1;         break;
    case 0x00C6: instr->uses_high_half = 1;         break;
    case 0x00C8: instr->uses_high_half = 1;         break; // CmpLocal
    case 0x00C9: instr->uses_high_half = 1;         break; // CmpConst
    case 0x00CC: instr->uses_high_half = 1;         break;
    case 0x00D4: instr->uses_high_half = 1;         break;
    case 0x00D2:                                    break; // Script Begin
    default:
      instr->op = -1;
  }

  return instr->op != -1;
}

void disasm_assign_labels_(u32 *labels, struct code_block *code) {
  u32 *ins = code->instrs;
  int n = code->ninstrs;

  // First mark all targets for jump instructions
  struct instr instr;
  for (int i = 0; i < n; i += instr.nargs + 1) {
    decode(&instr, &ins[i]);

    switch (instr.op) {
      case 0x002E: { // Begin
        labels[i] = -1;
      } break;

      case 0x0031: case 0x0033: case 0x0034: case 0x0035: case 0x0036:
      case 0x0037: case 0x0038:
      case 0x003D: case 0x003E:
      case 0x0040: case 0x005A: { // Jumps
        u32 target = i + (int) ins[i + 1]/4;
        if (target < n && labels[target] == 0) labels[target] = 1;
      } break;

      case 0x0081: { // Trampoline
        u32 target = i + (int) ins[i + 1]/4;
        if (target < n) labels[target] = 1;
      } break;

      case 0x0082: { // JumpMaps
        labels[i] = 1;
        int choices = ins[i + 1];
        u32 target;

        #define SAFE_LOOKUP(target, idx) { \
          if (!(0 < idx && idx < n)) break; \
          target = (idx) + (int) ins[idx]/4 - 1; \
        }

        for (int j = 0; j < choices; j++) {
          SAFE_LOOKUP(target, i + 2 + 2*j);
          if (target < n && labels[target] == 0) labels[target] = 1;
        }
        SAFE_LOOKUP(target, i + 2 + 2*choices);
        if (target < n && labels[target] == 0) labels[target] = 1;

        #undef SAFE_LOOKUP
      } break;
    }
  }

  // Then, compute proper label indices within each function
  u32 counter = 0;
  for (int i = 0; i < n; i++) {
    switch (labels[i]) {
      case -1: counter = 0;           break; // Begin: function label
      case  1: labels[i] = ++counter; break; // Local label
    }
  }
}


char identifier_buf[BUFSIZ];
char *disasm_lookup_identifier_(struct debug_block *debug, u32 id, u32 type, int i) {
  const char *fmt = type == 0x101? FMT_LOCAL : FMT_GLOBAL;

  const struct debug_symbol *sym = NULL;
  if (debug != NULL) sym = lookup_sym(debug, id, type, i * 4);

  if (sym != NULL) {
    sprintf(identifier_buf, "%s%s%s", fmt, sym->name, FMT_END);
  } else {
    sprintf(identifier_buf, "%s$%04hx%s", fmt, (u16) (id & 0xFFFF), FMT_END);
  }
  return identifier_buf;
}

char label_buf[BUFSIZ];
char *disasm_lookup_label_(struct debug_block *debug, u32 *labels, int i) {
  switch (labels[i]) {
    case -1: {
      const struct debug_symbol *sym = NULL;
      if (debug != NULL) sym = lookup_sym(debug, i*4, 0x0009, 0);

      if (sym != NULL) {
        sprintf(label_buf, "%s%s%s", FMT_FUNC, sym->name, FMT_END);
      } else {
        sprintf(label_buf, "%sFunc_%04x%s", FMT_FUNC, i * 4, FMT_END);
      }
    } break;

    case 0: {
      label_buf[0] = 0;
    } break;

    default: {
      sprintf(label_buf, "%s.l%d%s", FMT_LABEL, labels[i], FMT_END);
    }
  }
  return label_buf;
}

void print_column(const char *str, int w) {
  int n = 0;
  for (const char *p = str; *p; p++) {
    if (*p == '\x1B') {
      while (*p != 'm' && *p) p++;
      n--;
    }
    n++;
  }

//printf("[%d %d]", w, n);

  #define max(a,b) ((a) > (b)? (a) : (b))
  if (w > 0) printf("%*s%s", max(0, w - n), "", str);
  else       printf("%s%*s", str, max(0, (-w) - n), "");
}

void disasm_line_(u32 *ins, int i, int n, const char *str, u32 *labels, struct debug_block *debug) {
  char *label = disasm_lookup_label_(debug, labels, i);
  if (n == 0) label[0] = 0; // This line doesn't really count

  if (labels[i] == -1) {
    printf("\n%s:\n", label);
    label[0] = 0;
  }

  if (label[0]) strcat(label, ":");

  //  ..________..######################################;__####: xxxxxxxx xxxxxxxx
  // "  .l1:      Call Func_00a4                        ;  0004: 00000031 000000a0
  printf("  ");
  print_column(label, -8);
  printf("  ");
  print_column(str, -37);
  printf(" %s;", FMT_COMMENT);

  if (n > 0) {
    printf("%3x%03x:", (4*i) >> 12, (4*i) & 0xFFF);
    for (int j = 0; j < n; j++) {
      int v = ins[i + j];
      u16 hi = v >> 16,
          lo = v & 0xFFFF;
      printf(" %s%04hx%s%04hx%s", format_of(hi), hi, format_of(lo), lo, FMT_END);
    }
  }
  printf("%s\n", FMT_END);
}

/** Disassembles the given code section `code` and prints to stdout. */
void disassemble(struct code_block *code, struct debug_block *debug) {
  // TODO: This is just temporary
  struct code_header *hd = code->header;
  printf("[Code block] section_size=%x  magic=%08x\n",
         hd->section_size, hd->magic);
  printf("  unk1=%04x  unk2=%04x  header_size=%04x\n",
         hd->unk1, hd->unk2, hd->header_size);
  printf("  extracted_size=%08x  extracted_code_size=%08x  unk4=%08x  unk6=%08x\n",
         hd->extracted_size, hd->extracted_code_size, hd->unk4, hd->unk6);
  printf("\n");
  for (int i = 0; i < code->nextra; i += 2) {
    u32 a = code->extra[i],
        b = code->extra[i + 1];
    printf("  %s%08x%s %s%08x%s\n", format_of(a), a, FMT_END, format_of(b), b, FMT_END);
  }
  printf("\n");

  // Disassembler proper
  u32 *ins = code->instrs;
  int n = code->ninstrs;

  u32 *labels = calloc(n, sizeof(u32));
  disasm_assign_labels_(labels, code);

  //-- Disassemble each instruction
  struct instr instr;
  for (int i = 0; i < n; i += instr.nargs + 1) {
    decode(&instr, &ins[i]);

    int nargs = instr.nargs;
    u16 vh = instr.high_half;

    char buf[BUFSIZ];
    buf[0] = 0;

    #define RLABEL(offset, j) disasm_lookup_label_(debug, labels, (offset) + (int) ins[j]/4)
    #define ID(id, type) disasm_lookup_identifier_(debug, (i16) (id), (type), i)
    #define GLOBAL(id) ID(id, 0x0001)
    #define FUNC(id)   ID(id, 0x0009)
    #define LOCAL(id)  ID(id, 0x0101)

    switch (instr.op) {
      case 0x0027: sprintf(buf, "CPushConst $%x", ins[i + 1]);      break;
      case 0x002E: sprintf(buf, "Begin");                           break;
      case 0x0030: sprintf(buf, "Return");                          break;
      case 0x0031: sprintf(buf, "Call %s",       RLABEL(i, i + 1)); break;
      case 0x0033: sprintf(buf, "Jump %s",       RLABEL(i, i + 1)); break;
      case 0x0034: sprintf(buf, "Jump?? %s",     RLABEL(i, i + 1)); break;
      case 0x0035: sprintf(buf, "JumpNE %s",     RLABEL(i, i + 1)); break;
      case 0x0036: sprintf(buf, "JumpEq %s",     RLABEL(i, i + 1)); break;
      case 0x0037: sprintf(buf, "Jump?? %s",     RLABEL(i, i + 1)); break;
      case 0x0038: sprintf(buf, "Jump?? %s",     RLABEL(i, i + 1)); break;
      case 0x003D: sprintf(buf, "Jump?? %s",     RLABEL(i, i + 1)); break;
      case 0x003E: sprintf(buf, "Jump?? %s",     RLABEL(i, i + 1)); break;
      case 0x0040: sprintf(buf, "Jump?? %s",     RLABEL(i, i + 1)); break;
      case 0x004E: sprintf(buf, "Add?");                            break;
      case 0x0051: sprintf(buf, "Cmp?");                            break;
      case 0x0059: sprintf(buf, "DPushFalse");                      break;
      case 0x0081: sprintf(buf, "Trampoline %s", RLABEL(i, i + 1)); break;

      case 0x0082: { // JumpMaps
        disasm_line_(ins, i, 2, "JumpMap {", labels, debug);
        int choices  = ins[i + 1],
            fallback = ins[i + 2],
            base;
        // Print each choice
        int j;
        for (j = 0; j < choices; j++) {
          base = i + 3 + 2*j;
          if (base + (int) ins[base]/4 - 1 >= n) break;
          sprintf(buf, "  %3d => %s", ins[base], RLABEL(base, base + 1));
          disasm_line_(ins, base, 2, buf, labels, debug);
        }
        if (j != choices) { // Check for broken instruction
          instr.nargs = 0;
          break;
        }
        // Print the fallback choice
        sprintf(buf, "  %3c => %s", '*', RLABEL(i + 1, i + 2));
        disasm_line_(ins, base, 1, buf, labels, debug);
        disasm_line_(ins, base + 1, 0, "}", labels, debug);
        // Suppress standard printing
        buf[0] = 0;
      } break;

      case 0x0087: sprintf(buf, "DoCommand? %d (%d args)", ins[i + 1], ins[i + 2] / 4); break;
      case 0x0089: sprintf(buf, "LineNo");                           break;

      case 0x008A: case 0x008E: case 0x0096: {
        sprintf(buf, "$%02X", instr.op);
        for (int i = 0; i < instr.nargs; i++) {
          sprintf(buf + strlen(buf), " %f,", *((float *) &instr.args[i]));
        }
        buf[strlen(buf) - 1] = 0;
      } break;

      case 0x009B: sprintf(buf, "$9B $%04hx, $%04hx", (u16) ins[i + 1], (u16) ins[i + 2]); break;
      case 0x00A2: sprintf(buf, "TGetGlobal2 %s",   GLOBAL(vh));     break;
      case 0x00A3: sprintf(buf, "DGetGlobal %s",    GLOBAL(vh));     break;
      case 0x00A4: sprintf(buf, "DGetLocal %s",      LOCAL(vh));     break;
      case 0x00AB: sprintf(buf, "DPushConst %d",           vh );     break;
      case 0x00AC: sprintf(buf, "CmpConst2 $%04hx",        vh );     break;
      case 0x00AF: sprintf(buf, "DSetGlobal %s",    GLOBAL(vh));     break;
      case 0x00B1: sprintf(buf, "DSetLocal %s",      LOCAL(vh));     break;
      case 0x00BC: sprintf(buf, "CPushConst %d",     (i16) vh );     break;
      case 0x00BD: sprintf(buf, "CGetGlobal %s",    GLOBAL(vh));     break;
      case 0x00BE: sprintf(buf, "CGetLocal %s",      LOCAL(vh));     break;
      case 0x00BF: sprintf(buf, "CAdjustStack %+hd",       vh );     break;
      case 0x00C8: sprintf(buf, "CmpLocal %s",       LOCAL(vh));     break;
      case 0x00C9: sprintf(buf, "CmpConst $%04hx",         vh );     break;
      case 0x00D2: sprintf(buf, "Script Begin");                     break;

      default:
        sprintf(buf, "%s$%04hx%s", FMT_UNKNOWN, (u16) (ins[i] & 0xFFFF), FMT_END);
    }

    // Print the line for this instruction
    if (buf[0] != 0) {
      disasm_line_(ins, i, instr.nargs + 1, buf, labels, debug);
    }

    #undef LABEL
    #undef ID
    #undef GLOBAL
    #undef FUNC
    #undef LOCAL
  }

  //-- Movement
  printf("\n");
  for (int i = 0; i < code->nmovement; i++) {
    u32 v = code->movement[i];
    printf("  %s%08x%s\n", format_of(v), v, FMT_END);
  }

  // Cleanup
  free(labels);
}
