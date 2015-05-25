#ifndef ZONEDATA_H
#define ZONEDATA_H

#include <stdio.h>

#include "../poketools.h"

//-- Types ------------------------------------------------
// Raw (packed) types
struct zone_header {
  u32 magic;
  u32 unk1;
  u32 unk2;
  u32 code2_offset;
  u32 file_size;
  u32 file_size2;
  u16 unk3[0x1C];
} __attribute__((packed));

// unk1 section
struct zone_unk1_header {
  u32 size;
  u8  num_unk1;
  u8  num_unk2;
  u8  num_unk3;
  u8  num_unk4;
  u8  num_unk5;
  u8  pad[3];
} __attribute__((packed));

struct zone_unk1_entry_1 {
  u16 fields[10];
} __attribute__((packed));

struct zone_unk1_entry_2 {
  u16 fields[24];
} __attribute__((packed));

struct zone_unk1_entry_3 {
  u16 fields[12];
} __attribute__((packed));

struct zone_unk1_entry_4 {
  u16 fields[12];
} __attribute__((packed));


// High-level structs
struct zone_unk1 {
  struct zone_unk1_header *header;
  int nentry1;
  struct zone_unk1_entry_1 *entry1;
  int nentry2;
  struct zone_unk1_entry_2 *entry2;
  int nentry3;
  struct zone_unk1_entry_3 *entry3;
  int nentry4;
  struct zone_unk1_entry_4 *entry4;
  int nentry5;
  struct zone_unk1_entry_4 *entry5;
};

struct zonedata {
  struct zone_header *header;
  struct zone_unk1   *unk1;
  struct code_block  *code1;
  struct code_block  *code2;
};


//-- Functions --------------------------------------------
struct zonedata *read_zonedata(FILE *f);


#endif
