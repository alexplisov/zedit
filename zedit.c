#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k)&0x1f)

struct state;
void display(struct state *s);

struct state
{
	int running;
	int columns;
	int rows;
	int cursor_x;
	int cursor_y;
	char **buffer;
	int buffer_length;
	char filename[255];
};

void
save_file (struct state *s)
{
	if (strlen(s->filename) < 1)
		sprintf(s->filename, "buffer");
	FILE *file = fopen (s->filename, "w");
	for (int i = 0; i < s->rows; i++)
		{
			fwrite (s->buffer[i], s->columns * sizeof (char), 1, file);
		}
	fclose (file);
}

void
load_file (struct state *s)
{
	if (!*s->filename)
		return;
	FILE *file = fopen (s->filename, "r");
	if (file)
		{
			for (int i = 0; i < s->rows; i++)
				{
					fread (s->buffer[i], s->columns * sizeof (char), 1, file);
				}
			fclose (file);
		}
}

int
get_cursor_position (int *rows, int *cols)
{
	char buf[32];
	unsigned int i = 0;
	write (STDOUT_FILENO, "\x1b[6n", 4);
	while (i < sizeof (buf) - 1)
		{
			if (read (STDIN_FILENO, &buf[i], 1) != 1)
				break;
			if (buf[i] == 'R')
				break;
			i++;
		}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[')
		return -1;
	if (sscanf (&buf[2], "%d;%d", rows, cols) != 2)
		return -1;

	return 0;
}

void
init (struct state *s, struct termios *original_terminal_state,
	  struct termios *terminal_state)
{
	struct winsize w;
	ioctl (STDOUT_FILENO, TIOCGWINSZ, &w);
	s->columns = w.ws_col;
	s->rows = w.ws_row;
	tcgetattr (STDIN_FILENO, original_terminal_state);
	tcgetattr (STDIN_FILENO, terminal_state);
	cfmakeraw (terminal_state);
	tcsetattr (STDIN_FILENO, TCSAFLUSH, terminal_state);
	unsigned long rows_size = s->rows * sizeof (char *);
	s->buffer = malloc (rows_size);
	memset (s->buffer, 0, rows_size);
	for (int i = 0; i < s->rows; i++)
		{
			unsigned long columns_size = s->columns * sizeof (char);
			s->buffer[i] = malloc (columns_size);
			memset (s->buffer[i], 32, columns_size);
		}
	write (STDIN_FILENO, "\x1b[2J", 4);
	write (STDIN_FILENO, "\x1b[H", 3);
	get_cursor_position (&s->cursor_y, &s->cursor_x);
	load_file (s);
	display(s);
}

void
input (struct state *s)
{
	char input;
	int input_length = read (STDIN_FILENO, &input, 1);
	if (input_length < 0)
		return;
	switch (input)
		{
		case CTRL_KEY ('q'):
			s->running = 0;
			break;
		case CTRL_KEY ('s'):
			save_file (s);
			break;
		case 13:
			s->cursor_x = 0;
			s->cursor_y++;
			break;
		case 127:
			s->cursor_x--;
			memcpy (&s->buffer[s->cursor_y][s->cursor_x - 1], " ", 1);
			break;
		case '\x1b':
			{
				char seq[3];
				if (read (STDIN_FILENO, &seq[0], 1) != 1)
					return;
				if (read (STDIN_FILENO, &seq[1], 1) != 1)
					return;
				switch (seq[1])
					{
					case 'A':
						s->cursor_y--;
						break;
					case 'B':
						s->cursor_y++;
						break;
					case 'C':
						s->cursor_x++;
						break;
					case 'D':
						s->cursor_x--;
						break;
					}
			}
			break;
		default:
			{
				memcpy (&s->buffer[s->cursor_y][s->cursor_x - 1], &input,
						sizeof (input));
				s->cursor_x++;
			}
		}
}

// void update(struct state *s) {}

void
display (struct state *s)
{
	write (STDIN_FILENO, "\x1b[2J", 4);
	write (STDIN_FILENO, "\x1b[H", 3);
	for (int i = 0; i < s->rows; i++)
		{
			write (STDOUT_FILENO, s->buffer[i], s->columns);
			write (STDOUT_FILENO, "\r\n", 2);
		}
	char buffer[32];
	sprintf (buffer, "\x1b[%d;%dH", s->cursor_y, s->cursor_x);
	write (STDIN_FILENO, buffer, strlen (buffer));
	get_cursor_position (&s->cursor_y, &s->cursor_x);
}

void
die (struct termios *original_terminal_state)
{
	tcsetattr (STDIN_FILENO, TCSAFLUSH, original_terminal_state);
	write (STDIN_FILENO, "\x1b[2J", 4);
}

int
main (int argc, char *argv[])
{
	struct state state = { 1, 0, 0, 0, 0, NULL, 0, "" };
	struct termios original_terminal_state;
	struct termios terminal_state;

	// TODO: clear this shit out on refactoring.
	if (argc > 1)
		{
			sprintf (state.filename, "%s", argv[1]);
		}

	init (&state, &original_terminal_state, &terminal_state);
	while (state.running)
		{
			input (&state);
			// update(&state);
			display (&state);
		}
	die (&original_terminal_state);
}
