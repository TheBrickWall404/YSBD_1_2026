#ifndef RECORD_H
#define RECORD_H

typedef struct Record {
  int id;
  char firstname[25];
  char lastname[20];
  int postcode;
  char city[20];
} Record;

/* Δημιουργεί μια εγγραφή με τυχαία στοιχεία και αύξοντα id */
Record randomRecord();

/* Εκτυπώνει μια εγγραφή στη μορφή (id,firstname,lastname,postcode,city) */
void printRecord(Record record);

#endif  // RECORD_H
