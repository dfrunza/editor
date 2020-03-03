#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct EditorConfig {
  struct termios orig_termios;
  int screen_rows;
  int screen_cols;
};

struct EditorConfig editor_config = {0};

void die(char *s)
{
  perror(s);
  exit(1);
}

/*** Terminal ***/

void disableRawMode()
{
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_config.orig_termios)) {
    die("tcsetattr");
  }
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void enableRawMode()
{
  if (tcgetattr(STDIN_FILENO, &editor_config.orig_termios)) {
    die("tcgetattr");
  }
  atexit(disableRawMode);

  struct termios raw = editor_config.orig_termios;
  raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON | ICRNL);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag &= ~(CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)) {
    die("tcsetattr");
  }
}

char editorReadyKey()
{
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1) != 1)) {
    if (nread == -1 && errno != EAGAIN) {
      die("read");
    }
  }
  return c;
}

int getWindowSize(int* rows, int* cols)
{
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  }
  else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
  }
  return 0;
}

/*** Input ***/

char editorProcessKeyPress()
{
  char c = editorReadyKey();
  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }
}

/*** Output ***/

void editorDrawRows()
{
  for (int y = 0; y < editor_config.screen_rows; ++y) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen()
{
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** Main ***/

void initEditor()
{
  if (getWindowSize(&editor_config.screen_rows, &editor_config.screen_cols) == -1) {
    die("getWindowSize");
  }
}

int main()
{
  enableRawMode();
  initEditor();
  while (1) {
    editorRefreshScreen();
    editorProcessKeyPress();
  }
  return 0;
}
