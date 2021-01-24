.DEFAULT_GOAL := lifeterm
CC=gcc
lifeterm: lifeterm.c
	@$(CC) lifeterm.c hashlife.c -g -o lifeterm.o -Wall -Wextra -pedantic -std=c99

hashlife: hashlife.c 
	@$(CC) hashlife.c hashlife.c -g -o hashlife.o -Wall -Wextra -pedantic -std=c99 -Wno-incompatible-pointer-types-discards-qualifiers 
 
test: test.c 
	@$(CC) test.c hashlife.c lifeterm.c -g -o test.o -Wall -Wextra -pedantic -std=c11 
