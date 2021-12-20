#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

/* This program will read the first 3 lines of input 
   and prints a static 2D maze*/

typedef struct Array {
    char a;
    bool hasCoin;
    bool visited;
} array;

typedef struct mazeStruct
{
    array arr[32][32]; /* allows for a maze of size 30x30 plus outer walls */
    int xsize, ysize;
    int xstart, ystart;
    int xend, yend;
} maze;

typedef struct Coord {
    int xpos;
    int ypos;
} coord;

typedef struct stack {
    coord* coordList;
    int top;
    int size;
} Stack;

//
// markBorders()
//
// for use with prepMaze(), marks the border of the maze with *
// 
void markBorders(FILE *src, maze *m1) {
    int i;
    for (i=0; i < m1->xsize+2; i++) {
        m1->arr[i][0].a = '*';
        m1->arr[i][m1->ysize+1].a = '*';
    }
    for (i=0; i < m1->ysize+2; i++) {
        m1->arr[0][i].a = '*';
        m1->arr[m1->xsize+1][i].a = '*';
    }
    for (i=0; i < m1->xsize+2; i++) {
        m1->arr[i][0].a = '*';
        m1->arr[i][m1->ysize+1].a = '*';
    }
    for (i=0; i < m1->ysize+2; i++) {
        m1->arr[0][i].a = '*';
        m1->arr[m1->xsize+1][i].a = '*';
    }
}

//
// prepMaze()
//
// create a maze that fits the specifications found in the first 3 lines of the file
//
void prepMaze(FILE *src, maze *m1) {
    int xpos, ypos;
    int i, j;

    /* read in first 3 lines of file */
    fscanf(src, "%d %d", &m1->xsize, &m1->ysize);
    if (m1->xsize < 1 || m1->ysize < 1) {
        printf("Maze sizes mus be greater than 0.\n");
        exit(-1);
    }

    fscanf (src, "%d %d", &m1->xstart, &m1->ystart);
    if (m1->xstart < 1 || m1->xstart > m1->xsize || m1->ystart < 1 || m1->ystart > m1->ysize) {
        printf("Start position outside of maze range\n");
        exit(-1);
    }

    fscanf (src, "%d %d", &m1->xend, &m1->yend);
    if (m1->xend < 1 || m1->xend > m1->xsize || m1->yend < 1 || m1->yend > m1->ysize) {
        printf("End position outside of maze range\n");
        exit(-1);
    }

    /* initialize the maze to empty */
    for (i = 0; i < m1->xsize+2; i++) {
        for (j = 0; j < m1->ysize+2; j++) {
            m1->arr[i][j].a = '.';
            m1->arr[i][j].visited = false;
            m1->arr[i][j].hasCoin = false;
        }
    }

    markBorders(src, m1);
}

//
// errorCheck()
//
// Check for errors when adding blocks and coins to the maze.
//
void errorCheck(maze *m1, int xpos, int ypos) {
    if (xpos == m1->xstart && ypos == m1->ystart) {
        printf("Invalid coordinates: attempting to block start position.\n");
        exit(-1);
    } else if (xpos == m1->xend && ypos == m1->yend) {
        printf("Invalid coordinates: attempting to block end position.\n");
        exit(-1);
    } else if (xpos > m1->ysize || xpos < 1 || ypos > m1->ysize || ypos < 1) {
        printf("Invalid coordinates: outsize of maze range.\n");
        exit(-1);
    }
}

//
// createMaze()
//
// Read in the rest of the maze and fill in any coins and blocked positions
//
void createMaze(FILE *src, maze *m1) {
    int xpos, ypos;
    int i, j;

    /* mark the starting and ending positions in the maze */
    m1->arr[m1->xstart][m1->ystart].a = 's';
    m1->arr[m1->xend][m1->yend].a = 'e';

    char c;
    while (fscanf(src, "%d %d %c", &xpos, &ypos, &c) != EOF) {
        errorCheck(m1, xpos, ypos);

        switch (c) {
            case 'c' :
                c = 'C';
                m1->arr[xpos][ypos].hasCoin = true;
                break;
            case 'b' :
                c = '*';
                m1->arr[xpos][ypos].visited = true;
                break;
            default :
                printf("Error, invalid marker.\n");
                exit(-1);
        }
        
        m1->arr[xpos][ypos].a = c;
    }
}

//
// init()
//
// Given an uninitalized stack, initialize the stack
// array with 4 memory locations availabile and return it.
// 
Stack* init(Stack* myStack) {
    myStack->size = 4;
    myStack->top = 0;

	myStack->coordList = (coord*)malloc(sizeof(coord)*myStack->size);
	return myStack;
}

//
// is_empty()
//
// given a stack, check whether the stack is empty.
// if empty return true, if stack has one or more symbols return false
//
bool is_empty(Stack* myStack) {
    if (myStack->top == 0) {
        return true;
    }
    return false;
}

//
// grow()
//
// given a stack, grow the array by allocating 4 more memory locations
//
Stack* grow(Stack* myStack, bool debugMode) {
    coord* temp = (coord*)malloc(sizeof(coord)*myStack->size);
    int oldSize = myStack->size;
  
    for (int i = 0; i < oldSize; i++) {
        temp[i] = myStack->coordList[i];
    }

    // increase stack size by 4
    myStack->size = myStack->size + 4;
    int numMoved = 0;
    
    free(myStack->coordList);
    myStack->coordList = (coord*)malloc(sizeof(coord)*myStack->size);
  
    // only access index positions that were edited in temp array.
    for (int i = 0; i < oldSize; i++) {
        myStack->coordList[i] = temp[i];
        numMoved++;
    }
    if (debugMode)
        printf("Stack size increased from %d to %d, a total of %d values were copied \n", oldSize, myStack->size, numMoved);

    free(temp);
    return myStack;
}

//
// push()
//
// Given a stack and a symbol, push the symbol onto the stack. If the stack
// is full, increase the memory size by 4 before pushing the symbol.
//
void push(Stack* myStack, int xcoord, int ycoord, bool debugMode) {
	
    // check if stack needs to be expanded
    if (myStack->top+1 == myStack->size) {
        myStack = grow(myStack, debugMode);
    }

    if (debugMode)
        printf("(&d, &d) was pushed into the stack\n", xcoord, ycoord);

    myStack->top++;
    coord newCoord;
    newCoord.xpos = xcoord;
    newCoord.ypos = ycoord;
    myStack->coordList[myStack->top] = newCoord;
	
    return;
}

//
// pop()
//
// Given a stack, "pop" the top symbol and return it by decrementing the
// top variable by one.
// If the stack is empty, do nothing and output a prompt notifying the user.
//
coord pop(Stack* myStack, bool debugMode) {
    coord c;
    if (myStack->top == 0) {
        // stack is empty, 
        return c;
    } else {
        c = myStack->coordList[myStack->top];
        myStack->top--;
    }

    if (debugMode) {
      printf("(%d, %d) was popped from the stack\n", c.xpos, c.ypos);
    }
    return c;

}

//
// top()
//
// Given a stack, return the symbol that is at the top of the stack.
//
coord top(Stack* myStack) {
    coord c;
    if (is_empty(myStack)) {
        return c;
    } else {
        c = myStack->coordList[myStack->top];
        return c;
    }
}

// 
// clear()
//
// clears the stack so that it is empty
//
Stack* clear(Stack* myStack) {
    free(myStack->coordList);
    init(myStack);
    return myStack;
}

void attemptMove(maze *m1, Stack *solution, bool debugMode, int *numCoins) {
    // down > right > left > up
    int xCurr = top(solution).xpos;
    int yCurr = top(solution).ypos;

    printf("current position is: %d, %d\n", xCurr, yCurr);

    if (m1->arr[xCurr+1][yCurr].visited != true && m1->arr[xCurr+1][yCurr].a != '*') {
        xCurr++;
        push(solution, xCurr, yCurr, debugMode);
        m1->arr[xCurr][yCurr].visited = true;
        printf("going down\n");
    } else if (m1->arr[xCurr][yCurr+1].visited != true && m1->arr[xCurr][yCurr+1].a != '*') {
        yCurr++;
        push(solution, xCurr, yCurr, debugMode);
        m1->arr[xCurr][yCurr].visited = true;
        printf("going right\n");

    } else if (m1->arr[xCurr-1][yCurr].visited != true && m1->arr[xCurr-1][yCurr].a != '*') {
        xCurr--;
        push(solution, xCurr, yCurr, debugMode);
        m1->arr[xCurr][yCurr].visited = true;
        printf("going up\n");

    } else if (m1->arr[xCurr][yCurr-1].visited != true && m1->arr[xCurr][yCurr-1].a != '*') {
        yCurr--;
        push(solution, xCurr, yCurr, debugMode);
        m1->arr[xCurr][yCurr].visited = true;
        printf("going left\n");

    } else {
        pop(solution, debugMode);
        if (m1->arr[xCurr][yCurr].hasCoin) {
            numCoins--;
        }
        printf("popping\n");
        return;
    }
    sleep(1);

    if (m1->arr[xCurr][yCurr].hasCoin) {
        printf("got coin\n");
        numCoins++;
    }

}

bool traverse(maze *m1, int *numCoins, Stack *solution, bool debugMode) {
    
    /*
        Push start coord onto stack
        Check for end pos
            if end, break
            else
                push unvisited neighbor onto stack
    */
    init(solution);

    // push start coord
    m1->arr[m1->xstart][m1->ystart].visited = true;
    
    push(solution, m1->xstart, m1->ystart, debugMode);

    while (!is_empty(solution)) {
        // check for end position
        if (top(solution).xpos == m1->xend && top(solution).ypos == m1->yend) {
            return true;
        }
        attemptMove(m1, solution, debugMode, numCoins);
    }

    return false;

}

void outputAnswer(Stack *solution, int *numCoins, bool debugMode) {
    Stack reversed;
    init(&reversed);
    while (!is_empty(solution)) {
        int xPos = top(solution).xpos;
        int yPos = top(solution).ypos;
        pop(solution, debugMode);
        push(&reversed, xPos, yPos, debugMode);
    }
    
    return;
}

void attemptExit(maze *m1, bool debugMode) {
    int numCoins;
    Stack solution;

    if (traverse(m1, &numCoins, &solution, debugMode)) {
        outputAnswer(&solution, &numCoins, debugMode);
    }
}

int main (int argc, char **argv)
{
    maze m1;
    bool debugMode = false;
    int xpos, ypos;
    int i,j;

    FILE *src;

    /* verify the proper number of command line arguments were given */
    if (argc > 2) {
        printf("Too many input files.\n");
        exit(-1);
    } else if(argc != 2) {
        printf("Usage: %s <input file name>\n", argv[0]);
        exit(-1);
    }
   
    /* Try to open the input file. */
    if ( ( src = fopen( argv[1], "r" )) == NULL ) {
        printf ( "Can't open input file: %s", argv[1] );
        exit(-1);
    }

    /* create maze */
    prepMaze(src, &m1);
    createMaze(src, &m1);

    /*Close the file*/
    fclose(src);

    /* print out the initial maze */
    for (i = 0; i < m1.xsize+2; i++) {
        for (j = 0; j < m1.ysize+2; j++)
            printf ("%c", m1.arr[i][j].a);
    printf("\n");
    }

    /* attempt to traverse maze */
    attemptExit(&m1, debugMode);
}