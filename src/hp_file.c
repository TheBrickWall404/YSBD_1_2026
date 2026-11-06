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

/* Επιστρέφει το HP_info ενός ανοιχτού αρχείου, ή NULL αν το fileDesc δεν
   αντιστοιχεί σε ανοιχτό αρχείο σωρού */
static HP_info *getInfo(int fileDesc) {
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES) return NULL;
  BF_Block *block0 = HP_GetOpenBlocksTable()[fileDesc];
  if (block0 == NULL) return NULL;
  return (HP_info *)BF_Block_GetData(block0);
}

/* Πόσες εγγραφές έχει το data block blockid. Τα data blocks δεν κρατάνε δικό
   τους μετρητή: όλα είναι γεμάτα εκτός ίσως από το τελευταίο, οπότε το πλήθος
   προκύπτει από το total_records του HP_info */
static int recordsInBlock(const HP_info *info, int blockid) {
  if (blockid < 1 || blockid > info->last_block_id) return 0;
  if (blockid < info->last_block_id) return info->max_records_per_block;
  return info->total_records -
         (info->last_block_id - 1) * info->max_records_per_block;
}

/* Ελέγχει αν η εγγραφή rec έχει στο πεδίο fieldName την τιμή value.
   Για τα id και postcode το value δείχνει σε int, για τα υπόλοιπα σε string.
   Επιστρέφει 1 (ταιριάζει), 0 (δεν ταιριάζει) ή -1 (άγνωστο πεδίο). */
static int recordMatches(const Record *rec, const char *fieldName,
                         const void *value) {
  if (strcmp(fieldName, "id") == 0)
    return rec->id == *(const int *)value;
  if (strcmp(fieldName, "postcode") == 0)
    return rec->postcode == *(const int *)value;
  if (strcmp(fieldName, "firstname") == 0)
    return strcmp(rec->firstname, (const char *)value) == 0;
  if (strcmp(fieldName, "lastname") == 0)
    return strcmp(rec->lastname, (const char *)value) == 0;
  if (strcmp(fieldName, "city") == 0)
    return strcmp(rec->city, (const char *)value) == 0;
  return -1;
}

HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *fieldName, void *value) {
  HP_info *info = getInfo(fileDesc);
  if (info == NULL) return HP_ERROR;

  /* Με value != NULL χρειαζόμαστε έγκυρο όνομα πεδίου. Το ελέγχουμε από πριν
     με μια ψεύτικη εγγραφή, ώστε να μη σαρώσουμε άσκοπα όλο το αρχείο. */
  if (value != NULL) {
    Record dummy = {0};
    if (fieldName == NULL || recordMatches(&dummy, fieldName, value) == -1) {
      fprintf(stderr, "HP_PrintAllEntries: άγνωστο πεδίο '%s'\n",
              fieldName ? fieldName : "(null)");
      return HP_ERROR;
    }
  }

  BF_Block *block;
  BF_Block_Init(&block);

  /* Σειριακή σάρωση: όλα τα data blocks (1..last), όλα τα γεμάτα slots */
  for (int b = 1; b <= info->last_block_id; b++) {
    BF_ErrorCode code = BF_GetBlock(fileDesc, b, block); /* pin */
    if (code != BF_OK) {
      BF_PrintError(code);
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }

    int n = recordsInBlock(info, b);
    for (int slot = 0; slot < n; slot++) {
      Record rec;
      blockGetRecord(block, slot, &rec);
      if (value == NULL || recordMatches(&rec, fieldName, value) == 1)
        printRecord(rec);
    }

    code = BF_UnpinBlock(block); /* unpin πριν πάμε στο επόμενο block */
    if (code != BF_OK) {
      BF_PrintError(code);
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }
  }

  BF_Block_Destroy(&block);
  return HP_OK;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int blockid, int position,
                         Record *record) {
  if (record == NULL) return HP_ERROR;
  /* Σε κάθε αποτυχία η record επιστρέφεται "άδεια" (μηδενισμένη) */
  memset(record, 0, sizeof(Record));

  HP_info *info = getInfo(fileDesc);
  if (info == NULL) return HP_ERROR;

  /* Αν η θέση δεν περιέχει εγγραφή, δεν χρειάζεται καν να φέρουμε το block */
  if (position < 0 || position >= recordsInBlock(info, blockid))
    return HP_ERROR;

  BF_Block *block;
  BF_Block_Init(&block);
  BF_ErrorCode code = BF_GetBlock(fileDesc, blockid, block); /* pin */
  if (code != BF_OK) {
    BF_PrintError(code);
    BF_Block_Destroy(&block);
    return HP_ERROR;
  }

  blockGetRecord(block, position, record); /* μόνο ανάγνωση: όχι dirty */

  code = BF_UnpinBlock(block); /* κάθε GetBlock ζευγαρώνεται με ένα Unpin */
  BF_Block_Destroy(&block);
  if (code != BF_OK) {
    BF_PrintError(code);
    return HP_ERROR;
  }
  return HP_OK;
}

/*
 * Iterator: η κατάσταση ζει στο HP_info (block 0), στα πεδία cursor_block και
 * cursor_slot, που δείχνουν την ΕΠΟΜΕΝΗ θέση που θα εξεταστεί. Η αρχική
 * κατάσταση είναι (1, 0), όπως την ορίζει η HP_CreateFile.
 *
 *  - Κάθε κλήση συνεχίζει από τον cursor και, αν βρει εγγραφή με το ζητούμενο
 *    id, μετακινεί τον cursor ακριβώς μετά από αυτήν.
 *  - Όταν φτάσει στο τέλος επιστρέφει HP_EOF και επαναφέρει τον cursor στο
 *    (1, 0), ώστε η επόμενη αναζήτηση να ξεκινήσει από την αρχή.
 *  - Αν ο caller αλλάξει τιμή id στη μέση μιας αναζήτησης, η νέα αναζήτηση
 *    ξεκινά από την αρχή. Η τελευταία τιμή κρατιέται σε static πίνακα ανά
 *    ανοιχτό αρχείο (αρχικοποιείται στην πρώτη κλήση με cursor (1, 0)).
 */
static int iterator_id[BF_MAX_OPEN_FILES];

HP_ErrorCode HP_GetNextRecordWithID(int fileDesc, Record *record, void *value) {
  if (record == NULL || value == NULL) return HP_ERROR;
  HP_info *info = getInfo(fileDesc);
  if (info == NULL) return HP_ERROR;
  BF_Block *block0 = HP_GetOpenBlocksTable()[fileDesc];
  int id = *(int *)value;

  int fresh = (info->cursor_block == 1 && info->cursor_slot == 0);
  if (!fresh && iterator_id[fileDesc] != id) {
    /* άλλαξε η τιμή αναζήτησης: ξεκινάμε από την αρχή */
    info->cursor_block = 1;
    info->cursor_slot = 0;
  }
  iterator_id[fileDesc] = id;

  BF_Block *block;
  BF_Block_Init(&block);

  for (int b = info->cursor_block; b <= info->last_block_id; b++) {
    BF_ErrorCode code = BF_GetBlock(fileDesc, b, block); /* pin */
    if (code != BF_OK) {
      BF_PrintError(code);
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }

    int n = recordsInBlock(info, b);
    /* στο block του cursor ξεκινάμε από το cursor_slot, στα επόμενα από 0 */
    int start = (b == info->cursor_block) ? info->cursor_slot : 0;
    for (int slot = start; slot < n; slot++) {
      Record rec;
      blockGetRecord(block, slot, &rec);
      if (rec.id == id) {
        *record = rec;
        /* ο cursor δείχνει στην επόμενη θέση μετά την εγγραφή που βρέθηκε */
        if (slot + 1 < n) {
          info->cursor_block = b;
          info->cursor_slot = slot + 1;
        } else {
          info->cursor_block = b + 1;
          info->cursor_slot = 0;
        }
        BF_Block_SetDirty(block0); /* άλλαξαν τα μεταδεδομένα */
        BF_UnpinBlock(block);
        BF_Block_Destroy(&block);
        return HP_OK;
      }
    }

    BF_UnpinBlock(block);
  }

  /* Τέλος αρχείου: reset του iterator για την επόμενη αναζήτηση */
  BF_Block_Destroy(&block);
  info->cursor_block = 1;
  info->cursor_slot = 0;
  BF_Block_SetDirty(block0);
  memset(record, 0, sizeof(Record));
  return HP_EOF;
}
