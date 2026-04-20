CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11
SRC = src

CORE = $(SRC)/cycle.o $(SRC)/tile.o $(SRC)/hash.o $(SRC)/vec.o $(SRC)/attach.o

all: poly_count poly_print

poly_count: $(SRC)/count_poly.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/count_poly.o $(CORE)

poly_print: $(SRC)/print_poly.o $(CORE)
	$(CC) $(CFLAGS) -o $@ $(SRC)/print_poly.o $(CORE)

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

clean:
	rm -f $(SRC)/*.o poly_count poly_print
