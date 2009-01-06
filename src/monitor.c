#include "common.h"

#define VIDEO_MEMORY_BASE 0xB8000
#define SET_CURSOR_HIGH_BYTE 14
#define SET_CURSOR_LOW_BYTE 15
#define VIDEO_COMMAND_PORT 0x3d4
#define VIDEO_DATA_PORT 0x3d5

#define TERM_COLS 80
#define TERM_ROWS 25
#define BLACK 0
#define WHITE 15

#define SPACE 0x20
#define BACKSPACE 0x08
#define TAB 0x09
#define CR '\r'
#define LF '\n'

#define PACK_ATTRS(background,foreground) (((background) << 4) | ((foreground) & 0x0F))
#define PACK_CHAR(chr,attr) ((chr) | ((attr) << 8))

/* video memory */
u16int *video_memory = (u16int *)VIDEO_MEMORY_BASE;

/* cursor position, start at top left */
u16int cursor_x = 0;
u16int cursor_y = 0;

static void move_cursor()
{
  u16int cursorLocation = cursor_x * 80 + cursor_y;
  outb(VIDEO_COMMAND_PORT, SET_CURSOR_HIGH_BYTE); // set cursor high byte command
  outb(VIDEO_DATA_PORT, cursorLocation >> 8);
  outb(VIDEO_COMMAND_PORT, SET_CURSOR_LOW_BYTE);
  outb(VIDEO_DATA_PORT, cursorLocation);
}

static void scroll() 
{
  u8int attributeByte = PACK_ATTRS(BLACK,WHITE);
  u16int blank = PACK_CHAR(SPACE,attributeByte);

  // Row 25 is the end, this means we need to scroll up
  if(cursor_y >= TERM_ROWS)
    {
      // Move the current text chunk that makes up the screen
      // back in the buffer by a line
      int i;
      for (i = 0*TERM_COLS; i < (TERM_ROWS-1)*(TERM_COLS); i++) {
	video_memory[i] = video_memory[i+TERM_COLS];
      }
     
      // The last line should now be blank. Do this by writing
      // 80 spaces to it.
      for (i = (TERM_ROWS-1)*(TERM_COLS); i < TERM_ROWS*TERM_COLS; i++)
	{
	  video_memory[i] = blank;
	}
      // The cursor should now be on the last line.
      cursor_y = (TERM_ROWS)-1;
    }
} 

void monitor_put(char c)
{
  // background and foreground colors for the terminal
  u8int backColor = BLACK;
  u8int foreColor = WHITE;

  // attribute byte for the character to put
  u8int attributeByte = PACK_ATTRS(backColor,foreColor);

  // attr byte shifted into a word for the vga bios
  u16int attribute = attributeByte << 8;

  // crash early in the case of a bad pointer
  u16int *location = 0;

  // handle special character or put the character and attribute to the screen
  switch(c) {
  case BACKSPACE:
    if(cursor_x) cursor_x--;
    break;
  case TAB:
    cursor_x = (cursor_x + 8) & ~(8-1);
    break;
  case CR:
    cursor_x = 0;
    break;
  case LF:
    cursor_x = 0;
    cursor_y++;
    break;
  default:
    location = video_memory + (cursor_y*80 + cursor_x);
    *location = c | attribute;
    cursor_x++;
    break;
  }

  // check to see if we've reached the edge of the screen and wrap the line if we have
  if(cursor_x >= 80) {
    cursor_x = 0;
    cursor_y++;
  }

  scroll();
  move_cursor();
}

void monitor_clear()
{
  // blank character = 'white' space on black bg
  u8int attribute_byte = PACK_ATTRS(BLACK, WHITE);
  u8int blank = PACK_CHAR(SPACE,attribute_byte);
  
  for(int i = 0; i < TERM_COLS * TERM_ROWS; ++i) {
    video_memory[i] = blank;
  }
  
  cursor_x = 0;
  cursor_y = 0;
  move_cursor();

}

void monitor_write(char *c)
{
  int i = 0;
  while(c[i]) {
    monitor_put(c[i++]);
  }

}
