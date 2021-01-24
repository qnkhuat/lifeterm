.DEFAULT_GOAL := lifeterm
CC=gcc
lifeterm: lifeterm.c
	@$(CC) lifeterm.c -g -o lifeterm.o -Wall -Wextra -pedantic -std=c99

hashlife: hashlife.c 
	@$(CC) hashlife.c -g -o hashlife.o -Wall -Wextra -pedantic -std=c99 -Wno-incompatible-pointer-types-discards-qualifiers 
 
test: test.c 
	@$(CC) test.c -g -o test.o -Wall -Wextra -pedantic -std=c11 
