#ifndef SCRIPT_PP_H
#define SCRIPT_PP_H

#include "formats/script.h"

/** Prints the given code section `code` to stdout. */
void print_code(struct code_block *code);

/** Prints the given debug section `debug` to stdout. */
void print_debug(struct debug_block *debug);

/** Prints the given code section `code` to stdout, using debug info from
 *  `debug` as aid for pretty-printing. */
void print_debug_code(struct code_block *code, struct debug_block *debug);

#endif
