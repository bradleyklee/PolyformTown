CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
SRC = src

CORE = $(SRC)/cycle.o $(SRC)/tile.o $(SRC)/hash.o $(SRC)/vec.o $(SRC)/attach.o $(SRC)/tetrille.o
VCOMP_CORE = $(CORE) $(SRC)/vcomp.o

all: poly_count poly_print vcomp_count vcomp_print imgtable tile_to_imgtable

poly_count: $(SRC)/count_poly.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_poly.o $(CORE)

poly_print: $(SRC)/print_poly.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_poly.o $(CORE)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

clean:
	rm -f $(SRC)/*.o poly_count poly_print vcomp_count count_vcomp vcomp_print imgtable tile_to_imgtable

vcomp_count: $(SRC)/count_vcomp.o $(VCOMP_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_vcomp.o $(VCOMP_CORE)

count_vcomp: vcomp_count
	cp vcomp_count count_vcomp

vcomp_print: $(SRC)/print_vcomp.o $(VCOMP_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_vcomp.o $(VCOMP_CORE)
	
imgtable: $(SRC)/imgtable.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/imgtable.o -lm

tile_to_imgtable: $(SRC)/tile_to_imgtable.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/tile_to_imgtable.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o

test: all
	./tests/smoke.sh
