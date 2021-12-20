#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

/* This program will read the first 3 lines of input 
    and prints a static 2D maze*/

typedef struct mazeStruct {
    char** arr;
    int xsize, ysize;
    int xstart, ystart;
    int xend, yend;
} maze;

typedef struct node {
    int xpos;
    int ypos;
    struct node* next;
} node;

typedef struct stack {
    node* topNode;
    int numCoins;
} stack;

bool checkFile(FILE* src);

stack* init(stack* myStack);
int is_empty(stack* myStack);
stack* push(stack* myStack, int xpos, int ypos, bool debugMode);
stack* pop(stack* myStack, bool debugMode);
node* top(stack* myStack);
stack* clear(stack* myStack, bool debugMode);

maze* initDynMaze(maze *m1);
void markBorders(FILE *src, maze *m1);
void allocateMaze(FILE *src, maze *m1);
void prepMaze(FILE *src, maze *m1);
bool errorCheck(maze *m1, int xpos, int ypos);
void fillMaze(FILE *src, maze *m1);
void outputMaze(maze *m1, bool debugMode);
void createMaze(FILE *src, maze *m1, bool debugMode);

void attemptMove(maze *m1, stack* path, bool debugMode);
int findPath(maze *m1, stack *path, bool debugMode);
void printReverse(node* topNode);
void attemptEscape(maze *m1, bool debugMode);
void freeMaze(maze *m1);

int main (int argc, char **argv) {
    maze m1;
    bool debugMode;
    int xpos, ypos;
    int i,j,k;

    FILE *src;

    /* verify the proper number of command line arguments were given */
    if(argc > 3) {
        printf("Usage: %s <input file name>\n", argv[0]);
        exit(-1);
    }
    
    /* search for debug flag */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            debugMode = true;
            k = i;
            break;
        }
    }

    if (k == 1) {
        k = 2;
    } else {
        k = 1;
    }

    /* Try to open the input file. */
    if ( ( src = fopen( argv[k], "r" )) == NULL ) {
        printf ( "Can't open input file: %s", argv[k] );
        exit(-1);
    }
    if (!checkFile(src)) {
        printf("Invalid data file\n");
        exit(-1);
    }
    src = fopen(argv[k], "r");

    // create and fill maze
    createMaze(src, &m1, debugMode);

    /*Close the file*/
    fclose(src);
        
    // output maze
    outputMaze(&m1, debugMode);

    // attempt to escape the maze
    attemptEscape(&m1, debugMode);

    // free maze
    freeMaze(&m1);

}

bool checkFile(FILE* src) {
    char c = 'z';
    int numLines = 0;
    while (c != EOF) {
        c = getc(src);
        if (c == '\n') {
            numLines++;
        }
        if (numLines == 3) {
            fclose(src);
            return true;
        }
    }
    
    fclose(src);
    return false;
}

// stack related =============================================================

stack* init(stack* myStack) {
    myStack->topNode = NULL;
    myStack->numCoins = 0;
    return myStack;
}

int is_empty(stack* myStack) {
    if (myStack->topNode == NULL)
        return 1;
    else
        return 0;
}

stack* push(stack* myStack, int xpos, int ypos, bool debugMode) {
    node* new = (node*)malloc(sizeof(node));
    new->xpos = xpos;
    new->ypos = ypos;

    if (myStack->topNode == NULL) {
        new->next = NULL;
    } else {
        new->next = myStack->topNode;
    }

    myStack->topNode = new;
    if (debugMode) {
        printf("(%d, %d) pushed into the stack.\n", new->xpos, new->ypos);
    }

    return myStack;
}

stack* pop(stack* myStack, bool debugMode) {
    node* temp = myStack->topNode;
    myStack->topNode = myStack->topNode->next;

    if (debugMode) {
        printf("(%d, %d) popped off the stack.\n", temp->xpos, temp->ypos);
    }

    free(temp);

    return myStack;
}

node* top(stack* myStack) {
    return myStack->topNode;
}

stack* clear(stack* myStack, bool debugMode) {
    while (!is_empty(myStack)) {
        pop(myStack, debugMode);
    }
    myStack->topNode = NULL;
    return myStack;
}

// maze related ==============================================================

maze* initDynMaze(maze *m1) {
    int xsize = m1->xsize+2;
    int ysize = m1->ysize+2;
    m1->arr = (char**)malloc(sizeof(char*)*xsize);
    for (int i = 0; i < m1->xsize+2; i++) {
        m1->arr[i] = (char*)malloc(sizeof(char)*ysize);
    }
    return m1;
}

void markBorders(FILE *src, maze *m1) {
    int i;
    for (i=0; i < m1->xsize+2; i++) {
        m1->arr[i][0] = '*';
        m1->arr[i][m1->ysize+1] = '*';
    }
    for (i=0; i < m1->ysize+2; i++) {
        m1->arr[0][i] = '*';
        m1->arr[m1->xsize+1][i] = '*';
    }
    for (i=0; i < m1->xsize+2; i++) {
        m1->arr[i][0] = '*';
        m1->arr[i][m1->ysize+1] = '*';
    }
    for (i=0; i < m1->ysize+2; i++) {
        m1->arr[0][i] = '*';
        m1->arr[m1->xsize+1][i] = '*';
    }
}

void allocateMaze(FILE *src, maze *m1) {
    int xpos, ypos;

    /* read in first 3 lines of file */
    fscanf(src, "%d %d", &m1->xsize, &m1->ysize);
    if (m1->xsize < 1 || m1->ysize < 1) {
        printf("Maze sizes must be greater than 0.\n");
        exit(-1);
    }
    printf ("size: %d, %d\n", m1->xsize, m1->ysize);

    initDynMaze(m1);

    fscanf (src, "%d %d", &m1->xstart, &m1->ystart);
    if (m1->xstart < 1 || m1->xstart > m1->xsize || m1->ystart < 1 || m1->ystart > m1->ysize) {
        printf("Start/End position outside of maze range\n");
        exit(-1);
    }
    printf ("start: %d, %d\n", m1->xstart, m1->ystart);

    fscanf (src, "%d %d", &m1->xend, &m1->yend);
    if (m1->xend < 1 || m1->xend > m1->xsize || m1->yend < 1 || m1->yend > m1->ysize) {
        printf("Start/End position outside of maze range\n");
        exit(-1);
    }
    printf ("end: %d, %d\n", m1->xend, m1->yend);

}

void prepMaze(FILE *src, maze *m1) {
    int i, j;

    allocateMaze(src, m1);


    /* initialize the maze to empty */
    for (i = 0; i < m1->xsize+2; i++) {
        for (j = 0; j < m1->ysize+2; j++) {
            m1->arr[i][j] = '.';
        }
    }

    markBorders(src, m1);
}

bool errorCheck(maze *m1, int xpos, int ypos) {
    if (xpos == m1->xstart && ypos == m1->ystart) {
        printf("Invalid coordinates: attempting to block start/end position.\n");
        return true;
    } else if (xpos == m1->xend && ypos == m1->yend) {
        printf("Invalid coordinates: attempting to block start/end position.\n");
        return true;
    } else if (xpos > m1->xsize || xpos < 1 || ypos > m1->ysize || ypos < 1) {
        printf("Invalid coordinates: outside of maze range.\n");
        return true;
    }

    return false;
}

void fillMaze(FILE *src, maze *m1) {
    int xpos, ypos;
    char c;

    /* mark the starting and ending positions in the maze */
    m1->arr[m1->xstart][m1->ystart] = 's';
    m1->arr[m1->xend][m1->yend] = 'e';

    while (fscanf(src, "%d %d %c", &xpos, &ypos, &c) != EOF) {
        bool dontAdd = false;
        if (errorCheck(m1, xpos, ypos)) {
            dontAdd = true;
        }

        switch (c) {
            case 'c' :
                c = 'C';
                break;
            case 'b' :
                c = '*';
                break;
            default :
                printf("Invalid type: type is not recognized.\n");
                dontAdd = true;
        }

        if (!dontAdd) {
            m1->arr[xpos][ypos] = c;
        }
    }
}

void outputMaze(maze *m1, bool debugMode) {
    int i, j;

    /* print out the initial maze */
    for (i = 0; i < m1->xsize+2; i++) {
        for (j = 0; j < m1->ysize+2; j++)
        printf ("%c", m1->arr[i][j]);
        printf("\n");
    }
}

void createMaze(FILE *src, maze *m1, bool debugMode) {
    prepMaze(src, m1);
    fillMaze(src, m1);
}

// related to finding exit to maze ============================================

void attemptMove(maze *m1, stack* path,  bool debugMode) {
    int xCurr = top(path)->xpos;
    int yCurr = top(path)->ypos;

    if (m1->arr[xCurr+1][yCurr] != '*') {
        xCurr++;
        push(path, xCurr, yCurr, debugMode);
    } else if (m1->arr[xCurr][yCurr+1] != '*') {
        yCurr++;
        push(path, xCurr, yCurr, debugMode);

    } else if (m1->arr[xCurr-1][yCurr] != '*') {
        xCurr--;
        push(path, xCurr, yCurr, debugMode);

    } else if (m1->arr[xCurr][yCurr-1] != '*') {
        yCurr--;
        push(path, xCurr, yCurr, debugMode);

    } else {
        if (m1->arr[top(path)->xpos][top(path)->ypos] == 'c') {
            path->numCoins--;
            m1->arr[xCurr][yCurr] == '*';
        }
        pop(path, debugMode);
    }

    if (m1->arr[xCurr][yCurr] == 'C') {
        path->numCoins += 1;
    }
    m1->arr[xCurr][yCurr] = '*';
}

int findPath(maze *m1, stack *path, bool debugMode) {
    push(path, m1->xstart, m1->ystart, debugMode);
    while (top(path)->xpos != m1->xend || top(path)->ypos != m1->yend) {
        attemptMove(m1, path, debugMode);
        if (is_empty(path)) {
            return 0;
        }
    }
    
    return 1;
}

void printReverse(node* topNode) {
    if (topNode == NULL) {
        return;
    }
    
    printReverse(topNode->next);
    printf("(%d,%d) ", topNode->xpos, topNode->ypos);
}

void attemptEscape(maze *m1, bool debugMode) {
    stack path;
    init(&path);
    if (findPath(m1, &path, debugMode) == 1) {
        printf("The maze has a solution.\n");
        printf("The amount of coins collected: %d\n", path.numCoins);
        printf("The path from start to end: \n");
        printReverse(top(&path));
        printf("\n");

    } else {
        printf("This maze has no solution.\n");
        return;
    }

    clear(&path, debugMode);
}

void freeMaze(maze *m1) {
    for (int i = 0; i < m1->xsize+2; i++) {
        free(m1->arr[i]);
    }
    free(m1->arr);
}