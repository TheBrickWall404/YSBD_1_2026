#include <string.h>

#include "block_records.h"

int blockSetRecord(BF_Block *block, int slot, Record record) {
  if (slot < 0 || slot >= RECORDS_PER_BLOCK)
    return -1;

  char *data = BF_Block_GetData(block);

  /* Αντιγραφή της εγγραφής στη θέση slot του block */
  memcpy(data + slot * sizeof(Record), &record, sizeof(Record));

  /* Τα περιεχόμενα του block άλλαξαν, οπότε πρέπει κάποια στιγμή να γραφτούν
     στον δίσκο */
  BF_Block_SetDirty(block);
  return 0;
}

int blockGetRecord(BF_Block *block, int slot, Record *record) {
  if (slot < 0 || slot >= RECORDS_PER_BLOCK)
    return -1;

  char *data = BF_Block_GetData(block);

  /* Αντιγραφή της εγγραφής από τη θέση slot του block */
  memcpy(record, data + slot * sizeof(Record), sizeof(Record));
  return 0;
}
