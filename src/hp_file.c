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

//get the metadata block of an open file, NULL if fileDesc is not open
static BF_Block *getMetaBlock(int fileDesc) {
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES) return NULL;
  return HP_GetOpenBlocksTable()[fileDesc];
}

//number of records in a data block: all blocks are full except maybe the last
static int recordsInBlock(const HP_info *info, int blockid) {
  if (blockid < 1 || blockid > info->last_block_id) return 0;
  if (blockid < info->last_block_id) return info->max_records_per_block;
  return info->total_records - (info->last_block_id - 1) * info->max_records_per_block;
}

static int isValidField(const char *field) {
  return strcmp(field, "id") == 0 || strcmp(field, "firstname") == 0 ||
         strcmp(field, "lastname") == 0 || strcmp(field, "postcode") == 0 ||
         strcmp(field, "city") == 0;
}

//id and postcode are compared as ints, the rest as strings
static int fieldMatches(const Record *rec, const char *field, const void *value) {
  if (strcmp(field, "id") == 0) return rec->id == *(const int *)value;
  if (strcmp(field, "postcode") == 0) return rec->postcode == *(const int *)value;
  if (strcmp(field, "firstname") == 0) return strcmp(rec->firstname, value) == 0;
  if (strcmp(field, "lastname") == 0) return strcmp(rec->lastname, value) == 0;
  return strcmp(rec->city, value) == 0;
}

HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *fieldName, void *value) {
  BF_Block *block0 = getMetaBlock(fileDesc);
  if (block0 == NULL) return HP_ERROR;
  if (value != NULL && (fieldName == NULL || !isValidField(fieldName))) return HP_ERROR;
  HP_info *info = (HP_info *)BF_Block_GetData(block0);

  BF_Block *block;
  BF_Block_Init(&block);
  //scan every data block, keeping only one of them pinned at a time
  for (int b = 1; b <= info->last_block_id; b++) {
    if (BF_GetBlock(fileDesc, b, block) != BF_OK) {
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }
    int n = recordsInBlock(info, b);
    for (int i = 0; i < n; i++) {
      Record rec;
      blockGetRecord(block, i, &rec);
      if (value == NULL || fieldMatches(&rec, fieldName, value))
        printRecord(rec);
    }
    BF_UnpinBlock(block);
  }
  BF_Block_Destroy(&block);
  return HP_OK;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int blockid, int position,
                         Record *record) {
  if (record == NULL) return HP_ERROR;
  memset(record, 0, sizeof(Record));
  BF_Block *block0 = getMetaBlock(fileDesc);
  if (block0 == NULL) return HP_ERROR;
  HP_info *info = (HP_info *)BF_Block_GetData(block0);

  //empty slot or block 0 / nonexistent block
  if (position < 0 || position >= recordsInBlock(info, blockid)) return HP_ERROR;

  BF_Block *block;
  BF_Block_Init(&block);
  if (BF_GetBlock(fileDesc, blockid, block) != BF_OK) {
    BF_Block_Destroy(&block);
    return HP_ERROR;
  }
  blockGetRecord(block, position, record);
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return HP_OK;
}

//id of the last search per open file, a new id restarts the iterator
static int search_id[BF_MAX_OPEN_FILES];

HP_ErrorCode HP_GetNextRecordWithID(int fileDesc, Record *record, void *value) {
  BF_Block *block0 = getMetaBlock(fileDesc);
  if (block0 == NULL || record == NULL || value == NULL) return HP_ERROR;
  HP_info *info = (HP_info *)BF_Block_GetData(block0);
  int id = *(int *)value;

  //the cursor (cursor_block, cursor_slot) is the next position to check
  if (id != search_id[fileDesc]) {
    search_id[fileDesc] = id;
    info->cursor_block = 1;
    info->cursor_slot = 0;
  }
  BF_Block_SetDirty(block0);

  BF_Block *block;
  BF_Block_Init(&block);
  for (; info->cursor_block <= info->last_block_id; info->cursor_block++, info->cursor_slot = 0) {
    if (BF_GetBlock(fileDesc, info->cursor_block, block) != BF_OK) {
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }
    int n = recordsInBlock(info, info->cursor_block);
    while (info->cursor_slot < n) {
      blockGetRecord(block, info->cursor_slot++, record);
      if (record->id == id) {
        BF_UnpinBlock(block);
        BF_Block_Destroy(&block);
        return HP_OK;
      }
    }
    BF_UnpinBlock(block);
  }
  BF_Block_Destroy(&block);

  //end of file: reset the cursor for the next search
  info->cursor_block = 1;
  info->cursor_slot = 0;
  memset(record, 0, sizeof(Record));
  return HP_EOF;
}
