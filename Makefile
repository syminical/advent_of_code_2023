BIN := $(shell basename $F .c)
build:
	@gcc -o "$(BIN)" "$(F)"
run:
	@gcc -o "$(BIN)" "$(F)"
	@./$(BIN)
runa:
	@gcc -o "$(BIN)" "$(F)"
	@./$(BIN) $(A)
