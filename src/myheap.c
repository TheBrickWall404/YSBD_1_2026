#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

/* Τα αρχεία βάσης δημιουργούνται στον κατάλογο DBs/ του project */
#define DB_FILE "DBs/myheap.db"

/*
 * O driver της ασκησης.
 *
 * Το εκτελέσιμο πρέπει να διαβάζει τις εγγραφές που αποθηκεύει από ένα αρχείο
 * κειμένου, το οποίο υποδεικνύεται με τη σημαία -i:
 *
 *     ./bin/myheap -i inputs/records.txt
 *
 * Κάθε γραμμή του αρχείου περιέχει μία εγγραφή, με τα πεδία χωρισμένα με κενά
 * και στη σειρά που έχουν στη δομή Record:
 *
 *     id firstname lastname postcode city
 *     151 Dimitra Stasinou 12376 Ioannina
 *
 * Ο driver θα πρέπει να δημιουργεί το αρχείο σωρού, να εισάγει σε αυτό τις
 * εγγραφές του αρχείου εισόδου, και να επιδεικνύει τη λειτουργία των
 * συναρτήσεων του API.
 */

#define CALL_HP(call)                                              \
  {                                                                \
    HP_ErrorCode code = call;                                      \
    if (code != HP_OK) {                                           \
      fprintf(stderr, "Σφάλμα στη γραμμή %d: %s\n", __LINE__, #call); \
      BF_Close();                                                  \
      return EXIT_FAILURE;                                         \
    }                                                              \
  }

static void usage(const char *prog) {
  fprintf(stderr, "Χρήση: %s -i <αρχείο εισόδου>\n", prog);
}

int main(int argc, char *argv[]) {
  /* 1. Ορίσματα γραμμής εντολών: -i inputfile */
  const char *input = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc)
      input = argv[++i];
    else {
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }
  if (input == NULL) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  FILE *fp = fopen(input, "r");
  if (fp == NULL) {
    perror(input);
    return EXIT_FAILURE;
  }

  /* 2. Αρχικοποίηση BF και δημιουργία/άνοιγμα του αρχείου σωρού.
     Η HP_CreateFile αποτυγχάνει αν το αρχείο υπάρχει, οπότε σβήνουμε
     ό,τι έμεινε από προηγούμενη εκτέλεση. */
  BF_Init(LRU);
  remove(DB_FILE);
  CALL_HP(HP_CreateFile(DB_FILE));
  int fd;
  CALL_HP(HP_OpenFile(DB_FILE, &fd));

  /* 3. Ανάγνωση εγγραφών, μία ανά γραμμή, και εισαγωγή */
  char line[256];
  int lineno = 0, inserted = 0;
  Record first = {0};
  while (fgets(line, sizeof(line), fp) != NULL) {
    lineno++;
    Record rec = {0};
    int n = sscanf(line, "%d %24s %19s %d %19s", &rec.id, rec.firstname,
                   rec.lastname, &rec.postcode, rec.city);
    if (n != 5) {
      if (n > 0)
        fprintf(stderr, "Γραμμή %d: λάθος μορφή, παραλείπεται\n", lineno);
      continue; /* κενές ή λάθος γραμμές */
    }
    CALL_HP(HP_InsertEntry(fd, rec));
    if (inserted == 0) first = rec;
    inserted++;
  }
  fclose(fp);
  printf("Εισήχθησαν %d εγγραφές από το %s\n", inserted, input);
  if (inserted == 0) {
    HP_CloseFile(fd);
    BF_Close();
    return EXIT_SUCCESS;
  }

  /* Κλείνουμε και ξανανοίγουμε για να φανεί ότι τα δεδομένα και τα
     μεταδεδομένα έχουν όντως αποθηκευτεί */
  CALL_HP(HP_CloseFile(fd));
  CALL_HP(HP_OpenFile(DB_FILE, &fd));

  /* 4. Επίδειξη του API. Οι τιμές αναζήτησης παίρνονται από την πρώτη
     εγγραφή, ώστε η επίδειξη να δουλεύει με οποιοδήποτε αρχείο εισόδου. */
  printf("\n=== HP_PrintAllEntries: όλες οι εγγραφές ===\n");
  CALL_HP(HP_PrintAllEntries(fd, "id", NULL));

  printf("\n=== HP_PrintAllEntries: city = %s ===\n", first.city);
  CALL_HP(HP_PrintAllEntries(fd, "city", first.city));

  printf("\n=== HP_PrintAllEntries: postcode = %d ===\n", first.postcode);
  CALL_HP(HP_PrintAllEntries(fd, "postcode", &first.postcode));

  printf("\n=== HP_GetEntry ===\n");
  Record rec;
  int probes[][2] = {{1, 0}, {1, 1}, {0, 0}, {1, -1}, {1000, 0}};
  for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
    printf("block %d, θέση %d: ", probes[i][0], probes[i][1]);
    if (HP_GetEntry(fd, probes[i][0], probes[i][1], &rec) == HP_OK)
      printRecord(rec);
    else
      printf("δεν υπάρχει εγγραφή (HP_ERROR)\n");
  }

  printf("\n=== HP_GetNextRecordWithID: id = %d ===\n", first.id);
  HP_ErrorCode code;
  int found = 0;
  while ((code = HP_GetNextRecordWithID(fd, &rec, &first.id)) == HP_OK) {
    printRecord(rec);
    found++;
  }
  if (code == HP_EOF)
    printf("HP_EOF μετά από %d εγγραφές\n", found);
  else
    printf("HP_ERROR\n");

  /* 5. Κλείσιμο */
  CALL_HP(HP_CloseFile(fd));
  BF_Close();
  return EXIT_SUCCESS;
}
