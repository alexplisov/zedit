zedit: zedit.c
	rm -rf build/
	mkdir build
	$(CC) zedit.c -o build/zedit -Wall -Wextra -pedantic -std=c99

debug: zedit.c
	rm -rf debug/
	mkdir debug
	$(CC) -g zedit.c -o debug/zedit -Wall -Wextra -pedantic -std=c99
