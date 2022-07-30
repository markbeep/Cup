LIBSPATH= -L../external/Disco-C -L/usr/lib -L/usr/local/lib
LIBS 	= -ldisco -lcurl -lpthread -lwebsockets
CC 		= gcc
CFLAGS	= -std=gnu11 -g -pedantic -Wall -Wno-conversion \
    -Wextra -Wno-missing-field-initializers -Wformat=2 \
    -Wswitch-default -Wswitch -Wcast-align -Wpointer-arith \
    -Wno-bad-function-cast -Wstrict-prototypes -Winline \
    -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code \
    -Wlogical-op -Wno-float-equal -Wstrict-aliasing=3 -Wredundant-decls \
    -Wold-style-definition -Werror \
    -ggdb3 \
    -O0 \
    -fno-omit-frame-pointer -ffloat-store -fno-common -fstrict-aliasing \
    -lm
INCLUDE = -I../external/Disco-C/include -I../external/Disco-C/external -I/usr/local/include -I/usr/include

main: main.c
	$(CC) $(INCLUDE) $(CFLAGS) main.c -o $@ $(LIBSPATH) $(LIBS)
