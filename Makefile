.DEFAULT_GOAL := lifeterm
CC=gcc

lifeterm: lifeterm.c
	@$(CC) lifeterm.c hashlife.c log.c -g -o lifeterm.o -Wall -Wextra -pedantic -std=c99

hashlife: hashlife.c 
	@$(CC) hashlife.c hashlife.c -g -o hashlife.o -Wall -Wextra -pedantic -std=c99 -Wno-incompatible-pointer-types-discards-qualifiers 
 
test_hash: test_hash.c 
	@$(CC) test_hash.c -g -o test_hash.o -Wall -Wextra -pedantic -std=c11 

clean:
	@rm -rf *.dSYM *.swp
