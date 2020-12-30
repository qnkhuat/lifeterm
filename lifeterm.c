#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>


/*** defines ***/
// fix errors when run getline
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#define KILO_VERSION "0.0.0"
#define KILO_TAB_STOP 4

#define CTRL_KEY(k) ((k) & 0x1f) // & in this line is bitwise-AND operator

enum editorKey {
	BACKSPACE = 127,
	ARROW_LEFT = 1000,
	ARROW_RIGHT, // = 1001 by convention
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	A_UPPER,
	D_UPPER,
	W_UPPER,
	S_UPPER,
	PAGE_UP,
	PAGE_DOWN,
};

/*** data ***/

// The typedef lets us refer to the type as erow instead of struct erow
typedef struct erow{ // editor row
	int size;
	int rsize;
	char *chars;
	char *render;
} erow;

typedef enum {false, true} bool;


struct editorConfig { 
	int cx, cy;
	int rx;
	int rowoff, coloff; // screen scroll
	int screenrows, screencols;
	int numrows;
	erow *row;
	int dirty;
	bool edit;
	char *filename;
	char statusmsg[80];
	time_t statusmsg_time;
	struct termios orig_termios;
};


struct editorConfig E;

/*** prototypes ***/
void editorSetStatusMessage(const char *fmt, ...);



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
	if (!E.edit){
		switch(c){
			case 'W': return W_UPPER;
			case 'A': return A_UPPER;
			case 'S': return S_UPPER;
			case 'D': return D_UPPER;
			case 'w': return ARROW_UP;
			case 'a': return ARROW_LEFT;
			case 's': return ARROW_DOWN;
			case 'd': return ARROW_RIGHT;
		}
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
			// Page up, page down
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1': return HOME_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
						case '3': return DEL_KEY;
					}
				}
			} else {	
				// arrow keys
				switch (seq[1]) {
					case 'A': return ARROW_UP; // \x1b[A -> up arrows
					case 'B': return ARROW_DOWN; // \x1b[B -> down arrows
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}
		}else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
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
		if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
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

/*** row operations ***/
int editorRowCxToRx(erow *row, int cx) {
	// Handle when cursor go to a tab, it won't stay at middle of the tab
	int rx = 0;
	int j;
	for (j = 0; j < cx; j++) {
		if (row->chars[j] == '\t')
			rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
		rx++;
	}
	return rx;
}


void editorUpdateRow(erow *row){
	int j, tabs = 0;
	// count number of tabs in a line
	for (j=0; j < row->size; j++)
		if (row->chars[j] == '\t') tabs++;

	free(row->render);
	row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);

	// copy text to render
	int idx = 0;
	for (j=0;j < row->size; j++){
		if (row->chars[j] =='\t') {
			row->render[idx++] = ' ';
			while(idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
		} else{
			row->render[idx++] = row->chars[j];
		}
	}

	row->render[idx] = '\0';
	row->rsize = idx;


}

void editorAppendRow(char *s, size_t len){
	E.row = realloc(E.row, sizeof(erow) * (E.numrows +1));

	int at = E.numrows; // Current row index
	E.row[at].size = len;
	E.row[at].chars = malloc(len+1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';

	E.row[at].rsize = 0;
	E.row[at].render = NULL;
	editorUpdateRow(&E.row[at]);

	E.numrows++;
	E.dirty++;

}

void editorRowInsertChar(erow *row, int at, int c){
	if (at < 0 || at > row->size) at = row->size;
	row->chars = realloc(row->chars, row->size + 2);
	memmove(&row->chars[at+1], &row->chars[at], row->size - at + 1);
	row->size++;
	row->chars[at] = c;
	editorUpdateRow(row);
	E.dirty++;
}

/*** editor operations ***/
void editorInsertChar(int c){
	if (E.cy == E.numrows)
		editorAppendRow("", 0);// insert a new line if at the end of file

	editorRowInsertChar(&E.row[E.cy], E.cx, c);
	E.cx++;
}

/*** file i/o ***/
char *editorRowsToString(int *buflen){
	int totlen = 0;
	int j;
	for(j=0; j < E.numrows; j++)
		totlen += E.row[j].size + 1;
	*buflen = totlen;

	char *buf = malloc(totlen);
	char *p = buf;
	for(j=0; j < E.numrows; j++){
		memcpy(p, E.row[j].chars, E.row[j].size);
		p += E.row[j].size;
		*p = '\n';
		p++;
	}
	return buf;
}


void editorOpen(char* filename){
	free(E.filename);
	E.filename = strdup(filename);
	FILE *fp = fopen(filename, "r");
	if (!fp) die("fopen");

	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		// strip out the last char if it is \n or \r
		while (linelen > 0 && (
					line[linelen -1] == '\n' || line[linelen -1] == '\r')){
			linelen--;
		}
		editorAppendRow(line, linelen);
	}

	free(line);
	fclose(fp);
	E.dirty = 0; // set flag of modified 
}

void editorSave(){
	if (E.filename == NULL) return;

	int len;
	char *buf = editorRowsToString(&len);

	int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
	if (fd != -1){
		if (ftruncate(fd, len) != -1){
			if (write(fd, buf, len) == len){
				close(fd);
				free(buf);
				E.dirty = 0; // reset modified status
				editorSetStatusMessage("%d bytes written to disk", len);
				return;
			}
		}
		close(fd);
	}
	free(buf);
	editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}


/*** append buffer ***/
struct abuf {
	char *b;
	int len;
};

// acts as constructor for the abuf type
#define ABUF_INIT {NULL, 0}

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


/*** input ***/

void editorMoveCursor(int key){
	erow *row = (E.cy >= E.numrows) ? NULL: &E.row[E.cy];

	switch(key){
		case ARROW_LEFT:
			if (E.cx!=0) E.cx--;
			break;
		case ARROW_RIGHT:
			if (row && E.cx < row->size) {
				E.cx++;
			}
			break;
		case ARROW_UP:
			if(E.cy!=0)	E.cy--;
			break;
		case ARROW_DOWN:
			if (E.cy < E.numrows) E.cy++;
			break;
		case A_UPPER:
			if (E.cx -10 <= 0) E.cx = 0;
			else E.cx -= 10;
			break;
		case D_UPPER:
			if (row && E.cx < row->size) 
				E.cx+=10;
			break;
		case W_UPPER:
			if (E.cy - 10 <= 0) E.cy = 0;
			else E.cy-=10;
			break;
		case S_UPPER:
			if (E.cy + 10 >= E.numrows) E.cy = E.numrows-1;
			else E.cy+=10;
			break;
	}

	row = (E.cy >= E.numrows) ? NULL: &E.row[E.cy];
	int rowlen = row ? row->size : 0;
	if (E.cx > rowlen)
		E.cx = rowlen;

}

void editorProcessKeypress(){
	int c = editorReadKey();

	switch(c){
		case '\r':
			/* TODO */
			break;
		case CTRL_KEY('q'):
			clearScreen();
			exit(0);
			break;

		// toggle editor mode
		case CTRL_KEY('i'):
			E.edit = !E.edit;
			break;
		
		case CTRL_KEY('s'):
      editorSave();
      break;


		case HOME_KEY:
			E.cx = 0;
			break;
		case END_KEY:
			E.cx = E.screencols - 1;
			break;

		case BACKSPACE:
    case CTRL_KEY('h'): // ~ to BACKSPACE
    case DEL_KEY:
      /* TODO */
      break;

		case PAGE_UP:
		case PAGE_DOWN:
			{
				if (c == PAGE_UP) {
					E.cy = E.rowoff;
				} else if (c == PAGE_DOWN) {
					E.cy = E.rowoff + E.screenrows - 1;
					if (E.cy > E.numrows) E.cy = E.numrows;
				}
				int times = E.screenrows;
				while (times--)
					editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
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

		case CTRL_KEY('l'):
    case '\x1b': // esc
      break;
		default:
			if(E.edit) // if in edit mode
				editorInsertChar(c);
			break;
	}

}

/*** output ***/
void editorScroll() {
	E.rx = 0;
	if (E.cy < E.numrows) 
		E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);

	if (E.cy >= E.rowoff + E.screenrows) 
		E.rowoff = E.cy - E.screenrows + 1;

	if (E.rx < E.coloff) 
		E.coloff = E.rx;

	if (E.rx >= E.coloff + E.screencols) 
		E.coloff = E.rx - E.screencols + 1;

}


void editorDrawWelcomeMsg(struct abuf *ab){
	char welcome[80];
	int welcomelen = snprintf(welcome, sizeof(welcome),
			"Welcome to my VIM  -- version %s", KILO_VERSION);

	if (welcomelen > E.screencols) welcomelen = E.screencols;

	int padding = (E.screencols - welcomelen) / 2;
	if(padding) abAppend(ab, "~", 1);
	while (padding --) abAppend(ab, " ", 1);

	abAppend(ab, welcome, welcomelen); // say welcome to users
}

void editorDrawRows(struct abuf *ab){
	for(int y=0; y< E.screenrows; y++){
		int filerow = y + E.rowoff;
		if(filerow >= E.numrows) {
			if (E.numrows == 0 && y==E.screenrows / 3) editorDrawWelcomeMsg(ab);
			else abAppend(ab, "~", 1);
		} else { // display text line read from file
			int len = E.row[filerow].rsize - E.coloff;
			if (len<0) len = 0;
			if (len > E.screencols ) len = E.screencols;
			abAppend(ab, E.row[filerow].render+E.coloff, len);
		}
		abAppend(ab, "\x1b[K", 3); // clear the current line
		abAppend(ab, "\r\n", 2);
	}
}

void editorDrawStatusBar(struct abuf *ab) {
	abAppend(ab, "\x1b[7m", 4);// switch to inverted color
	char status[120], rstatus[120];

	int len = snprintf(status, sizeof(status), 
		"%.20s - %d lines %s %s",
    E.filename ? E.filename : "[No Name]", E.numrows,
		E.edit ? "(mode:insert)" : "(mode:visual)",
    E.dirty ? "(modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
			E.cy + 1, E.numrows);
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
	abAppend(ab, "\r\n", 2);
}


void editorDrawMessageBar(struct abuf *ab) {
	abAppend(ab, "\x1b[K", 3);
	int msglen = strlen(E.statusmsg);
	if (msglen > E.screencols) msglen = E.screencols;
	if (msglen && time(NULL) - E.statusmsg_time < 5)
		abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() {
	editorScroll();
	struct abuf ab = ABUF_INIT;

	abAppend(&ab, "\x1b[?25l", 6); // Turn off cursor before refresh 
	abAppend(&ab, "\x1b[H", 3); // clear screen

	editorDrawRows(&ab); // draw all lines have start with char: ~
	editorDrawStatusBar(&ab);
	editorDrawMessageBar(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
			(E.rx - E.coloff) + 1);
	abAppend(&ab, buf, strlen(buf)); // position cursor at user current position

	abAppend(&ab, "\x1b[?25h", 6); // Turn on cursor

	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...){ // ... defined this function a variadict function
	va_list ap; // access arguments from ...
	va_start(ap, fmt);
	vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
	va_end(ap);
	E.statusmsg_time = time(NULL);
}

/*** init ***/

void initEditor(){
	E.cx = 0;
	E.cy = 0;// point to index of opened file, not index of screen
	E.rx = 0;
	E.rowoff = 0;
	E.coloff= 0;
	E.numrows = 0;
	E.row = NULL;
	E.dirty = 0;
	E.edit = false;
	E.filename = NULL;
	E.statusmsg[0] = '\0';
	E.statusmsg_time = 0;

	if (getWindowSize(&E.screenrows, &E.screencols) == -1 ) die("WindowSize");
	E.screenrows -= 2;
}

int main(int argc, char *argv[]){
	enableRawMode();
	initEditor();
	if (argc >= 2) {
		editorOpen(argv[1]);
	}

	editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit");

	while(1){
		editorRefreshScreen();
		editorProcessKeypress();
	}

	return 0;
}
