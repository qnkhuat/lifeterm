#include "lifeterm.h"

/*** terminal ***/
void clearScreen() {
	write(STDOUT_FILENO, "\x1b[2J", 4); //4 means write 4 bytes out to terminal
	// \x1b ~ 27 ~ esc
	// esc + [ => means escape sequences
	// J is the erase command
	// 2 is the argument for J. and it means clear the entire screen
	write(STDOUT_FILENO, "\x1b[H", 3); // position cursor
}

void die(const char *s){
	// Exit program and with a message
	clearScreen();
	perror(s);
	exit(1);
}

void disableRawMode(){
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1 ) die("tcsetattr");
}

void enableRawMode(){
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");

	tcgetattr(STDIN_FILENO, &E.orig_termios);
	atexit(disableRawMode); // register to call this function after exit program

	struct termios raw = E.orig_termios;

	// c_iflag : input flags
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // IXON: prevent effect of Ctrl-S,-Q
	// ICRNL: fix Ctrl-M
	raw.c_cflag |= (CS8);

	// c_oflag : output flags
	raw.c_oflag &= ~(OPOST); // OPOST: turn off rendering things like \n as new line

	// c_flag : local flags
	raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); // ECHO turn off echo when we type 
	// ICANON : turn of CANONICAL mode
	// ISIG : prevent Ctrl-C, -Z to send sign
	// IEXTEN : prevent effect of Ctrl-V, -O
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1; // interval between reads

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


int editorReadKey() {
	// Wait for user input
	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1){
		if (nread == -1 && errno != EAGAIN) die("read");
	}

	// handle upper case cursor moving
	switch(c){
		case 'W': return W_UPPER;
		case 'A': return A_UPPER;
		case 'S': return S_UPPER;
		case 'D': return D_UPPER;
		case 'K': return W_UPPER;
		case 'H': return A_UPPER;
		case 'J': return S_UPPER;
		case 'L': return D_UPPER;

		case 'w': return ARROW_UP;
		case 'a': return ARROW_LEFT;
		case 's': return ARROW_DOWN;
		case 'd': return ARROW_RIGHT;
		case 'k': return ARROW_UP;
		case 'h': return ARROW_LEFT;
		case 'j': return ARROW_DOWN;
		case 'l': return ARROW_RIGHT;

		case 'i': return INC_BASE;
		case 'I': return DEC_BASE;

		case ' ':
		case 'x': 
							return MARK;

		case 'n':
		case 'u': return STEP;
		case 'r': return ERASE;

		case 'Q':
		case 'q': return QUIT;
	}

	if (c == '\x1b') {
		char seq[3];
		// These 2 reads are to ensure after ~ 0.1 second after user pressed a key starts with Esc
		// The next 2 bytes are belong to arrow keys
		// Note that arrow key is an Esc key with a char : A,B,C,D. E.g : \x1b[A
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

		// moving cursor keys
		if (seq[0] == '[' ) {
			// arrow keys
			switch (seq[1]) {
				case 'A': return ARROW_UP; // \x1b[A -> up arrows
				case 'B': return ARROW_DOWN; // \x1b[B -> down arrows
				case 'C': return ARROW_RIGHT;
				case 'D': return ARROW_LEFT;
			}
		}
		return '\x1b';
	} else {
		return c;
	}
}

int getCursorPosition(int *rows, int*cols){
	char buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; // send command to get the current cursor position
	// with n command parsed with a paramenter : 6 (6n)

	while(i < sizeof(buf) -1 ) { // after sending command. This is how we read the response
		// The response will have format : rows;colsR
		if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if (buf[i] == 'R') break; // break until we read the R char
		i++;
	}
	if (buf[0] != '\x1b' || buf[1] != '[') return -1;
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

	return 0;
}

int getWindowSize(int *rows, int *cols){
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ){
		// move cursor 999 forward (C command), move cursor 999 down (B command)
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12 ) return -1;
		return getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}

/*** append buffer ***/
void abAppend(struct abuf *ab, const char *c, int len){
	char *new = realloc(ab->b, ab->len + len);
	if (new == NULL) return;
	memcpy(&new[ab->len], c, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab){
	free(ab->b);
}

/*** grid operations ***/
void gridUpdateOrigin(){
  // Maintain the universe to be rendered at the center of screen
  // As the universe grow bigger, the origin willl be push to the upper left
	E.ox = E.screencols/2/2 - (1 << (E.root->k - 1)); 
  E.oy = E.screenrows/2 - ( 1 << (E.root->k - 1) );
}

void pushRoot(){
	E.root = centre(E.root);
  gridUpdateOrigin();
	log_warn("Expanding universe (%d x %d). Depth: %d", 1 << E.root->k, 1 << E.root->k, E.root->k);
}

void gridMark(){
	while(E.cx/2 - E.ox - E.offx < 0 || E.cy - E.oy - E.offy < 0 ||
		E.cx/2 - E.ox - E.offx > (1 << E.root->k) || E.cy - E.oy - E.offy > (1 << E.root->k))
		pushRoot();

  int x = E.cx/2 - E.ox - E.offx;
  int y = E.cy - E.oy - E.offy;
	mark(E.root, x, y);

	gridRender();
}

void emptyRoot(){
  E.root = get_zero(E.root->k);
  gridRender();
}
void gridErase(){
	// TODO : use memset to set values not for loop
	for (int col = 0; col < E.gridcols; col++){
		for (int row = 0; row < E.gridrows; row++){
			E.grid[row][col] = 0;
		}
	}
}

void gridUpdate(){
	int last_k = E.root->k;
	int step = pow(2, E.basestep);
	E.root = advance(E.root, step);
	if (last_k != E.root->k)
		log_warn("Expanding universe (%dx%d). Depth:%d", 1 << E.root->k, 1 << E.root->k, E.root->k);
	gridRender();
}

void gridPlay(){
	while(1){
		int c = editorReadKey();
		if (c == PLAY){
			gridUpdate();
			break;
		}
	}
}


void gridRender(){
	// By default the the upper left of the node will be (0, 0). 
	// In order to render consistently we push the orgin to the upper left as the level of Root increase.
	gridErase();
  gridUpdateOrigin();
	expand(E.root, E.ox + E.offx, E.oy + E.offy);
}


void changeBasestep(int order){
	// Can't decrease anymore
	if (E.basestep == 0 && order != 1)
		return;
	// 1 to incerase
	// else decrease speed
	E.basestep = order == 1 ? E.basestep + 1 : E.basestep - 1;
	log_info("%s base step to: 2^%d", order == 1 ? "Increased" : "Decreased", E.basestep);
}

/*** input ***/

void editorMoveCursor(int key){
	switch(key){
		case ARROW_LEFT:
			if (E.cx!=0) E.cx-=2;
			else E.offx+=2;
			break;
		case ARROW_RIGHT:
			if (E.cx <= E.gridcols-2) E.cx+=2;
			else E.offx-=2;
			break;
		case ARROW_UP:
			if(E.cy!=0)	E.cy--;
			else E.offy++;
			break;
		case ARROW_DOWN:
			if (E.cy != E.gridrows-1) E.cy++;
			else E.offy--;
			break;
		case A_UPPER:
			if (E.cx < 10){
				E.offx += 10 - E.cx;
				E.cx = 0;
			}
			else E.cx -= 10;
			break;
		case D_UPPER:
			if (E.cx + 10 >= E.screencols){
				E.offx -= 10 - (E.screencols-1 - E.cx);
				E.cx = E.screencols - 1;
			}
			else E.cx += 10;
			break;
		case W_UPPER:
			if (E.cy < 10){
				E.offy += 10 - E.cy;
				E.cy = 0;
			}
			else E.cy-=10;
			break;
		case S_UPPER:
			if (E.cy + 10 >= E.gridrows){
				E.offy -= 10 - (E.gridrows-1 - E.cy);
				E.cy = E.gridrows - 1;
			}
			else E.cy+=10;
			break;
	}
	gridRender();
}

void editorProcessKeypress(){
	int c = editorReadKey();
	switch(c){
		case QUIT:
		case CTRL_KEY('q'):
			clearScreen();
			exit(0);
			break;
		case ERASE:
      emptyRoot();
			break;
		case ARROW_LEFT:
		case ARROW_RIGHT:
		case ARROW_UP:
		case ARROW_DOWN:
		case W_UPPER:
		case A_UPPER:
		case S_UPPER:
		case D_UPPER:
			editorMoveCursor(c);
			break;

		case INC_BASE:
			changeBasestep(1);
			break;

		case DEC_BASE:
			changeBasestep(0);
			break;

		case MARK:
			gridMark();
			break;

		case STEP:
			gridUpdate();
			break;

		case PLAY:
			gridPlay();
			break;
	}
}

Node *readPattern(char* filename){
	FILE *fp;
	char line[10000];
	fp = fopen(filename, "r");
	Node *root;
	int indlen = 10;
	int inode = 1;
	Node **ind = (Node **)calloc(indlen, sizeof(Node *)); 
	ind[0] = get_zero(2) ; /* allow zeros to work right */
	while (fgets(line, 10000, fp) != NULL){
		if(line[0] == '#' | line[0] == '[' | strlen(line) <=1) // Skip the Header and rule line
			continue;

		if (inode > indlen - 1) {
			indlen += 10;
			ind = (Node **)realloc(ind, sizeof(Node*) * indlen) ;
		}

		log_info("Proecss line:%s", line);
		if (line[0] == '.' || line[0] == '*' || line[0] == '$') {
			// Each line represent an 8x8 node
			// "." representing an empty cell
			// "*" representing a live cell
			// "$" representing the end of line
			int cellnums = 0, x = 0, y = 0;
			int points[64][2];
			char *c = 0;
			int ipos = 0;
			for (c=line; *c > ' '; c++) {
				switch (*c){
					case '*':
						if (x > 7 || y < 0) {
							fprintf(stderr, "Illegal coordinates (%d,%d)\n", x, y) ;
							exit(10) ;
						}
						// in case the constructs output a 4x4 node, we need to know which position of this 4x4 is in 8x8
						// ipos keep track of it
						if (x < 4){
							if (y< 4)
								ipos = 1; // a
							else
								ipos = 3; // c
						}else{
							if (y< 4)
								ipos = 2; // b
							else
								ipos = 4; // d
						}
						points[cellnums][0] = x;
						points[cellnums][1] = y;
						log_info("Get new cell x:%d, y:%d", x, y);
						x++;
						cellnums++;
						break;
					case '.':
						x++;
						break;
					case '$':
						y++;
						x=0;
						break;
					default:       
						fprintf(stderr, "Illegal char %c\n", *c) ;
						exit(10) ;
				}
			}
			root = construct(points, cellnums);
			while(root->k < 3){
				root = join(
						ipos == 1 ? root : get_zero(root->k),
						ipos == 2 ? root : get_zero(root->k),
						ipos == 3 ? root : get_zero(root->k),
						ipos == 4 ? root : get_zero(root->k)
						);
				log_info("Expanding constructed to depth :%d ipos:%d", root->k, ipos);
			}
			ind[inode++] = root;
			//return root;
		} else {
			//Level 4 and above nodes are represented by five numbers: lev a b c d
			//where lev is the level and a, b, c d are for index quaters of the node 
			int n, ia, ib, ic, id, depth;
			n = sscanf(line, "%d %d %d %d %d", &depth, &ia, &ib, &ic, &id);
			if (n < 4) {
				fprintf(stderr, "Parse error; line is \"%s\"\n", line) ;
				exit(10) ;
			}
			ind[0] = get_zero(depth-1) ; /* allow zeros to work right */
			Node *p = find_node(ind[ia], ind[ib], ind[ic], ind[id]) ;
			if (p==NULL)
				p = join(ind[ia], ind[ib], ind[ic], ind[id]);
			root = ind[inode++] = p;
		}

	}
	fclose(fp);
	if (ind)
		free(ind);
	return root;
}

/*** output ***/
void editorDrawWelcomeMsg(struct abuf *ab){
	char welcome[80];
	int welcomelen = snprintf(welcome, sizeof(welcome),
			"Welcome to my LIFETERM -- version %s", LIFETERM_VERSION);

	if (welcomelen > E.screencols) welcomelen = E.screencols;

	int padding = (E.screencols - welcomelen) / 2;
	if(padding) abAppend(ab, "~", 1);
	while (padding --) abAppend(ab, " ", 1);

	abAppend(ab, welcome, welcomelen); // say welcome to users
}

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[7m", 4);// switch to inverted color
	char status[120], rstatus[120];

	int len = snprintf(status, sizeof(status), "press q to quit --- wasd|hjkl|ARROWS to navigate (upper case to move faster) --- x|space to mark --- u|n to update");
	int rlen = snprintf(rstatus, sizeof(rstatus), "Step: 2^%d | %d-%d",E.basestep, E.cx,  E.cy);
	if (len > E.screencols) len = E.screencols;
	abAppend(ab, status, len);
	while (len < E.screencols) {
		if (E.screencols - len == rlen) {
			abAppend(ab, rstatus, rlen);
			break;
		} else {
			abAppend(ab, " ", 1);
			len++;
		}
	}
	abAppend(ab, "\x1b[m", 3);// switch back to normal color
}


void editorDrawGrid(struct abuf *ab) {
	for (int row = 0; row < E.gridrows; row++){
		for (int col = 0; col < E.gridcols; col++){
			if (E.grid[row][col] == 1){
				abAppend(ab, "\x1b[7m", 4);// switch to inverted color
				abAppend(ab, "  ", 2);
				abAppend(ab, "\x1b[m", 3);// switch back to normal color
			}
			else
				abAppend(ab, "  ", 2);
		}
		abAppend(ab, "\r\n", 2);
	}
}


void editorRefreshScreen() {
	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6); // Turn off cursor before refresh 
	abAppend(&ab, "\x1b[H", 3); // clear screen

	editorDrawGrid(&ab);
	editorDrawStatusBar(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
	abAppend(&ab, buf, strlen(buf)); // position cursor at user current position

	abAppend(&ab, "\x1b[?25h", 6); // Turn on cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}



/*** init ***/
void initEditor(int argc, char *argv[]){
	if (getWindowSize(&E.screenrows, &E.screencols) == -1 ) die("WindowSize");
	E.cx = 0; E.cy = 0;
	E.offx = 0; E.offy = 0;
	E.gridrows = E.screenrows - 1; // status bar
	E.gridcols = E.screencols / 2;
	E.basestep= 0;
	
  // Init the grid to display
	E.grid = calloc( E.gridrows, sizeof(int *) );
	for ( int i = 0; i < E.gridrows; i++ )
		E.grid[i] = calloc( E.gridcols, sizeof(int) );

	init_hashtab();
	int n = 4;
	int points[4][2] = {{0, 0}, {0, 7}, {1, 7}, {2, 7}};
	//Node *root = construct(points, n);
  //E.root = root;
  if (argc == 2){
		E.root = readPattern(argv[1]);
    gridUpdateOrigin();
    //E.offx = -E.ox;
    //E.offy = -E.oy;
    // TODO : auto reallocate the pattern to the center
    
  }
	else
		E.root = get_zero(1);
	gridRender();

	log_warn("Universe Created: (%d x %d), Depth: %d, Population: %d, E.ox:%d, E.oy:%d, E.offx:%d, E.offy:%d", 
      1 << E.root->k, 1 << E.root->k, E.root->k, E.root->n, E.ox, E.oy, E.offx, E.offy);
}

int main(int argc, char *argv[] ){
  log_set_quiet(true);
  if (getenv("DEBUG")){
    FILE *fp = fopen("lifeterm.log", "a+");
    if (fp==NULL){
      printf("unable to open file to write log");
      return 0;
    }
    log_add_fp(fp, 3); // 3 is warn, 0 is trace
    log_info("Start");
    log_info("-------------------------------------------------------");
  } 	
	enableRawMode();
	initEditor(argc, argv);

	while(1){
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}

