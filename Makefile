all:
	gcc src/*.c -lreadline -lm -o /usr/bin/parus

clean:
	rm /usr/bin/parus
