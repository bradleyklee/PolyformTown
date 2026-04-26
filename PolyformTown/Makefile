CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
SRC = src

CORE = $(SRC)/cycle.o $(SRC)/tile.o $(SRC)/hash.o $(SRC)/vec.o $(SRC)/attach.o $(SRC)/tetrille.o $(SRC)/lattice.o
POLY_CORE = $(CORE) $(SRC)/vcomp.o $(SRC)/poly_pipeline.o
VCOMP_CORE = $(CORE) $(SRC)/vcomp.o $(SRC)/vcomp_pipeline.o

all: poly_count poly_print vcomp_count vcomp_print imgtable tile_to_imgtable rl0_generate rl0_depict rl0_stats

poly_count: $(SRC)/count_poly.o $(POLY_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_poly.o $(POLY_CORE)

poly_print: $(SRC)/print_poly.o $(POLY_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_poly.o $(POLY_CORE)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

clean:
	rm -f $(SRC)/*.o $(SRC)/rl0/*.o $(SRC)/usage/*.o poly_count poly_print vcomp_count count_vcomp vcomp_print imgtable tile_to_imgtable rl0_generate rl0_depict rl0_stats

vcomp_count: $(SRC)/count_vcomp.o $(VCOMP_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_vcomp.o $(VCOMP_CORE)

count_vcomp: vcomp_count
	cp vcomp_count count_vcomp

vcomp_print: $(SRC)/print_vcomp.o $(VCOMP_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_vcomp.o $(VCOMP_CORE)

imgtable: $(SRC)/imgtable.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/imgtable.o -lm

tile_to_imgtable: $(SRC)/tile_to_imgtable.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o $(SRC)/lattice.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/tile_to_imgtable.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o $(SRC)/lattice.o

rl0_generate: $(SRC)/rl0/generate_rl0.o $(VCOMP_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/rl0/generate_rl0.o $(VCOMP_CORE)

rl0_depict: $(SRC)/usage/depict_rl0.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o $(SRC)/lattice.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/usage/depict_rl0.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o $(SRC)/lattice.o

rl0_stats: $(SRC)/usage/rl0_stats.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o $(SRC)/lattice.o $(SRC)/vcomp.o $(SRC)/attach.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/usage/rl0_stats.o $(SRC)/tile.o $(SRC)/cycle.o $(SRC)/tetrille.o $(SRC)/lattice.o $(SRC)/vcomp.o $(SRC)/attach.o

test: all
	./tests/smoke.sh
