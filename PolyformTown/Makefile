CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
SRC = src

CORE = $(SRC)/cycle.o $(SRC)/tile.o $(SRC)/hash.o $(SRC)/vec.o $(SRC)/attach.o

all: mono_count mono_dump

mono_count: $(SRC)/main_mono.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/main_mono.o $(CORE)

mono_dump: $(SRC)/dump_mono.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/dump_mono.o $(CORE)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

clean:
	rm -f $(SRC)/*.o mono_count mono_dump
