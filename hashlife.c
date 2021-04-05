#include "hashlife.h"
Node on  = {1, 0, NULL, NULL, NULL, NULL};
Node off = {0, 0, NULL, NULL, NULL, NULL};
Node **hashtab;


/*** Node operations ***/
Node *join(const Node *a, const Node *b, const Node *c, const Node *d){
	assert((a->k ^ b->k ^ c->k ^ d->k) == 0); // make sure all nodes are the same level
	Node *p;
	p = find_node(a, b, c, d);
	if (!p)
		p = newnode(a, b, c, d);
	return p;
}


void init_hashtab(){ 
	hashtab = (Node **)calloc(MAX_NODES, sizeof(Node *)); 
}

unsigned int node_hash(Node *a, Node *b, Node *c, Node *d) {
	// Refer to test_hash.c for different hash methods
	unsigned int h = 65537*(unsigned int)(d)+257*(unsigned int)(c)+17*(unsigned int)(b)+5*(unsigned int)(a);
	return h % MAX_NODES;
}

// Create a node from 4 child node
Node *newnode(Node *a, Node *b, Node *c, Node *d){
	assert((a->k ^ b->k ^ c->k ^ d->k) == 0); // make sure all nodes are the same level
	assert(a->k < 30); // At development stage we want to make sure everything is in our control
	Node *node = malloc(sizeof(Node));

	// init value of node
	int n = a->n + b->n + c->n + d->n; 
	node->k = a->k+1;
	node->n = n;
	node->a = a;
	node->b = b;
	node->c = c;
	node->d = d;
	node->next = NULL;

	int h = node_hash(a, b, c, d);
	if (hashtab[h] != NULL){
		log_info("Create: Hash collided");
		node->next = hashtab[h];
	}

	hashtab[h] = node; // push in to hashtable
	//log_info("Create new node: Node k=%d, %d x %d, population %d at hash:%d", node->k, 1 << node->k, 1 << node->k, node->n, h); 
	return node;
}

Node *find_node(Node *a, Node *b, Node *c, Node *d){
	int h = node_hash(a, b, c, d);
	Node *p;
	for (p=hashtab[h]; p; p = p->next)  /* make sure to compare a first */
		if (p->a == a && p->b == b && p->c == c && p->d == d) // In case hash collision compare its value
			return p;
	return p; // NULL
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

Node *construct(int points[][2], int n){
	 // Init a mapping of node with ON (level=0)
	 MapNode *pattern = malloc(n*sizeof (MapNode)); 
	 for (int i=0; i < n; i++){
		int x = points[i][0];
		int y = points[i][1];
		log_info("construct x:%d, y:%d", x, y);
		pattern[i] = (MapNode){.p = ON, .x = x, .y = y}; 
	}

	int k = 0;
	while (n > 1){ // until there are only one node left
		Node *z = get_zero(k);
		MapNode *next_level = malloc(n * sizeof(MapNode));
		int m = 0; // store number of node in this level
		for (int i = 0; i < n; i++){ // Group all childs node in current depth to from parents nodes
			MapNode p = pattern[i];
			if (p.p == NULL)
				continue;

			Node *a = z, *b = z, *c = z, *d = z;
			int x = p.x - (int)(p.x & 1); int y = p.y - (int)(p.y & 1); // Move index to the start of its block
			for (int j = i; j < n; j++){ // find neighbours of current node
				MapNode *pp = &pattern[j];
				if (pp->p == NULL)
					continue;

				if (pp->x == x && pp->y == y){
					a = pp->p;
					pp->p = NULL; // set to None so this node will be excluded in the next loop
				} else if (pp->x == x + 1 && pp->y == y){
					b = pp->p;
					pp->p = NULL;
				} else if (pp->x == x && pp->y == y + 1){
					c = pp->p;
					pp->p = NULL;
				} else if (pp->x == x + 1 && pp->y == y + 1){
					d = pp->p;
					pp->p = NULL;
				}
			}

			Node *nodek = newnode(a, b, c, d);
			next_level[m] = (MapNode){.x = x >> 1, .y = y >> 1, .p = nodek}; // store a list of all pattern in this level
			m++;
		}
		n = m; k++;
		free(pattern);
		pattern = next_level;
	}

	Node *result = pattern->p;
	free(pattern); // Can't let the garbage floatting around
	log_info("Constructed node: Node k=%d, %d x %d, population %d", result->k, 1 << result->k, 1 << result->k, result->n); 
	return result;
}


void expand(Node *node, int x, int y){
  // if node->k == 0 : (x, y) is the position on the grid
  // else (x, y) is the position of the node's upper left tile
	int offset = 1 << (node->k - 1);
	if (node->n == 0)
		return;

	// clip only points in view
	int size = 1 << node->k;
	if (x + size <= 0 || x >= E.gridcols|| y + size <= 0 || y >= E.gridrows)
		return;

	// base case
	if (node->k == 0){
		E.grid[y][x] = 1;
		log_info("expand x:%d, y:%d, Ox:%d, Oy:%d", x, y, E.ox, E.oy);
		return;
	}

	expand(node->a, x, y);
	expand(node->b, x + offset, y);
	expand(node->c, x, y + offset);
	expand(node->d, x + offset, y + offset);
}

void mark(Node *p, int x, int y){
  // x, y is the position in the universe with the universe's origin at upper left corner

	Node *n = p;
	MapNode *nodetab = (MapNode *)calloc((p->k+1), sizeof (MapNode)); 

	int size;
	int x_1, y_1; // store x and y at level 1
	nodetab[n->k] = (MapNode){.p = p, .x = 0, .y = 0};  // store the root
	for (int k = p->k; k >= 2; k--){
		size = 1 << (n->k - 1);
		if ( x < size ){
			if ( y < size ){
				n = n->a;
				nodetab[n->k] = (MapNode){.p = n, .x = 0, .y = 0}; 
			} else {
				n = n->c;
				nodetab[n->k] = (MapNode){.p = n, .x = 0, .y = 1}; 
				y = y - size;
			}
		} else {
			if ( y < size ){
				n = n->b;
				nodetab[n->k] = (MapNode){.p = n, .x = 1, .y = 0}; 
				x = x - size;
			} else {
				n = n->d;
				nodetab[n->k] = (MapNode){.p = n, .x = 1, .y = 1}; 
				x = x - size;
				y = y - size;
			}
		}
		if(n->k == 1){
			x_1 = x;
			y_1 = y;
		}
	}

	n = nodetab[1].p;
	size = 1 << (n->k - 1);
	Node *node2x2 = join(
			x_1 == 0 && y_1 == 0 ? (n->a->n ==0 ? ON : OFF) : n->a,
			x_1 == 1 && y_1 == 0 ? (n->b->n ==0 ? ON : OFF) : n->b,
			x_1 == 0 && y_1 == 1 ? (n->c->n ==0 ? ON : OFF) : n->c,
			x_1 == 1 && y_1 == 1 ? (n->d->n ==0 ? ON : OFF) : n->d
			);

	nodetab[1].p = node2x2;

	// Recreate the tree from bottom up. reuse the node that is not-modified
	for (int k = 1; k < p->k; k++){
		MapNode *cur = &nodetab[k];
		MapNode *next= &nodetab[k+1];
		next->p = join(
				cur->x == 0 && cur->y == 0 ? cur->p : next->p->a,
				cur->x == 1 && cur->y == 0 ? cur->p : next->p->b,
				cur->x == 0 && cur->y == 1 ? cur->p : next->p->c,
				cur->x == 1 && cur->y == 1 ? cur->p : next->p->d
				);
	}

	E.root = nodetab[p->k].p;
	free(nodetab);
}

Node *successor(Node *p, int j){
	/*
	 *  +--+--+--+--+
	 *  |aa|ab|ba|bb|
	 *  +--+--+--+--+
	 *  |ac|ad|bc|bd|
	 *  +--+--+--+--+
	 *  |ca|cb|da|db|
	 *  +--+--+--+--+
	 *  |cc|cd|dc|dd|
	 *  +--+--+--+--+
	 */

	assert(p->k >= 2);
	Node *result;
	if (p->n == 0)
		result = p->a;
	else if (p->k == 2)
		result = life4x4(p);
	else {
		j = j <= 0 ? p->k - 2 : min(j, p->k - 2);

		Node *c1 = successor(join(p->a->a, p->a->b, p->a->c, p->a->d), j);
		Node *c2 = successor(join(p->a->b, p->b->a, p->a->d, p->b->c), j);
		Node *c3 = successor(join(p->b->a, p->b->b, p->b->c, p->b->d), j);
		Node *c4 = successor(join(p->a->c, p->a->d, p->c->a, p->c->b), j);
		Node *c5 = successor(join(p->a->d, p->b->c, p->c->b, p->d->a), j);
		Node *c6 = successor(join(p->b->c, p->b->d, p->d->a, p->d->b), j);
		Node *c7 = successor(join(p->c->a, p->c->b, p->c->c, p->c->d), j);
		Node *c8 = successor(join(p->c->b, p->d->a, p->c->d, p->d->c), j);
		Node *c9 = successor(join(p->d->a, p->d->b, p->d->c, p->d->d), j);
		if (j < p->k - 2){
			result = join(
					join(c1->d, c2->c, c4->b, c5->a),
					join(c2->d, c3->c, c5->b, c6->a),
					join(c4->d, c5->c, c7->b, c8->a),
					join(c5->d, c6->c, c8->b, c9->a));
		} else {
			result = join(
					successor(join(c1, c2, c4, c5), j),
					successor(join(c2, c3, c5, c6), j),
					successor(join(c4, c5, c7, c8), j),
					successor(join(c5, c6, c8, c9), j));

		}
	}
	return result;
}


Node *advance(Node *p, int n){
	if (n==0)
		return p;

	int nbits = 0;
	int bits[n];
	while (n>0){
		bits[nbits] = n &1;
		n = n >> 1;
		p = centre(p);
		nbits++;
	}
	p = centre(p); // Another extra at last. Don't know why but it works

	
	for (int i = 0; i < nbits; i++){
		int j = nbits - i;
		if (bits[nbits - i - 1]){
			p = successor(p, j);
		}
	}
	return crop(p);
}


Node *life(Node *n1, Node *n2, Node *n3, Node *n4, Node *c, Node *n6, Node *n7, Node *n8, Node *n9){
	/*
	 *  +--+--+--+
	 *  |n1|n2|n3|
	 *  +--+--+--+
	 *  |n4|c |n6|
	 *  +--+--+--+
	 *  |n7|n8|n9|
	 *  +--+--+--+
	 *
	 * 1. Any live cell with fewer than two live neighbours dies, as if by underpopulation.
	 * 2. Any live cell with two or three live neighbours lives on to the next generation.
	 * 3. Any live cell with more than three live neighbours dies, as if by overpopulation.
	 * 4. Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
	 */

	// assert all node are level 0;
	assert(n1->k ^ n2->k ^ n3->k ^ n4->k ^ c->k ^ n6->k ^ n7->k ^ n8->k ^ n9 ->k == 0 && n1->k == 0);
	int nb = n1->n + n2->n + n3->n + n4->n + n6->n + n7->n + n8->n + n9->n;
	return ((c->n == 1 && nb == 2) || nb == 3) ? ON : OFF;
}

Node *life4x4(Node *p){
	/*
	 *  +--+--+--+--+
	 *  |aa|ab|ba|bb|
	 *  +--+--+--+--+
	 *  |ac|ad|bc|bd|
	 *  +--+--+--+--+
	 *  |ca|cb|da|db|
	 *  +--+--+--+--+
	 *  |cc|cd|dc|dd|
	 *  +--+--+--+--+
	 */

	assert(p->k == 2);
	Node *ad = life(p->a->a, p->a->b, p->b->a, p->a->c, p->a->d, p->b->c, p->c->a, p->c->b, p->d->a);
	Node *bc = life(p->a->b, p->b->a, p->b->b, p->a->d, p->b->c, p->b->d, p->c->b, p->d->a, p->d->b);
	Node *cb = life(p->a->c, p->a->d, p->b->c, p->c->a, p->c->b, p->d->a, p->c->c, p->c->d, p->d->c);
	Node *da = life(p->a->d, p->b->c, p->b->d, p->c->b, p->d->a, p->d->b, p->c->d, p->d->c, p->d->d);
	return join(ad, bc, cb, da);
}


/*** Utilities ***/
int is_padded(Node *p){
	if (p->k < 3)
		return 0;
	else 	
		return (
				p->a->n == p->a->d->d->n
				&& p->b->n == p->b->c->c->n
				&& p->c->n == p->c->b->b->n
				&& p->d->n == p->d->a->a->n);
}

Node *inner(Node *p){
	return join(p->a->d, p->b->c, p->c->b, p->d->a);
}

Node *crop(Node *p){
	if (p->k <= 3 || !is_padded(p))
		return p;
	else
		return crop(inner(p));
}

Node *centre(Node *p){
	Node *z = get_zero(p->k - 1);
	return join(
			join(z, z, z, p->a),
			join(z, z, p->b, z),
			join(z, p->c, z, z),
			join(p->d, z, z, z));
}

Node *pad(Node *p){
	if (p->k <= 3 || !is_padded(p))
		return pad(centre(p));
	else
		return p;
}

void print_node(const Node *node){ 
	printf("Node k=%d, %d x %d, population %d\n", node->k, 1 << node->k, 1 << node->k, node->n); 
}


int next_prime(int i) {
	int j ;
	i |= 1 ;
	for (;; i+=2) {
		for (j=3; j*j<=i; j+=2)
			if (i % j == 0)
				break;
		if (j*j > i)
			return i;
	}
}

/*** Tests ***/
void test_get_zero(){
	Node *p = get_zero(3);
	print_node(p);
	Node *p1 = get_zero(4);
	print_node(p1);
}

void test_construct(){
	int n = 3; // number of points
	int points[3][2] = {{0, 1}, {0, 2}, {0,6}};

	Node *p = construct(points, n);
	print_node(p);
}


void test_life(){
	/*
	 *  +--+--+--+
	 *  |  |x |  |
	 *  +--+--+--+
	 *  |x |  |x |
	 *  +--+--+--+
	 *  |  |  |  |
	 *  +--+--+--+
	 */

	Node *p =life(
			OFF, ON, OFF,
			ON , OFF, ON, 
			OFF, OFF, OFF);
	print_node(p);
}

void test_life4x4(){
	/*
	 *  +--+--+--+--+
	 *  |aa|  |  |  |
	 *  +--+--+--+--+
	 *  |  |  |x |  |
	 *  +--+--+--+--+
	 *  |  |  |x |  |
	 *  +--+--+--+--+
	 *  |  |  |  |  |
	 *  +--+--+--+--+
	 */

	int points[4][2] = {{0, 0}, {2, 1}, {2, 2}, {2, 3}};

	Node *p = construct(points, 4);
	log_info("The constructed node is: "); print_node(p);
	expand(p, 0, 0);
	p = life4x4(p);
	log_info("The out node is: "); print_node(p);
	expand(p, 0, 0);
}


void test_centre(){
	Node *p = join(OFF, OFF, OFF, OFF);
	log_info("Node before centre: "); print_node(p);
	p = centre(p);
	log_info("Node after centre: "); print_node(p);
	expand(p, 0, 0);
}


void test_pad(){
	Node *p = join(ON, OFF, OFF, OFF);
	log_info("Node before centre: "); print_node(p);
	p = pad(p);
	log_info("Node after centre: "); print_node(p);
	expand(p, 0, 0);
}


void test_successor(){
	int points[5][2] = {{0, 0}, {4, 1}, {4, 2}, {4, 3}, {10, 10}};

	Node *p = construct(points, 5);
	log_info("Before update: "); print_node(p);
	expand(p, 0, 0);
	p = successor(p, 0);
	log_info("After update1: ");print_node(p);
	expand(p, 0, 0);
	p = successor(p, 0);
	log_info("After update2: ");print_node(p);
	expand(p, 0, 0);

}

void test_new_collided(){
	// In order for this test to work, hardcode the hash_node to return h=2 in find_node and newnnode funciton
	Node *n1 = newnode(ON, ON, ON, ON);
	print_node(n1);
	Node *n2 = newnode(OFF, OFF, OFF, OFF);
	print_node(n2);
	printf("Popullation needs to be 0: "); print_node(hashtab[2]); // n2
	printf("Popullation needs to be 4: "); print_node(hashtab[2]->next); // n1
	//print_node(hashtab[2]->next->next); // NULL
	expand(n2, 0, 0);
}

void init_e(){
	E.cx = 0;
	E.cy = 0;
	E.gridrows=58;
	E.gridcols=238;
	E.screenrows=58;
	E.screencols=238;

	E.grid = calloc( E.gridcols, sizeof(int *) );
	for ( int i = 0; i < E.gridcols; i++ )
		E.grid[i] = calloc( E.gridrows, sizeof(int) );
}

