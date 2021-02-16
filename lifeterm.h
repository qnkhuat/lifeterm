#ifndef LIFETERM
#define LIFETERM
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <string.h>
#include "hashlife.h"
#include "log.h"


/*** defines ***/
#define LIFETERM_VERSION "0.0.0"

#define CTRL_KEY(k) ((k) & 0x1f) // & in this line is bitwise-AND operator


/*** structs ***/

void editorRefreshScreen();

enum editorKey {
	ARROW_LEFT = 1000,
	ARROW_RIGHT, // = 1001 by convention
	ARROW_UP,
	ARROW_DOWN,
	A_UPPER,
	D_UPPER,
	W_UPPER,
	S_UPPER,
	STEP,
	PLAY,
	MARK,
	ERASE,
	QUIT
};

struct abuf {
	char *b;
	int len;
};


struct editorConfig { 
	int cx, cy; // Position of Cursor
	int ox, oy; // Origin of the root node
	int offx, offy; // Offset of the universe when move to the edges
	int screenrows;
	int screencols;
	int gridrows;
	int gridcols;
	int playing;
	int **grid;
	struct Node *root;
	struct termios orig_termios;
};

/*** terminal ***/
void clearScreen();
void die(const char *s);
void disableRawMode();
void disableRawMode();
int editorReadKey();
int getCursorPosition(int *rows, int*cols);
int getWindowSize(int *rows, int *cols);


/*** buffer operators ***/
void abAppend(struct abuf *ab, const char *c, int len);
void abFree(struct abuf *ab);


/*** grid operations ***/
void gridMark();
void gridErase();
void gridUpdate();
void gridRender();
void gridPlay();


/*** input ***/
void editorMoveCursor(int key);
void editorProcessKeypress();


/*** output ***/
void editorDrawWelcomeMsg(struct abuf *ab);
void editorDrawStatusBar(struct abuf *ab);
void editorDrawGrid(struct abuf *ab);
void editorRefreshScreen();


/*** init ***/
void initEditor();


/*** Global ***/
// acts as constructor for the abuf type
#define ABUF_INIT {NULL, 0}
struct editorConfig E;

#endif
