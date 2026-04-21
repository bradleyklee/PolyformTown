CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
SRC = src

CORE = $(SRC)/cycle.o $(SRC)/tile.o $(SRC)/hash.o $(SRC)/vec.o $(SRC)/attach.o
VCOMP_CORE = $(CORE) $(SRC)/vcomp.o

all: poly_count poly_print count_vcomp print_vcomp imgtable

poly_count: $(SRC)/count_poly.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_poly.o $(CORE)

poly_print: $(SRC)/print_poly.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_poly.o $(CORE)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

clean:
	rm -f $(SRC)/*.o poly_count poly_print count_vcomp print_vcomp imgtable

count_vcomp: $(SRC)/count_vcomp.o $(VCOMP_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_vcomp.o $(VCOMP_CORE)

print_vcomp: $(SRC)/print_vcomp.o $(VCOMP_CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_vcomp.o $(VCOMP_CORE)
	
imgtable: $(SRC)/imgtable.o
	$(CC) $(CFLAGS) -lm -o $@ $(SRC)/imgtable.o
	
