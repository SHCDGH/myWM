mywm: main.c
	$(CC) -Wall -O2 -o mywm main.c -lX11

clean:
	rm -f mywm
