all:
	gcc src/*.c -lreadline -o /usr/bin/parus

clean:
	rm /usr/bin/parus
