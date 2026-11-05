#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "block_records.h"
#include "hp_file.h"
#include "record.h"

/* Βοηθητικό macro: καλεί μια συνάρτηση του επιπέδου BF και, σε περίπτωση
   σφάλματος, εκτυπώνει την περιγραφή του και επιστρέφει HP_ERROR */
#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return HP_ERROR;        \
    }                         \
  }

HP_ErrorCode HP_CreateFile(const char *fileName) {
  /* TODO: δημιουργήστε το αρχείο, δεσμεύστε το πρώτο μπλοκ και αποθηκεύστε
     εκεί ένα στιγμιότυπο της HP_info */
  //create the heap file and open it
  CALL_BF(BF_CreateFile(fileName));
  int fileDesc;
  CALL_BF(BF_OpenFile(fileName, &fileDesc));
  BF_Block *block0;
  BF_Block_Init(&block0);
  CALL_BF(BF_AllocateBlock(fileDesc, block0));
  HP_info *info = (HP_info *)BF_Block_GetData(block0);

  //initialise the metadata of the heap file
  info->magic = 0xDEADBEEF;
  info->last_block_id = 0;
  info->max_records_per_block = BF_BLOCK_SIZE / sizeof(Record);
  info->total_records = 0;
  info->cursor_block = 1;
  info->cursor_slot = 0;

  //commit the changes to the block and unpin it
  BF_Block_SetDirty(block0);
  CALL_BF(BF_UnpinBlock(block0)); 
  BF_Block_Destroy(&block0);
  CALL_BF(BF_CloseFile(fileDesc));
  return HP_OK;
}

HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc) {
  /* TODO: ανοίξτε το αρχείο, διαβάστε τα μεταδεδομένα από το πρώτο μπλοκ και
     ελέγξτε ότι πρόκειται όντως για αρχείο σωρού */
  //open the heap file and read the metadata from the first block
  CALL_BF(BF_OpenFile(fileName, fileDesc));
  BF_Block *block0;
  BF_Block_Init(&block0);
  BF_Block **open_blocks0 = HP_GetOpenBlocksTable();

  //check if the first block is already open
  if (BF_GetBlock(*fileDesc, 0, block0) != BF_OK) {
    BF_Block_Destroy(&block0);
    BF_CloseFile(*fileDesc);
    return HP_ERROR;
  }
  //check if the magic number is correct
  HP_info *info = (HP_info *)BF_Block_GetData(block0);
  if (info->magic != 0xDEADBEEF) {
    BF_UnpinBlock(block0);
    BF_Block_Destroy(&block0);
    BF_CloseFile(*fileDesc);
    return HP_ERROR;
  }
  //store the block in the open blocks table
  open_blocks0[*fileDesc] = block0;
  return HP_OK;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  /* TODO: γράψτε στον δίσκο ό,τι εκκρεμεί και κλείστε το αρχείο */
  //check if the file descriptor is valid
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES) {
    return HP_ERROR;
  }
  //initialize the block and get the open blocks table
  BF_Block *block0;
  BF_Block **open_blocks0 = HP_GetOpenBlocksTable();
  block0 = open_blocks0[fileDesc];
  //check if the block is NULL
  if (block0 == NULL) {
    return HP_ERROR;
  }
  //unpin the block and destroy it
  CALL_BF(BF_UnpinBlock(block0));
  BF_Block_Destroy(&block0);
  open_blocks0[fileDesc] = NULL;
  CALL_BF(BF_CloseFile(fileDesc));
  return HP_OK;
}

HP_ErrorCode HP_InsertEntry(int fileDesc, Record record) {
  /* TODO: βρείτε ή δεσμεύστε το κατάλληλο μπλοκ και αντιγράψτε εκεί την
     εγγραφή με την blockSetRecord. Μην ξεχάσετε να ενημερώσετε τα
     μεταδεδομένα του αρχείου. */
  //initialize the block and get the open blocks table
  BF_Block **open_blocks0 = HP_GetOpenBlocksTable();
  //check if the file descriptor is valid and the block is not NULL
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES || open_blocks0[fileDesc] == NULL) {
      return HP_ERROR;
  }
  //get the first block and the metadata of the heap file
  BF_Block *block0 = open_blocks0[fileDesc];
  HP_info *info = (HP_info *)BF_Block_GetData(block0);
  /* 2. Επεξεργασία και εισαγωγή εγγραφής ... */
  BF_Block *target_block;
  BF_Block_Init(&target_block);
  //check if we need to allocate a new block or get the last block
  if (info->last_block_id == 0 || info->total_records % info->max_records_per_block == 0) {
    CALL_BF(BF_AllocateBlock(fileDesc, target_block));
    info->last_block_id++;
  } else {
    CALL_BF(BF_GetBlock(fileDesc, info->last_block_id, target_block));
  }
  //set the record in the target block and unpin it
  blockSetRecord(target_block, info->total_records % info->max_records_per_block, record);
  CALL_BF(BF_UnpinBlock(target_block));
  BF_Block_Destroy(&target_block);
  info->total_records++;
  BF_Block_SetDirty(block0);
  return HP_OK;
}

HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *fieldName, void *value) {
  /* TODO: διατρέξτε τα μπλοκ του αρχείου και εκτυπώστε τις εγγραφές που στο
     πεδίο fieldName έχουν τιμή value. Αν value == NULL, εκτυπώστε 
     τις όλες. */
  return HP_ERROR;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int blockid, int position,
                         Record *record) {
  /* TODO: φέρτε το μπλοκ blockid και διαβάστε την εγγραφή της θέσης position
     με την blockGetRecord */
  
  return HP_ERROR;
}

HP_ErrorCode HP_GetNextRecordWithID(int fileDesc, Record *record, void *value) {
  /* TODO: υλοποιήστε τον iterator. Η κατάσταση του (πού σταμάτησε η
     προηγούμενη κλήση) πρέπει να επιβιώνει ανάμεσα στις κλήσεις. */
  return HP_ERROR;
}
