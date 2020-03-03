#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#define arrlen(a) (sizeof(a) / sizeof(a[0]))
#define ERR_RESULT -1
#define true 1
#define false 0
typedef int bool;

struct termios original_termattrs;

void restore_original_termattrs() {
  tcsetattr(STDIN_FILENO, TCSANOW, &original_termattrs);
}

void app_exit(char* failed_action) {
  restore_original_termattrs();
  perror(failed_action);
  exit(EXIT_FAILURE);
}

void sleep_msec(float msec) {
  struct timespec requested;
  int nsec = msec*1000*1000;
  if(msec < 0) {
    nsec = 0.0;
  } else if(msec > 1000.0) {
    nsec = 1000.0*1000*1000 - 1.0;
  }
  requested.tv_sec = 0;
  requested.tv_nsec = nsec;
  if(nanosleep(&requested, 0) == ERR_RESULT) {
    app_exit("nanosleep");
  }
}

typedef enum {
  Stream_None,
  Stream_Screen,
  Stream_Log,
  Stream_Input,
  Stream__Count,
} eStream;

void ed_write(eStream stream, char* data) {
  int fileno = -1;
  if(stream == Stream_Screen) {
    fileno = STDOUT_FILENO;
  } else if(stream == Stream_Log) {
    fileno = STDERR_FILENO;
  } else {
    assert(false);
  }
  int count = write(fileno, data, strlen(data));
  if(count == ERR_RESULT) {
    app_exit("write");
  } else if (count == 0) {
    assert(false);
  }
}

void ed_print(char* txt) {
  ed_write(Stream_Screen, txt);
}

void ed_log(char* txt) {
  ed_write(Stream_Log, txt);
}

bool cstr_match(char* str_a, char* str_b)
{
  while(*str_a == *str_b)
  {
    str_a++;
    str_b++;
    if(*str_a == '\0')
      break;
  }
  bool result = (*str_a == *str_b);
  return result;
}

int main() {
  int exit_status = EXIT_SUCCESS;

  if(!isatty(STDIN_FILENO)) {
    ed_log("STDIN is not a terminal\n");
    exit_status = EXIT_FAILURE;
    goto quit;
  }
  char* tty_name = ttyname(STDIN_FILENO);

  struct termios termattrs;
  if(tcgetattr(STDIN_FILENO, &original_termattrs) == ERR_RESULT) {
    app_exit("tcgetattr");
  }
  if(tcgetattr(STDIN_FILENO, &termattrs) == ERR_RESULT) {
    app_exit("tcgetattr");
  }
  termattrs.c_lflag &= ~(ICANON | ECHO);
  termattrs.c_cc[VMIN] = 0;
  termattrs.c_cc[VTIME] = 0;
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &termattrs) == ERR_RESULT) {
    app_exit("tcsetattr");
  }
  atexit(restore_original_termattrs);

  ed_print("\e[2J"); // clear screen
  ssize_t count = 0;
  char buf[32];
  while(true) {
    count = read(STDIN_FILENO, buf, arrlen(buf)-1);
    if(count >= 0) {
      buf[count] = '\0';
      if(count == 0) {
        sleep_msec(30.0);
      } else if(count == 1) {
        char c = buf[0];
        if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
          ed_print(buf);
        }
        if(c == '\x7f') { // Backspace
          ed_print("\x08");
        }
      } else if(count >= 1) {
        if(cstr_match(buf, "\e[A") || // Up
           cstr_match(buf, "\e[B") || // Down
           cstr_match(buf, "\e[C") || // Left
           cstr_match(buf, "\e[D")) { // Right
          ed_print(buf);
        }
      }
    } else {
      app_exit("read");
    }
  }

quit:
  return exit_status;
}
