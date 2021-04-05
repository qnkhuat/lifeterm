#ifndef HASHLIFE
#define HASHLIFE
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <termios.h>
#include "lifeterm.h"
#include "log.h"
#include <limits.h>

/*** Structs ***/
typedef struct Node Node;
typedef struct Node {
	unsigned int n; // number of live cells. Max 4,294,967,295
	unsigned short k; // level. Max 65,535
	Node *next; // Chaining to handle hash collision
	Node *a; // top left
	Node *b; // top right
	Node *c; // bottom left
	Node *d; // bottom right
};

typedef struct{
	int x;
	int y;
	Node *p;
} MapNode;

/*** Node operations ***/
Node *get_zero(int k);
Node *newnode(Node *a, Node *b, Node *c, Node *d);
uintptr_t node_hash(Node *a, Node *b, Node *c, Node *d);
Node *find_node(Node *a, Node *b, Node *c, Node *d);
Node *join(const Node *a, const Node *b, const Node *c, const Node *d);
Node *construct(int points[][2], int n);
void mark(Node *node, int x, int y);
void expand(Node *node, int x, int y);

// For Update
Node *successor(Node *p, int j);
Node *advance(Node *p, int n);
Node *life(Node *n1, Node *n2, Node *n3, Node *n4, Node *c, Node *n6, Node *n7, Node *n8, Node *n9);
Node *life4x4(Node *p);

/*** View helpers ***/
void init_hashtab();
void resize();

/*** Utilities ***/
int is_padded(Node *p);
Node *inner(Node *p);
Node *crop(Node *p);
Node *centre(Node *p);
Node *pad(Node *p);
void print_node(const Node *p);
int next_prime(int i);


/*** Test ***/
void test_new_collided();

/*** Defines ***/
#define MAX_DEPTH SHORT_MAX
//#define MAX_NODES INT_MAX
#define MAX_NODES INT_MAX
#define ON  &on
#define OFF &off
#define min(a, b) (((a) < (b)) ? (a) : (b))

#endif
