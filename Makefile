CC=gcc
USE_READLINE=1

all:
ifeq ($(USE_READLINE), 0)
	cc src/*.c -o /usr/bin/parus -lm
else
	cc src/*.c -o /usr/bin/parus -lm -lreadline -D USE_READLINE
endif

clean:
	rm /usr/bin/parus
