all:
	gcc src/*.c -lreadline -lmath -o /usr/bin/parus

clean:
	rm /usr/bin/parus
