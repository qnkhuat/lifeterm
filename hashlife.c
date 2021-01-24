#include "hashlife.h"

extern Node **hashtab;
/*** Node operations ***/
Node *join(const Node *a, const Node *b, const Node *c, const Node *d){
	assert((a->k ^ b->k ^ c->k ^ d->k) == 0); // make sure all nodes are the same level
	Node *p;
	p = find_node(a, b, c, d);
	if (!p)
		p = newnode(a, b, c, d);
	return p;
}


void init(){ hashtab = (Node **)calloc(MAX_NODES, sizeof(Node *)); }

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
	assert(a->k < 30); // At development stage we want to make sure everything is in our control
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
#if DEBUG
	printf("Create new node: "); print_node(node);
#endif

	return node;
}

Node *find_node(Node *a, Node *b, Node *c, Node *d){
	int h = node_hash(a, b, c, d);
	Node *p = hashtab[h];
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
#if DEBUG
		printf("construct x:%d, y:%d\n", x, y);
#endif
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
	return result;
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


	// TODO: add assert x, y are inside the view

	// base case
	if (node->k == 0){
		E.grid[x][y] = 1;
#if DEBUG
		printf("expand x:%d, y:%d\n", x, y);
#endif
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
	 *  +---+---+---+---+++---+---+---+---+
	 *  |aaa aab aba abb|||baa bab bba bbb|
	 *  +   +   +   +   +++   +   +   +   +
	 *  |aac aad abc abd|||bac bad bbc bbd|
	 *  +   +   +   +   +++   +   +   +   +
	 *  |aca acb ada|adb|||bca bcb bda bdb|
	 *  +   +   +   +   +++   +   +   +   +
	 *  |acc acd adc add|||bcc bcd bdc bdd|
	 *  +---+---+---+---+++---+---+---+---+
	 *  +---+---+---+---+++---+---+---+---+
	 *  |caa cab cba cbb|||daa dab dba dbb|
	 *  +   +   +   +   +++   +   +   +   +
	 *  |cac cad cbc cbd|||dac dad dbc dbd|
	 *  +   +   +   +   +++   +   +   +   +
	 *  |cca ccb cda cdb|||dca dcb dda ddb|
	 *  +   +   +   +   +++   +   +   +   +
	 *  |ccc ccd cdc cdd|||dcc dcd ddc ddd|
	 *  +---+---+---+---+++---+---+---+---+
	 */

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

	Node *result;
	if (p->n ==0)
		result = p->a;
	else if (p->k == 2)
		result = life4x4(p);
	else {
		//j = j < p->k - 2 ? j : p->k -2;
		Node *c1 = successor(join(p->a->a, p->a->b, p->a->c, p->a->d), j);
		Node *c2 = successor(join(p->a->b, p->b->a, p->a->d, p->b->c), j);
		Node *c3 = successor(join(p->b->a, p->b->b, p->b->c, p->b->d), j);
		Node *c4 = successor(join(p->a->c, p->a->d, p->c->a, p->c->b), j);
		Node *c5 = successor(join(p->a->d, p->b->c, p->c->b, p->d->a), j);
		Node *c6 = successor(join(p->b->c, p->b->d, p->d->a, p->d->b), j);
		Node *c7 = successor(join(p->c->a, p->c->b, p->c->c, p->c->d), j);
		Node *c8 = successor(join(p->c->b, p->d->a, p->c->d, p->d->c), j);
		Node *c9 = successor(join(p->d->a, p->d->b, p->d->c, p->d->d), j);
		//if (j < p->k - 2){
		result = join(
				join(c1->d, c2->c, c4->b, c5->a),
				join(c2->d, c3->c, c5->b, c6->a),
				join(c4->d, c5->c, c7->b, c8->a),
				join(c5->d, c6->c, c8->b, c9->a));
		//} else {
		//	s = join(
		//			sucessor(join(c1->d, c2->c, c4->b, c5->a), j),
		//			sucessor(join(c2->d, c3->c, c5->b, c6->a), j),
		//			sucessor(join(c4->d, c5->c, c7->b, c8->a), j),
		//			sucessor(join(c5->d, c6->c, c8->b, c9->a), j));
		//}
	}
	return result;
}

Node *advance(Node *p, int step){
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
	 */

	// assert all node are level 0;
	assert(n1->k ^ n2->k ^ n3->k ^ n4->k ^ c->k ^ n6->k ^ n7->k ^ n8->k ^ n9 ->k == 0 && n1->k == 0);
	int nb = n1->n + n2->n + n3->n + n4->n +
		n6->n + n7->n + n8->n + n9->n;
	if ((c->n == 1 && nb == 2) || nb == 3)
		return ON;
	else 
		return OFF;
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
	if (p->k <= 3 || ! is_padded(p))
		return p;
	else
		return crop(inner(p));
}

Node *centre(Node *p){
	Node *z = get_zero(p->k-1);
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

void reset(){
	// free
	for ( int i = 0; i < E.gridcols; i++ )
		free(E.grid[i]);
	free(E.grid);

	// re-allocte with zeros
	E.grid = calloc( E.gridcols, sizeof(int *) );
	for ( int i = 0; i < E.gridcols; i++ )
		E.grid[i] = calloc( E.gridrows, sizeof(int) );
}

void render(Node *p){
	reset();
	expand(p, 0, 0);
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
//#if DEBUG
//				printf("grid x:%d, y:%d\n", x, y);
//#endif
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
	printf("The constructed node is: "); print_node(p);
	expand(p, 0, 0);
	p = life4x4(p);
	printf("The out node is: "); print_node(p);
	expand(p, 0, 0);
}


void test_centre(){
	Node *p = join(OFF, OFF, OFF, OFF);
	printf("Node before centre: "); print_node(p);
	p = centre(p);
	printf("Node after centre: "); print_node(p);
	expand(p, 0, 0);
}


void test_pad(){
	Node *p = join(ON, OFF, OFF, OFF);
	printf("Node before centre: "); print_node(p);
	p = pad(p);
	printf("Node after centre: "); print_node(p);
	expand(p, 0, 0);
}


void test_successor(){
	int points[4][2] = {{0, 0}, {4, 1}, {4, 2}, {4, 3}};

	Node *p = construct(points, 4);
	printf("Before update: "); print_node(p);
	expand(p, 0, 0);
	p = successor(p, 0);
	printf("After update1: ");print_node(p);
	expand(p, 0, 0);
	p = successor(p, 0);
	printf("After update2: ");print_node(p);
	expand(p, 0, 0);


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

int main(){
	init();
	init_e();
	//test_construct();
	//test_expand();
	//test_life();
	//test_life4x4();
	//test_centre();
	test_successor();
	return 0;
}

