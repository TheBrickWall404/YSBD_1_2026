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
  return HP_ERROR;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  /* TODO: γράψτε στον δίσκο ό,τι εκκρεμεί και κλείστε το αρχείο */
  return HP_ERROR;
}

HP_ErrorCode HP_InsertEntry(int fileDesc, Record record) {
  /* TODO: βρείτε ή δεσμεύστε το κατάλληλο μπλοκ και αντιγράψτε εκεί την
     εγγραφή με την blockSetRecord. Μην ξεχάσετε να ενημερώσετε τα
     μεταδεδομένα του αρχείου. */
  BF_Block *block0;
  BF_Block_Init(&block0);
  CALL_BF(BF_GetBlock(fileDesc, 0, block0));
  
  HP_info *info = (HP_info *)BF_Block_GetData(block0);
  
  /* 2. Επεξεργασία και εισαγωγή εγγραφής ... */

  /* 3. Ενημέρωση Block 0 και άμεσο unpin */
  info->total_records++;
  BF_Block_SetDirty(block0);
  CALL_BF(BF_UnpinBlock(block0));
  BF_Block_Destroy(&block0);
  return HP_ERROR;
}

HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *fieldName, void *value) {
  /* TODO: διατρέξτε τα μπλοκ του αρχείου και εκτυπώστε τις εγγραφές που στο
     πεδίο fieldName έχουν τιμή value. Αν value == NULL, εκτυπώστε τις όλες. */
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
