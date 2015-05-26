#include <stdio.h>

#include "formats/zonedata.h"
#include "formats/script.h"
#include "script_pp.h"
#include "hexdump.h"
#include "poketools.h"

void print_entry_line(int i, u16 *fields, int n) {
  printf("  %2d:", i);
  for (int j = 0; j < n; j++) printf(" %4x", fields[j]);
  printf("\n");
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s <filename>\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    fprintf(stderr, "Couldn't open '%s' for reading.", argv[1]);
    return 2;
  }

  struct zonedata *zone = read_zonedata(f);

  //-- Print header -------------------
  struct zone_header *hd = zone->header;

  printf("===> \x1B[1mHeader\x1B[m <===\n");
  printf("  unk1=%04x  unk2=%04x  code2_offset=%04x  filesize=(%x %x)\n",
         hd->unk1, hd->unk2, hd->code2_offset, hd->file_size, hd->file_size2);
  printf("  unk3=\n");
  for (int i = 0; i < 0x1C; i++) {
    if (i % 7 == 0) printf("    ");
    printf(" %4x", hd->unk3[i]);
    if (i % 7 == 6) printf("\n");
  }
  printf("\n");

  //-- Print unk1 section -------------
  printf("===> \x1B[1munk1\x1B[m <===\n");
  for (int i = 0; i < zone->unk1->nentry1; i++) {
    struct zone_unk1_entry_1 *ent = &zone->unk1->entry1[i];
    print_entry_line(i, ent->fields, 10);
  }
  printf("  ----\n");

  for (int i = 0; i < zone->unk1->nentry2; i++) {
    struct zone_unk1_entry_2 *ent = &zone->unk1->entry2[i];
    print_entry_line(i, ent->fields, 24);
  }
  printf("  ----\n");

  for (int i = 0; i < zone->unk1->nentry3; i++) {
    struct zone_unk1_entry_3 *ent = &zone->unk1->entry3[i];
    print_entry_line(i, ent->fields, 12);
  }
  printf("  ----\n");

  for (int i = 0; i < zone->unk1->nentry4; i++) {
    struct zone_unk1_entry_4 *ent = &zone->unk1->entry4[i];
    print_entry_line(i, ent->fields, 12);
  }
  printf("  ----\n");

  for (int i = 0; i < zone->unk1->nentry5; i++) {
    struct zone_unk1_entry_4 *ent = &zone->unk1->entry5[i];
    print_entry_line(i, ent->fields, 12);
  }
  printf("\n");

  //-- Print code sections ------------
  printf("===> \x1B[1mcode1\x1B[m <===\n");
//print_code(zone->code1);
  disassemble(zone->code1, NULL);
  printf("\n");

  printf("===> \x1B[1mcode2\x1B[m <===\n");
  disassemble(zone->code2, NULL);
//print_code(zone->code2);
}
