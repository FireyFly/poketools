#ifndef HEXDUMP_H
#define HEXDUMP_H

/** SGR string to format the byte `v`. */
const char *format_of(int v);

/** Hexdump `n` bytes at `p`. */
void hexdump(void *p, int n);

#endif
