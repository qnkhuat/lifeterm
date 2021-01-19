#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#define DEBUG 1
 /*
	*	Several things we need to consider
	* - How do we hash in C? I don't think we need to store the hash in side the Node, bc address itself is a good hash.
	* - 
	*/

/*** Node ***/
typedef struct Node Node;
struct Node {
	unsigned int n; // number of live cells. Max 4,294,967,295
	unsigned short k; // level. Max 65,535
	Node *a; // top left
	Node *b; // top right
	Node *c; // bottom left
	Node *d; // bottom right
	// We don't need the hash since the address is a hash itself
};

#define ON  &on
#define OFF &off
Node on  = {1, 0, NULL, NULL, NULL, NULL};
Node off = {0, 0, NULL, NULL, NULL, NULL};

/*** Prototypes ***/
void print_node();
Node *find_node();
Node *newnode();

/*** Node operations ***/
Node *join(const Node *a, const Node *b, const Node *c, const Node *d){
	//assert(a->k == b->k, a->k == c->k, a->k  == d->k); // make sure all nodes are the same level
	assert((a->k ^ b->k ^ c->k ^ d->k) == 0); // make sure all nodes are the same level
	Node *p;
	p = find_node(a, b, c, d);
	if (!p){
		p = newnode(a, b, c, d);
	}
	return p;
}

#define MAX_DEPTH 1 << 2*8 - 1
#define MAX_NODES 1 << 2*8 - 1
// INIT THE HASHTAB
Node **hashtab;
void init(){
	hashtab = (Node **)calloc(MAX_NODES, sizeof(Node *)) ;
}

unsigned int node_hash(Node *a, Node *b, Node *c, Node *d) {
	// TODO: is there a better way to hash?
	unsigned int h = a->k + 2
          + 5131830419411 * (int)a 
          + 3758991985019 * (int)b 
          + 8973110871315 * (int)c 
          + 4318490180473 * (int)d;
	return h % MAX_NODES;
}

// Create a node from 4 child node
Node *newnode(Node *a, Node *b, Node *c, Node *d){
		assert((a->k ^ b->k ^ c->k ^ d->k) == 0); // make sure all nodes are the same level
		assert(a->k < 30); // At development stage we want to make sure everything goes into our control
		Node *node = calloc(1, sizeof(Node));
		int n = a->n + b->n + c->n + d->n;
		node->n = n;
		node->k = a->k+1;
		node->a = a;
		node->b = b;
		node->c = c;
		node->d = d;
		return node;
}

Node *find_node(Node *a, Node *b, Node *c, Node *d){
	int h = node_hash(a, b, c, d);
	Node *p = hashtab[h];
	if (!p || (p->a != a && p->b != b && p->c != c && p->d != d)){
		p = newnode(a, b, c, d);
#ifdef DEBUG
		printf("Create new node: ");
		print_node(p);
#endif
		hashtab[h] = p; // put into hash table
	}
	return p;
}

Node *get_zero(int k){
	int c = 0;
	Node *p = OFF;
	while (c!=k){
		p = find_node(p, p, p, p);
		c++;
	}
	return p;
}

void construct(){
	int points[3][2] = {{0, 1}, {0, 2}, {0,3}};
	int n = 3;
	for (int i=0; i < n; i++){
		int x = points[i][0];
		int y = points[i][1];
	}
}

Node expand();
Node advance();
Node successor();


/*** Utilities ***/
Node pad();
Node centre();
void print_node(const Node *node){ 
	//printf("Node k=%d, %lli x %lli, population %d\n", node->k, (unsigned long int)1 << node->k, (unsigned long int)1 << node->k, node->n); 
	printf("Node k=%d, %d x %d, population %d\n", node->k, 1 << node->k, 1 << node->k, node->n); 
}


int main(){
	init();
	construct();
	return 0;
}
