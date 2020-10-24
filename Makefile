CC=gcc
USE_READLINE=1

all:
ifeq ($(USE_READLINE), 0)
	$(CC) src/*.c -o /usr/bin/parus -lm
else
	$(CC) src/*.c -o /usr/bin/parus -lm -lreadline -D USE_READLINE
endif

clean:
	rm /usr/bin/parus
