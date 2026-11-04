#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "record.h"

const char* firstnames[] = {
    "Alexandros",  // Αλέξανδρος
    "Sofia",       // Σοφία
    "Dimitris",    // Δημήτρης
    "Anna",        // Αννα
    "Konstantinos",// Κωνσταντίνος
    "Maria",       // Μαρία
    "Georgios",    // Γεώργιος
    "Eleni",       // Ελένη
    "Petros",      // Πέτρος
    "Evangelia"    // Ευαγγελία
};

const char* lastnames[] = {
    "Papadopoulos",   // Παπαδόπουλος
    "Georgiou",       // Γεωργίου
    "Dimitriou",      // Δημητρίου
    "Anagnostopoulos",// Αναγνωστόπουλος
    "Karagiannis",    // Καραγιάννης
    "Mavromatis",     // Μαυρομάτης
    "Nikolaou",       // Νικολάου
    "Christodoulou",  // Χριστοδούλου
    "Kostopoulos",    // Κωστόπουλος
    "Stamatopoulos"   // Σταματόπουλος
};

/* Οι πόλεις και οι ταχυδρομικοί τους κώδικες είναι παράλληλοι πίνακες, ώστε
   κάθε εγγραφή να παίρνει κώδικα που ταιριάζει με την πόλη της */
const char* cities[] = {
    "Athina",     // Αθήνα
    "Patra",      // Πάτρα
    "Irakleio",   // Ηράκλειο
    "Larisa",     // Λάρισα
    "Volos",      // Βόλος
    "Ioannina",   // Ιωάννινα
    "Chania",     // Χανιά
    "Kalamata",   // Καλαμάτα
    "Rodos"       // Ρόδος
};

const int postcodes[] = {
    10431,        // Αθήνα
    26221,        // Πάτρα
    71201,        // Ηράκλειο
    41221,        // Λάρισα
    38221,        // Βόλος
    45221,        // Ιωάννινα
    73131,        // Χανιά
    24100,        // Καλαμάτα
    85100         // Ρόδος
};

static int id = 0;

Record randomRecord(){
    Record record = {0};
    // create a record
    record.id = id++;
    int r = rand() % (sizeof(firstnames) / sizeof(firstnames[0]));
    memcpy(record.firstname, firstnames[r], strlen(firstnames[r]) + 1);
    //
    r = rand() %  (sizeof(lastnames) / sizeof(lastnames[0]));
    memcpy(record.lastname, lastnames[r], strlen(lastnames[r]) + 1);
    //
    r = rand() %  (sizeof(cities) / sizeof(cities[0]));
    memcpy(record.city, cities[r], strlen(cities[r]) + 1);
    record.postcode = postcodes[r];
    return record;
}

void printRecord(Record record){
    printf("(%d,%s,%s,%d,%s)\n", record.id, record.firstname,
           record.lastname, record.postcode, record.city);
}
