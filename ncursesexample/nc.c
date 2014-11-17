#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include "nc.h"

// Textinput from ui
char textbuffer[BUFFER] = {0};
int textinput = 0;

// Chatlog
char chatlog[LOGSIZE][BUFFER];
int logposition = 0;
int logentries = 0;

// Some status variables
int status = 0;

// Horizontal line for grid
char* line = NULL;

gamearea* new_gamearea(int width, int height, int blockcount, uint8_t blocks[][BLOCKSIZE]) {
  // Allocate game struct
  gamearea *game = (gamearea*)malloc(sizeof(gamearea));
  
  // Set the game area
  game->array = (char*)malloc(width*height);
  game->width = width;
  game->height = height;
  memset(game->array,0,width*height);
  
  // Set the blocks
  if(blockcount > BLOCKSMAX) blockcount = BLOCKSMAX;
  memcpy(game->blocks,blocks,sizeof(uint8_t)*blockcount*BLOCKSIZE);
  game->blockcount = blockcount;
  
  return game;
}

void free_gamearea(gamearea* game) {
  if(game) {
    free(game->array);
    free(game);
  }
}

// New player
player* new_player(uint8_t id, uint8_t startx, uint8_t starty, uint8_t health) {
  player *p = (player*)malloc(sizeof(p));
  p->id = id;
  p->posx = startx;
  p->posy = starty;
  p->health = health;
  p->hitwall = 0;
  p->hitrepeat = 0;
  return p;
}

void free_player(player* p) {
  if(p) free(p);
}

int is_position_a_wall(gamearea *game, int posx, int posy) {

	for(int i = 0; i < game->blockcount; i++) if(game->blocks[i][0] == posx && game->blocks[i][1] == posy) return 1;
	// Right wall
	if(posx >= game->width) return 1;
	// Left wall
	if(posx < 0) return 1;
	// Bottom wall
	if(posy >= game->height) return 1;
	// Top wall
	if(posy < 0) return 1;
	
	return 0;
}



void ui_draw_grid(gamearea* game, player* pl) {
	int x = 0, y = 0, index = 0, was_wall = 0;
	char pchar = ' ';

	clear(); // clear screen

	if(pl->hitwall) pl->hitrepeat++;
	else pl->hitrepeat = 0;

	printw("Player ");
	attron(COLOR_PAIR(pl->id));
	printw("%d",pl->id);
	attroff(COLOR_PAIR(pl->id));
	printw(" at x = %d, y = %d\n",pl->posx,pl->posy);
	attron(COLOR_PAIR(pl->id));
	printw("LIFE[%d]",pl->health);
	attroff(COLOR_PAIR(pl->id));
	printw("\n%s\n", pl->hitwall ? "OUCH! HIT A WALL (LOST 1 HEALTH)" : "TRAVELLING...");
	printw("Game status: %s\n",status == 0 ? "not ready" : "ready" );
	printw("Connection status: %s\n",status == 0 ? "disconnected" : "connected" );
	printw("%s\n",line);

	// Draw grid
	for(index = 0; index < game->width * game->height; index++,x++) {
		// Start a new line?
		if(index % game->width == 0) {
			x = 0; // reset the x coordinate
			// The first index is always 0, skip it
			if(index > 0) {
				printw("|\n%s\n",line);
				y++;
			}
		}
		printw("|");
		
		// The player
		if(x == pl->posx && y == pl->posy) {
			if(pl->hitwall) attron(COLOR_PAIR(PLAYER_HIT_COLOR)); // Did player hit a wall
			else attron(COLOR_PAIR(pl->id)); // Normal color
			pchar = 'o'; // Player mark
		}
		else pchar = ' '; // Otherwise empty mark

		// Is it a wall
		if(is_position_a_wall(game,x,y) == 1) {
		  attron(COLOR_PAIR(WALL_COLOR)); // Paint it white
		  was_wall = 1;
		}

		// Put the mark on the grid
		printw(" %c ",pchar);
		
		// If player hit a wall disable the color
		if(pl->hitwall) attroff(COLOR_PAIR(PLAYER_HIT_COLOR));
		else attroff(COLOR_PAIR(pl->id));
		
		// Wall painted, disable the color
		if(was_wall == 1) attroff(COLOR_PAIR(WALL_COLOR));
		was_wall = 0;
	}
	printw("|\n%s\n",line);
	if(pl->hitrepeat > 1) printw("STOP HITTING THE WALL YOU'LL KILL YOURSELF");
	if(textinput==1) printw("\tTYPING: %s",textbuffer);
	printw("\n%s\n",line);
	printw("\nCHAT\n");
	for(int row = 0; row < LOGSIZE; row++) {
	  if(strlen(&chatlog[row][0]) > 0) printw("\t%d: %s\n",row+logentries,&chatlog[row][0]);
	}
	refresh();
}

void ui_draw_end(int death) {
  clear();
  attron(A_BOLD); // Bold text
  
  if(death) {
    // Blinking sequence
    for(int i = 0; i < 10; i++) {
      // On
      printw("YOU ARE DEAD!\n");
      refresh();
      usleep(200000);
      // Off
      clear();
      refresh();
      usleep(200000);
    }
  }
  
  clear();
  printw("Quitting.\n");
  attroff(A_BOLD);
  refresh();
}

void prepare_horizontal_line(int width) {
  line = (char*)malloc(sizeof(char)*(width*4+1));
  memset(line,'-',width*4+1);
}

void free_horizontal_line() {
  if(line) free(line);
}

void clear_log() {
  for(int i = 0; i < LOGSIZE; i++) {
  memset(&chatlog[i][0],0,BUFFER);
  }
}

void add_log(char* message, int msglen) {
  // Log is less than maximum
  if(logposition < LOGSIZE) {
    memset(&chatlog[logposition][0],0,BUFFER);
    memcpy(&chatlog[logposition][0],message,msglen); // Copy to the position
    logposition++; // Increase counter
  }
  // Log is full
  else {
    // Move the entries 
    for(int i = 0; i < LOGSIZE-1; i++) memcpy(&chatlog[i][0],&chatlog[i+1][0],BUFFER);
    memset(&chatlog[LOGSIZE-1][0],0,BUFFER);
    memcpy(&chatlog[LOGSIZE-1][0],message,msglen); // Replace the last with the new one
    logentries++; // Increase counter
  }
}


int main() {

  // Initial blocks
  uint8_t blocks[5][2] = { {2,1}, {3,5}, {4,5}, {4,6}, {9,8}};

  // Initial start position
  uint8_t startx = WIDTH / 2;
  uint8_t starty = HEIGHT / 2;
	
  int readc = 0, quit = 0, playerid = PLAYER1;
  int textpos = 0;
	
  // Game area
  gamearea* game = new_gamearea(WIDTH,HEIGHT,5,blocks);
	
  // Player
  player* pl1 = new_player(playerid,startx,starty,HEALTH);

  initscr(); // Start ncurses
  noecho(); // Disable echoing of terminal input
  cbreak(); // Individual keystrokes
  intrflush(stdscr, FALSE); // Prevent interrupt flush
  keypad(stdscr,TRUE); // Allow keypad usage 

  start_color(); // Initialize colors

  // Color pairs init_pair(colorpair id,foreground color, background color)
  init_pair(PLAYER1,PLAYER1,COLOR_BLACK); // Player1 = COLOR_RED (1)
  init_pair(PLAYER2,PLAYER2,COLOR_BLACK); // Player2 = COLOR_GREEN (2)
  init_pair(PLAYER3,PLAYER3,COLOR_BLACK); // Player3 = COLOR_YELLOW (3)
  init_pair(PLAYER4,PLAYER4,COLOR_BLACK); // Player4 = COLOR_BLUE (4)
  init_pair(PLAYER_HIT_COLOR,COLOR_RED,COLOR_YELLOW);
  init_pair(WALL_COLOR,COLOR_WHITE,COLOR_WHITE);

  // Prepare everything
  clear_log();
  prepare_horizontal_line(WIDTH);
  ui_draw_grid(game, pl1);

  fd_set readfs;
  int rval = 0;

  while(1) {
    FD_ZERO(&readfs);
    FD_SET(fileno(stdin),&readfs);
 
    // Block until we have something
    if((rval = select(fileno(stdin)+1,&readfs,NULL,NULL,NULL)) > 0) {

      // From user
      if(FD_ISSET(fileno(stdin),&readfs)) {
        readc = getch(); // Get each keypress
        pl1->hitwall = 0;

        switch(readc) {
          case KEY_LEFT:
            if(is_position_a_wall(game,pl1->posx-1,pl1->posy)) pl1->hitwall = 1;
            else pl1->posx--;
            break;
          case KEY_RIGHT:
            if(is_position_a_wall(game,pl1->posx+1,pl1->posy)) pl1->hitwall = 1;
            else pl1->posx++;
            break;
          case KEY_UP:
            if(is_position_a_wall(game,pl1->posx,pl1->posy-1)) pl1->hitwall = 1;
            else pl1->posy--;
            break;
          case KEY_DOWN:
            if(is_position_a_wall(game,pl1->posx,pl1->posy+1)) pl1->hitwall = 1;
            else pl1->posy++;
            break;
          // Function keys, here F1 is reacted to
          case KEY_F(1):
            status = status ^ 1;
            break;
          case 27: // Escape key
            quit = 1;
            break;
          case '/':
            // User wants to write something
            memset(&textbuffer,0,BUFFER);
            textinput = 1;
            textpos = 0;
            break;
          // Erase text
          case KEY_BACKSPACE:
          case KEY_DC:
            textpos--;
            textbuffer[textpos] = '\0';
            break;
          // Push the line to log with enter
          case KEY_ENTER:
          case '\n':
            textinput = 0;
            if(strlen(textbuffer) > 0) add_log(textbuffer,textpos);
            textpos = 0;
            memset(&textbuffer,0,BUFFER);
            break;
          // Add the character to textbuffer if we were inputting text
          default:
            if(textinput == 1) {
              textbuffer[textpos] = readc;
              textpos++;
              if(textpos == BUFFER-1) {
                textpos = 0;
                textinput = 0;
              }
            }
            break;
          }
        }
      }

    // Hit a wall, change player
    if(pl1->hitwall) {
      pl1->health--;
      if(pl1->id == PLAYER4) pl1->id = PLAYER1;
      else pl1->id++;
    }

    // Update screen
    ui_draw_grid(game, pl1);

    // Suicide
    if(pl1->health == 0) {
      ui_draw_end(1);
      break;
    }
		
    // Surrended
    if(quit) {
      ui_draw_end(0);
      break;
    }
  }
  free_gamearea(game);
  free_player(pl1);
  free_horizontal_line();
  sleep(1);
  endwin(); // End ncurses
  return 0;
}

