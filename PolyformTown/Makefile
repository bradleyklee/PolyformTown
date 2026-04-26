CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
SRC = src

CORE = $(SRC)/core/cycle.o $(SRC)/core/tile.o $(SRC)/core/hash.o \
       $(SRC)/core/vec.o $(SRC)/core/attach.o \
       $(SRC)/core/tetrille.o $(SRC)/core/lattice.o \
       $(SRC)/core/boundary.o
POLY_SURFACE = $(SRC)/vcomp.o $(SRC)/poly_pipeline.o
VCOMP_SURFACE = $(SRC)/vcomp.o $(SRC)/vcomp_pipeline.o

all: poly_count poly_print vcomp_count vcomp_print imgtable \
	tile_to_imgtable rl0_generate rl0_depict rl0_data

poly_count: $(SRC)/count_poly.o $(CORE) $(POLY_SURFACE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_poly.o $(CORE) $(POLY_SURFACE)

poly_print: $(SRC)/print_poly.o $(CORE) $(POLY_SURFACE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_poly.o $(CORE) $(POLY_SURFACE)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

clean:
	rm -f $(SRC)/*.o $(SRC)/core/*.o $(SRC)/rl0/*.o $(SRC)/usage/*.o \
		poly_count poly_print vcomp_count count_vcomp vcomp_print \
		imgtable tile_to_imgtable rl0_generate rl0_depict

vcomp_count: $(SRC)/count_vcomp.o $(CORE) $(VCOMP_SURFACE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_vcomp.o $(CORE) $(VCOMP_SURFACE)

count_vcomp: vcomp_count
	cp vcomp_count count_vcomp

vcomp_print: $(SRC)/print_vcomp.o $(CORE) $(VCOMP_SURFACE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_vcomp.o $(CORE) $(VCOMP_SURFACE)

imgtable: $(SRC)/imgtable.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/imgtable.o -lm

tile_to_imgtable: $(SRC)/tile_to_imgtable.o $(SRC)/core/tile.o \
	$(SRC)/core/cycle.o $(SRC)/core/tetrille.o $(SRC)/core/lattice.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/tile_to_imgtable.o \
		$(SRC)/core/tile.o $(SRC)/core/cycle.o \
		$(SRC)/core/tetrille.o $(SRC)/core/lattice.o

rl0_generate: $(SRC)/rl0/generate_rl0.o $(CORE) $(SRC)/vcomp.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/rl0/generate_rl0.o $(CORE) $(SRC)/vcomp.o

rl0_depict: $(SRC)/usage/depict_rl0.o $(CORE) $(SRC)/vcomp.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/usage/depict_rl0.o $(CORE) $(SRC)/vcomp.o

rl0_stats: $(SRC)/usage/rl0_stats.o $(CORE) $(SRC)/vcomp.o
	$(CC) $(CFLAGS) -o $@ $(SRC)/usage/rl0_stats.o $(CORE) $(SRC)/vcomp.o

test: all
	./tests/smoke.sh

rl0_data: rl0_generate
	./rl0_generate tiles/hat.tile levelData/rl0/completions.dat

rl0_svg: rl0_data rl0_depict imgtable
	./rl0_depict --data levelData/rl0/completions.dat | \
		./imgtable > levelData/rl0/rl0_all.svg
