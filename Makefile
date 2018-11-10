


CC = gcc
CFLAGS = -Wall


DAEMON_EXEC = daemon
DAEMON_SOURCES = dmain.c db.c controls.c
DAEMON_OBJS = $(patsubst %.c, %.o, $(DAEMON_SOURCES))

CLI_EXEC = cli
CLI_SOURCES = cmain.c db.c controls.c
CLI_OBJS = $(patsubst %.c, %.o, $(CLI_SOURCES))

ALL: $(DAEMON_EXEC) $(CLI_EXEC)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(CLI_EXEC): .clidepend $(CLI_OBJS)
	$(CC) -g -o $(CLI_EXEC) $(CLI_SOURCES)

$(DAEMON_EXEC): .daemondepend $(DAEMON_OBJS)
	$(CC) -g -o $(DAEMON_EXEC) $(DAEMON_SOURCES)

.PHONY: clean

clean:
	rm -f $(DAEMON_EXEC)
	rm -f $(CLI_EXEC)
	rm -f *.o


.daemondepend: $(DAEMON_SOURCES)
	rm -f ./.daemondepend
	$(CXX) $(CPPFLAGS) -MM $^ | sed 's/[^ ]*\.o */$(BUILD_DIR)\/&/'>./.daemondepend;
.clidepend: $(CLI_SOURCES)
	rm -f ./.clidepend
	$(CXX) $(CPPFLAGS) -MM $^ | sed 's/[^ ]*\.o */$(BUILD_DIR)\/&/'>./.clidepend;

include .daemondepend
include .clidepend
