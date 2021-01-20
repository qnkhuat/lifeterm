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

#define MAX_DEPTH (1 << 2*8) - 1
#define MAX_NODES (1 << 2*8) - 1
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
	Node *node = malloc(sizeof(Node));

	// init value of node
	int n = a->n + b->n + c->n + d->n; 
	node->n = n;
	node->k = a->k+1;
	node->a = a;
	node->b = b;
	node->c = c;
	node->d = d;

	int h = node_hash(a, b, c, d);
	hashtab[h] = node; // push in to hashtable
#ifdef DEBUG
	printf("Create new node: "); print_node(node);
#endif

	return node;
}

Node *find_node(Node *a, Node *b, Node *c, Node *d){
	int h = node_hash(a, b, c, d);
	Node *p = hashtab[h];
	//if (!p || (p->a != a && p->b != b && p->c != c && p->d != d)){
	//	p = newnode(a, b, c, d);
	//#ifdef DEBUG
	//	printf("Create new node: "); print_node(p);
	//#endif
	//	hashtab[h] = p; // put into hash table
	//}
	return p;
}

Node *get_zero(int k){
	int c = 0;
	Node *p = OFF;
	while (c!=k){
		Node *np = find_node(p, p, p, p);
		if(np==NULL)
			p = newnode(p, p, p, p);
		else
			p = np;
		c++;
	}
	return p;
}

typedef struct MapNode MapNode;
struct MapNode {
	int x;
	int y;
	Node *p;
};

Node *construct(){
	// Better way to init a 2-D arrays
	//int n = 3; // Number of points
	//int (*points)[n] = malloc(sizeof(int[n][2]));

	unsigned int n = 3; // number of points
	int points[3][2] = {{0, 1}, {0, 2}, {0,6}};

	// Translate points to origin
	int min_x = (1 << sizeof(int)*8) - 1;
	int min_y = (1 << sizeof(int)*8) - 1;
	for(int i=0; i < n; i++){
		if (points[i][0] < min_x)
			min_x = points[i][0];
		if (points[i][1] < min_y)
			min_y = points[i][1];
	}


	// Init a mapping of node with ON (level=0)
	MapNode *pattern = malloc(n*sizeof (MapNode)); 
	for (int i=0; i < n; i++){
		int x = points[i][0] - min_x;
		int y = points[i][1] - min_y;
		pattern[i] = (MapNode){.p = ON, .x = x, .y = y}; 
	}

	int k = 0;
	while (n > 1){
		Node *z = get_zero(k);
		MapNode *next_level = malloc(n * sizeof(MapNode));
		int m = 0;
		for (int i = 0; i < n; i++){
			MapNode p = pattern[i];
			if (p.p == NULL)
				continue;

			Node *a = z, *b = z, *c = z, *d = z ;
			int x = p.x;
			int y = p.y;
			x = x - (x & 1); y = y - (y & 1); // Move index to the start of its block
			for (int j = i; j < n; j++){ // Find all points that inside this 2x2 block
				int hit = 0;

				MapNode pp = pattern[j];
				if (pp.p == NULL)
					continue;

				if (pp.x == x && pp.y == y){
					a = pp.p;
					hit = 1;
				}
				else if (pp.x == x + 1 && pp.y == y){
					b = pp.p;
					hit = 1;
				}
				else if (pp.x == x && pp.y == y + 1){
					c = pp.p;
					hit = 1;
				}
				else if (pp.x == x + 1 && pp.y == y + 1){
					d = pp.p;
					hit = 1;
				}

				if (hit){
					pattern[j].p = NULL;
				}

			}
			Node *nodek = newnode(a, b, c, d);
			next_level[m] = (MapNode){.x = x >> 1, .y = y >> 1, . p = nodek}; // store a list of all pattern in this level
			m++;
		}
		n = m;
		k++;
		free(pattern);
		pattern = next_level;
	}
	return pattern->p;
}

int **expand();
Node advance();
Node successor();


/*** Utilities ***/
Node pad();
Node centre();
void print_node(const Node *node){ 
	printf("Node k=%d, %d x %d, population %d\n", node->k, 1 << node->k, 1 << node->k, node->n); 
}


void test_get_zero(){
	Node *p = get_zero(3);
	print_node(p);
	Node *p1 = get_zero(4);
	print_node(p1);
}

void test_construct(){
	Node *p = construct();
}

int main(){
	init();
	return 0;
}










