# ---------------------------------------------------------------------------
# Όλα τα paths είναι σχετικά με τη ρίζα του project.
#   make         -> μεταγλώττιση όλων των εκτελέσιμων
#   make myheap  -> μόνο το ./bin/myheap
#   make bf      -> μόνο το ./bin/bf_main
#   make clean   -> διαγραφή εκτελέσιμων, object files και αρχείων βάσης
# ---------------------------------------------------------------------------

# Κατάλογοι του project
SRC_DIR := src
INC_DIR := include
EX_DIR  := examples
LIB_DIR := lib
OBJ_DIR := build
BIN_DIR := bin
DB_DIR  := DBs

# Σημαίες μεταγλώττισης
CC      := gcc
CFLAGS  := -Wall -O2 -I$(INC_DIR)
LDFLAGS := -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR)
LDLIBS  := -lbf

# Εκτελέσιμα
MYHEAP_TARGET := $(BIN_DIR)/myheap
BF_TARGET     := $(BIN_DIR)/bf_main
TARGETS       := $(MYHEAP_TARGET) $(BF_TARGET)

# Object files
COMMON_OBJS := $(OBJ_DIR)/record.o $(OBJ_DIR)/block_records.o
MYHEAP_OBJS := $(COMMON_OBJS) $(OBJ_DIR)/hp_file.o $(OBJ_DIR)/myheap.o
BF_OBJS     := $(COMMON_OBJS) $(OBJ_DIR)/bf_main.o

# Κάθε object ξαναμεταγλωττίζεται όταν αλλάξει οποιοδήποτε header
HEADERS := $(wildcard $(INC_DIR)/*.h)

.PHONY: all myheap bf clean

all: myheap bf

myheap: $(MYHEAP_TARGET)

bf: $(BF_TARGET)

# Σύνδεση

$(MYHEAP_TARGET): $(MYHEAP_OBJS) | $(BIN_DIR) $(DB_DIR)
	@echo "Linking myheap ..."
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(BF_TARGET): $(BF_OBJS) | $(BIN_DIR) $(DB_DIR)
	@echo "Linking bf_main ..."
	$(CC) $^ $(LDFLAGS) $(LDLIBS) -o $@

# Μεταγλώττιση

# Πηγαία αρχεία της υλοποίησης
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

# Προγράμμα παραδείγματος
$(OBJ_DIR)/%.o: $(EX_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $< ..."
	$(CC) $(CFLAGS) -c $< -o $@

# Κατάλογοι εξόδου

# δημιουργούνται μία φορά, χωρίς να προκαλούν rebuild
$(BIN_DIR) $(OBJ_DIR) $(DB_DIR):
	mkdir -p $@

# Καθαρισμός

clean:
	@echo "Removing executables, object files and database files ..."
	rm -f $(TARGETS)
	rm -f $(OBJ_DIR)/*.o
	rm -f $(DB_DIR)/*.db
