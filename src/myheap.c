#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#define CALL_HP(call)                              \
  {                                                \
    if ((call) != HP_OK) {                         \
      fprintf(stderr, "%s failed\n", #call);       \
      BF_Close();                                  \
      return EXIT_FAILURE;                         \
    }                                              \
  }

int main(int argc, char *argv[]) {
  char *input = NULL;
  int opt;
  while ((opt = getopt(argc, argv, "i:")) != -1) {
    if (opt == 'i') input = optarg;
  }
  if (input == NULL) {
    fprintf(stderr, "usage: %s -i inputfile\n", argv[0]);
    return EXIT_FAILURE;
  }
  FILE *fp = fopen(input, "r");
  if (fp == NULL) {
    perror(input);
    return EXIT_FAILURE;
  }

  BF_Init(LRU);
  //HP_CreateFile fails if the file exists, so remove the one from the last run
  remove(DB_FILE);
  CALL_HP(HP_CreateFile(DB_FILE));
  int fd;
  CALL_HP(HP_OpenFile(DB_FILE, &fd));

  //one record per line
  char line[256];
  Record rec, first;
  int count = 0;
  while (fgets(line, sizeof(line), fp) != NULL) {
    memset(&rec, 0, sizeof(rec));
    if (sscanf(line, "%d %24s %19s %d %19s", &rec.id, rec.firstname,
               rec.lastname, &rec.postcode, rec.city) != 5)
      continue;
    CALL_HP(HP_InsertEntry(fd, rec));
    if (count++ == 0) first = rec;
  }
  fclose(fp);
  printf("inserted %d records\n", count);
  if (count == 0) {
    HP_CloseFile(fd);
    BF_Close();
    return EXIT_SUCCESS;
  }

  //reopen to show that everything was stored
  CALL_HP(HP_CloseFile(fd));
  CALL_HP(HP_OpenFile(DB_FILE, &fd));

  //the search values are taken from the first record of the input
  printf("\nall records:\n");
  CALL_HP(HP_PrintAllEntries(fd, "id", NULL));

  printf("\ncity = %s:\n", first.city);
  CALL_HP(HP_PrintAllEntries(fd, "city", first.city));

  printf("\npostcode = %d:\n", first.postcode);
  CALL_HP(HP_PrintAllEntries(fd, "postcode", &first.postcode));

  printf("\nHP_GetEntry(block 1, position 0): ");
  if (HP_GetEntry(fd, 1, 0, &rec) == HP_OK) printRecord(rec);
  printf("HP_GetEntry(block 0, position 0): %s\n",
         HP_GetEntry(fd, 0, 0, &rec) == HP_ERROR ? "HP_ERROR" : "HP_OK");

  printf("\nrecords with id = %d:\n", first.id);
  while (HP_GetNextRecordWithID(fd, &rec, &first.id) == HP_OK)
    printRecord(rec);
  printf("HP_EOF\n");

  CALL_HP(HP_CloseFile(fd));
  BF_Close();
  return EXIT_SUCCESS;
}
