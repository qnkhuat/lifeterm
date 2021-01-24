.DEFAULT_GOAL := lifeterm
lifeterm: lifeterm.c
	@$(CC) lifeterm.c -g -o lifeterm.o -Wall -Wextra -pedantic -std=c99

hashlife: hashlife.c hashlife.h
	@$(CC) hashlife.c -g -o hashlife.o -Wall -Wextra -pedantic -std=c99 -Wno-incompatible-pointer-types-discards-qualifiers -v
 
test: test.c 
	@$(CC) test.c -g -o test.o -Wall -Wextra -pedantic -std=c11 -v
