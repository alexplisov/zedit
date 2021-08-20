zedit: zedit.c
	$(CC) zedit.c -o zedit -Wall -Wextra -pedantic -std=c99

debug: zedit.c
	$(CC) -g zedit.c -o zedit -Wall -Wextra -pedantic -std=c99
