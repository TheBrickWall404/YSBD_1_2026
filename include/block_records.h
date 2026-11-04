#ifndef BLOCK_RECORDS_H
#define BLOCK_RECORDS_H

#include "bf.h"
#include "record.h"

/*
 * Αντιγραφή εγγραφών σε/από ένα block
 *
 * Το επίπεδο BF δίνει ένα block ως ακατέργαστη περιοχή BF_BLOCK_SIZE bytes.
 * Η περιοχή αυτή δεν είναι πίνακας Record, οπότε οι εγγραφές αντιγράφονται με
 * memcpy σε διαδοχικές θέσεις (slots) σταθερού μεγέθους sizeof(Record). Η θέση
 * slot ξεκινά στο byte slot * sizeof(Record) του block.
 *
 * Οι δύο συναρτήσεις που ακολουθούν κάνουν αυτή την αντιγραφή, ελέγχοντας ότι
 * η θέση είναι μέσα στα όρια του block.
 */


/* Το μέγιστο πλήθος εγγραφών που χωρούν σε ένα block */
#define RECORDS_PER_BLOCK ((int)(BF_BLOCK_SIZE / sizeof(Record)))

/* Αντιγράφει την record στη θέση slot του block και σημειώνει το block ως
   dirty, ώστε το επίπεδο BF να το γράψει στον δίσκο.
   Επιστρέφει 0 σε επιτυχία, -1 αν το slot είναι εκτός των ορίων του block. */
int blockSetRecord(BF_Block *block, int slot, Record record);

/* Αντιγράφει στην *record την εγγραφή που βρίσκεται στη θέση slot του block.
   Το block δεν τροποποιείται, οπότε δεν γίνεται dirty.
   Επιστρέφει 0 σε επιτυχία, -1 αν το slot είναι εκτός των ορίων του block. */
int blockGetRecord(BF_Block *block, int slot, Record *record);

#endif  // BLOCK_RECORDS_H
