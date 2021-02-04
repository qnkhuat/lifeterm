#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define MAX_NODES 7919
unsigned int node_hash1(int *a, int *b, int *c, int *d) { 
	// die after ~ 600 iters at size = 7919. But fluctuate between 500 -> 1500
	unsigned int h = (unsigned int) a;
	h *= 37;
	h += (unsigned int) b;
	h *= 37;
	h += (unsigned int) c;
	h *= 37;
	h += (unsigned int) d;
	return h % MAX_NODES;
}


unsigned int node_hash2(int *a, int *b, int *c, int *d) {
	// Consistent at around 800
	unsigned int h = ((unsigned int)d) + 3*( 
			((unsigned int)c) + 3*( 
				((unsigned int)b) + 3*((unsigned int)a) + 3 
				));
	return h % MAX_NODES;
}

unsigned int node_hash3(a,b,c,d) {
	// Fluctuate between 500-1500 but more consistent at 750
	unsigned int h = 65537*(unsigned int)(d)+257*(unsigned int)(c)+17*(unsigned int)(b)+5*(unsigned int)(a);
	return h % MAX_NODES;
}
int main(){
	int *table;
	table = (int *)calloc(MAX_NODES, sizeof(int));
	printf("Allocated %d int\n", MAX_NODES);
	//for (int i=0; i< MAX_NODES; i++){
	//	table[i] = 1;
	//}
	for (int i=0; i< 10000000; i++){
		int *a = (int *)malloc(sizeof(int));
		int *b = (int *)malloc(sizeof(int));
		int *c = (int *)malloc(sizeof(int));
		int *d = (int *)malloc(sizeof(int));
		int h = node_hash2(a, b, c, d);
		printf("Iter: %d - hash:%d\n", i, h);
		if (table[h] != NULL){
			printf("Hash collision at iter:%d with hash: %d\n", i, h);
			printf("Value is: %d\n", table[h]);
			exit(0);
		}else{
			table[h] = 2;
		}
	}
	printf("Done");
	return 0;
}

