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

/*** Structs ***/
typedef struct Node Node;
struct Node {
	unsigned int n; // number of live cells. Max 4,294,967,295
	unsigned short k; // level. Max 65,535
	Node *a; // top left
	Node *b; // top right
	Node *c; // bottom left
	Node *d; // bottom right
};

typedef struct MapNode MapNode;
struct MapNode {
	int x;
	int y;
	Node *p;
};

/*** Node operations ***/
Node *get_zero(int k);
Node *newnode(Node *a, Node *b, Node *c, Node *d);
unsigned int node_hash(Node *a, Node *b, Node *c, Node *d);
Node *find_node(Node *a, Node *b, Node *c, Node *d);
Node *join(const Node *a, const Node *b, const Node *c, const Node *d);
Node *construct(int points[][2], int n);
void expand(Node *node, int x, int y);
void mark(Node *node, int x, int y, int mx, int my);

// For Update
Node *successor(Node *p, int j);
Node *advance(Node *p, int step);
Node *life(Node *n1, Node *n2, Node *n3, Node *n4, Node *c, Node *n6, Node *n7, Node *n8, Node *n9);
Node *life4x4(Node *p);

/*** View helpers ***/
void init();
void resize();
void render(Node *p);

/*** Utilities ***/
int is_padded(Node *p);
Node *inner(Node *p);
Node *crop(Node *p);
Node *centre(Node *p);
Node *pad(Node *p);
void print_node();


/*** Defines ***/
#define DEBUG 1
#define MAX_DEPTH (1 << 2*8) - 1
#define MAX_NODES (1 << 2*8) - 1
#define ON  &on
#define OFF &off
#define MAX_DEPTH (1 << 2*8) - 1
#define MAX_NODES (1 << 2*8) - 1

#endif
