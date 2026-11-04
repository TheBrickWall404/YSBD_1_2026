#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "block_records.h"
#include "record.h"

/* Τα αρχεία βάσης δημιουργούνται στον κατάλογο DBs/ του project */
#define DB_FILE "DBs/bf.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
  int fd1;
  BF_Block *block;
  BF_Block_Init(&block);



  /* Πρώτο Μέρος: χρήση της βιβλιοθήκης για block η οποία δημιουργεί ένα
  αρχείο με 10 block, μεταφέρει το καθένα από τα blocks στην ενδιάμεση
  μνήμη και γράφει δύο εγγραφές στην αρχή του κάθε block. Για να
  καταγραφούν οι αλλαγές  στον δίσκο, χρειάζεται να γίνει dirty το block
  και unpin για να μην γεμίσει η ενδιάμεση μνήμη. Μπορούμε να ανοίξουμε
  το αρχείο DBs/bf.db για να δούμε τα περιεχόμενά του.
  */
  CALL_OR_DIE(BF_Init(LRU));
  CALL_OR_DIE(BF_CreateFile(DB_FILE))
  CALL_OR_DIE(BF_OpenFile(DB_FILE, &fd1));

  for (int i = 0; i < 100; ++i) {
    CALL_OR_DIE(BF_AllocateBlock(fd1, block));  // Δημιουργία καινούριου block
    blockSetRecord(block, 0, randomRecord());   // Αντιγραφή εγγραφής στη θέση 0
    blockSetRecord(block, 1, randomRecord());   // Αντιγραφή εγγραφής στη θέση 1
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  CALL_OR_DIE(BF_CloseFile(fd1));               //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  CALL_OR_DIE(BF_Close());




  /* Δεύτερο Μέρος: χρήση της βιβλιοθήκης για block η οποία διαβάζει
  από κάθε block τα δύο πρώτα records.*/

  CALL_OR_DIE(BF_Init(LRU));
  CALL_OR_DIE(BF_OpenFile(DB_FILE, &fd1));
  int blocks_num;
  CALL_OR_DIE(BF_GetBlockCounter(fd1, &blocks_num));

  for (int i = 0; i < blocks_num; ++i) {
    printf("Contents of Block %d\n\t",i);
    CALL_OR_DIE(BF_GetBlock(fd1, i, block));
    Record rec;
    blockGetRecord(block, 0, &rec);             // Ανάγνωση της εγγραφής στη θέση 0
    printRecord(rec);
    printf("\t");
    blockGetRecord(block, 1, &rec);             // Ανάγνωση της εγγραφής στη θέση 1
    printRecord(rec);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(fd1));
  CALL_OR_DIE(BF_Close());

}

