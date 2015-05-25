#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "zonedata.h"
#include "script.h"
#include "../poketools.h"
#include "../hexdump.h"

struct zonedata *read_zonedata(FILE *f) {
  struct zonedata *res = malloc(sizeof(struct zonedata));

  long section_start, section_end, section_size;


  //-- Header -------------------------
  res->header = malloc(sizeof(struct zone_header));
  struct zone_header *hd = res->header;
  fread(hd, sizeof(struct zone_header), 1, f);

  assert(hd->magic == 0x00044F5A);


  //-- Unk1 section -------------------
  res->unk1 = malloc(sizeof(struct zone_unk1));
  res->unk1->header = malloc(sizeof(struct zone_unk1_header));
  struct zone_unk1_header *unk1_hd = res->unk1->header;

  for (int i = 0; i < 3; i++) assert(unk1_hd->pad[i] == 0);

  section_start = ftell(f);

  fread(unk1_hd, sizeof(struct zone_unk1_header), 1, f);

  res->unk1->nentry1 = unk1_hd->num_unk1;
  res->unk1->entry1 = malloc(sizeof(struct zone_unk1_entry_1) * unk1_hd->num_unk1);
  fread(res->unk1->entry1, sizeof(struct zone_unk1_entry_1), unk1_hd->num_unk1, f);

  res->unk1->nentry2 = unk1_hd->num_unk2;
  res->unk1->entry2 = malloc(sizeof(struct zone_unk1_entry_2) * unk1_hd->num_unk2);
  fread(res->unk1->entry2, sizeof(struct zone_unk1_entry_2), unk1_hd->num_unk2, f);

  res->unk1->nentry3 = unk1_hd->num_unk3;
  res->unk1->entry3 = malloc(sizeof(struct zone_unk1_entry_3) * unk1_hd->num_unk3);
  fread(res->unk1->entry3, sizeof(struct zone_unk1_entry_3), unk1_hd->num_unk3, f);

  res->unk1->nentry4 = unk1_hd->num_unk4;
  res->unk1->entry4 = malloc(sizeof(struct zone_unk1_entry_4) * unk1_hd->num_unk4);
  fread(res->unk1->entry4, sizeof(struct zone_unk1_entry_4), unk1_hd->num_unk4, f);

  res->unk1->nentry5 = unk1_hd->num_unk5;
  res->unk1->entry5 = malloc(sizeof(struct zone_unk1_entry_4) * unk1_hd->num_unk5);
  fread(res->unk1->entry5, sizeof(struct zone_unk1_entry_4), unk1_hd->num_unk5, f);

  // Check if we read the entire section properly.
  section_end = ftell(f);
  section_size = unk1_hd->size + 4;

  if (section_end != section_start + section_size) {
    fprintf(stderr, "\x1B[33mwarning: unk1 section not read properly (size delta is %ld, @ $%lx)\x1B[m\n", 
            section_end - (section_start + section_size), ftell(f));
  }
  fseek(f, section_start + section_size, SEEK_SET);


  //-- Code sections ------------------
  //*
  section_start = ftell(f);

  res->code1 = read_code_block(f);

  // Check if we read the entire section properly.
  section_end = ftell(f);
  section_size = res->code1->header->section_size;

  if (section_end != section_start + section_size) {
    fprintf(stderr, "\x1B[33mwarning: code1 section not read properly (size delta is %ld)\x1B[m\n", 
            section_end - (section_start + section_size));
  }
  fseek(f, section_start + section_size, SEEK_SET);
  fseek(f, (4 - ftell(f) % 4) % 4, SEEK_CUR); // Round to full word

  res->code2 = read_code_block(f);
  //*/

  return res;
}
