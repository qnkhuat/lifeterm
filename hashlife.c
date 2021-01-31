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


void init(){ 
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
	log_info("Create new node: Node k=%d, %d x %d, population %d", node->k, 1 << node->k, 1 << node->k, node->n); 
	return node;
}

Node *find_node(Node *a, Node *b, Node *c, Node *d){
	int h = node_hash(a, b, c, d);
	Node *p;
	for (p=hashtab[h]; p; p = p->next) { /* make sure to compare a first */
		if (p->a == a && p->b == b && p->c == c && p->d == d) {// In case hash collision compare its value
			log_info("Find: hash collided");
			return p ;
		}
	}
	p = NULL;
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

Node *construct(int points[][2], int n){

	// Translate points to origin
	int min_x = INT_MAX;
	int min_y = INT_MAX;
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
		log_info("construct x:%d, y:%d", x, y);
		pattern[i] = (MapNode){.p = ON, .x = x, .y = y}; 
	}

	int k = 0;
	while (n > 1){ // until there are only node node left
		Node *z = get_zero(k);
		MapNode *next_level = malloc(n * sizeof(MapNode));
		int m = 0; // store number of node in this level
		for (int i = 0; i < n; i++){ // Each loop is to construct one level
			MapNode p = pattern[i];
			if (p.p == NULL)
				continue;

			Node *a = z, *b = z, *c = z, *d = z ;
			int x = p.x - (int)(p.x & 1); int y = p.y - (int)(p.y & 1); // Move index to the start of its block
			for (int j = i; j < n; j++){ // Find all points that inside this 2x2 block
				MapNode pp = pattern[j];
				if (pp.p == NULL)
					continue;

				if (pp.x == x && pp.y == y){
					a = pp.p;
					// TODO: Are there a way we can remove pointer using pp instead of original pattern[j]?
					pattern[j].p = NULL;
				} else if (pp.x == x + 1 && pp.y == y){
					b = pp.p;
					pattern[j].p = NULL;
				} else if (pp.x == x && pp.y == y + 1){
					c = pp.p;
					pattern[j].p = NULL;
				} else if (pp.x == x + 1 && pp.y == y + 1){
					d = pp.p;
					pattern[j].p = NULL;
				}
			}

			Node *nodek = find_node(a, b, c, d);
			if (nodek == NULL) // create new if does not exist
				nodek = newnode(a, b, c, d);
			next_level[m] = (MapNode){.x = x >> 1, .y = y >> 1, . p = nodek}; // store a list of all pattern in this level
			m++;
		}
		n = m; k++;
		free(pattern);
		pattern = next_level;
	}

	Node *result = pattern->p;
	free(pattern); // Can't let the garbage floatting around
	result = pad(result);
	log_info("Constructed node: Node k=%d, %d x %d, population %d", result->k, 1 << result->k, 1 << result->k, result->n); 
	return pad(result);
}


void mark(Node *node, int x, int y, int mx, int my){
	// (mx, my) are not in this node
	int size = 1 << node->k;
	if (mx < x || mx > x + size || my < y || my > y + size)
		return;

	// base case
	if (node->k == 1){
		if(x == mx && y == my){
			E.grid[x][y] = ~E.grid[x][y]; // flip it boiss
		}
		return;
	}
	// TODO: if the marked node is out of current size => expand

	int offset = 1 << (node->k - 1);
	mark(node->a, x, y, mx, my);
	mark(node->b, x + offset, y, mx, my);
	mark(node->c, x, y + offset, mx, my);
	mark(node->d, x + offset, y + offset, mx ,my);

}

void expand(Node *node, int x, int y){
	if (node->n == 0)
		return;

	int size = 1 << node->k;
	// clip only points in view
	if (x + size <= E.x || x >= E.x + E.screencols || y + size <= E.y || y >= E.y + E.screenrows)
		return;

	// base case
	if (node->k == 0){
		E.grid[y][x] = 1;
		log_info("expand x:%d, y:%d", x, y);
		return;
	}

	int offset = 1 << (node->k - 1);
	expand(node->a, x, y);
	expand(node->b, x + offset, y);
	expand(node->c, x, y + offset);
	expand(node->d, x + offset, y + offset);
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

	assert(p->k >=2);
	Node *result;
	if (p->n ==0)
		result = p->a;
	else if (p->k == 2)
		result = life4x4(p);
	else {
		j = j < p->k - 2 ? j : p->k -2;
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
					successor(join(c1->d, c2->c, c4->b, c5->a), j),
					successor(join(c2->d, c3->c, c5->b, c6->a), j),
					successor(join(c4->d, c5->c, c7->b, c8->a), j),
					successor(join(c5->d, c6->c, c8->b, c9->a), j));
		}
	}
	return result;
}

Node *advance(Node *p, int n){
	// TODO : add advance expansion
	if (n==0)
		return p;
	p = centre(p); // p->k+=1
	p = successor(p, 0); // p->k-=1
	return p;
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
				break ;
		if (j*j > i)
			return i ;
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



//void test_expand(){
//	int n = 3; // number of points
//	int points[3][2] = {{0, 1}, {2, 2}, {2,6}};
//
//	Node *p = construct(points, n);
//	print_node(p);
//	mark(p, 0, 0, 3, 3);
//	render(p);
//	for (int x = 0; x < E.gridcols; x++){
//		for (int y = 0; y < E.gridrows ; y++){
//			if (E.grid[x][y] == 1){
//				log_info("grid x:%d, y:%d", x, y);
//			}
//		}
//	}
//}
//

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
	int points[4][2] = {{0, 0}, {4, 1}, {4, 2}, {4, 3}};

	Node *p = construct(points, 4);
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
	E.x = 0;
	E.y = 0;
	E.cx = 0;
	E.cy = 0;
	E.playing = 0;
	E.gridrows=58;
	E.gridcols=238;
	E.screenrows=58;
	E.screencols=238;

	E.grid = calloc( E.gridcols, sizeof(int *) );
	for ( int i = 0; i < E.gridcols; i++ )
		E.grid[i] = calloc( E.gridrows, sizeof(int) );
}

