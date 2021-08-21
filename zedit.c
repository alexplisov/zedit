#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct state {
  int running;
  int columns;
  int rows;
  int cursor_x;
  int cursor_y;
  char *buffer;
  int buffer_length;
};

void init(struct state *s, struct termios *original_terminal_state,
          struct termios *terminal_state) {
  tcgetattr(STDIN_FILENO, original_terminal_state);
  tcgetattr(STDIN_FILENO, terminal_state);
  cfmakeraw(terminal_state);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, terminal_state);
  s->buffer_length = 0;
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[1;1H", 6);
}

void input(struct state *s) {
  char input[1];
  int input_length = read(STDIN_FILENO, &input, 1);
  if (input_length < 0)
    return;
  switch (input[0]) {
  case 'q':
    s->running = 0;
    break;
  default: {
    const int symbol_size = 1;
    char *buffer = realloc(s->buffer, s->buffer_length + symbol_size);
    if (buffer == NULL)
      return;
    memcpy(&buffer[s->buffer_length], &input, symbol_size);
    s->buffer = buffer;
    s->buffer_length += symbol_size;
  }
  }
}

void update(struct state *s) {}

void display(struct state *s) {
  write(STDIN_FILENO, "\x1b[2J", 4);
  write(STDIN_FILENO, "\x1b[1;1H", 6);
  write(STDOUT_FILENO, s->buffer, s->buffer_length);
}

void die(struct termios *original_terminal_state) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, original_terminal_state);
  write(STDIN_FILENO, "\x1b[2J", 4);
}

int main(int argc, char *argv[]) {
  struct state state = {1, 0, 0, 0, 0, NULL, 0};
  struct termios original_terminal_state;
  struct termios terminal_state;

  init(&state, &original_terminal_state, &terminal_state);
  while (state.running) {
    input(&state);
    update(&state);
    display(&state);
  }
  die(&original_terminal_state);
}
