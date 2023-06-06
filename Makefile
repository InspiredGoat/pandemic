SOURCES = main.c graph.c slider.c
SRC = $(addprefix src/, $(SOURCES))
OBJ = $(addsuffix .o, $(addprefix bin/, $(basename $(notdir $(SRC)))));
INCLUDE = -I include -I deps/include
DEPS = -lm -lraylib
CFLAGS = -W -O2 #-D_DEBUG_ # -pg

all: simulator

again: clean simulator
windows: simulator.exe

simulator: $(OBJ)
	$(CC) -W $^ $(INCLUDE) $(DEPS) -o $@

simulator.exe: $(SRC)
	$(CC) $^ $(INCLUDE) -L deps -l:libraylib.a -mwindows -lwinmm -o $@

bin/%.o : src/%.c
	$(CC) $(INCLUDE) $(DEPS) $(CFLAGS) -c $< -o $@

clean:
	rm bin/*.o simulator

try: simulator
	./simulator

run:
	./simulator
