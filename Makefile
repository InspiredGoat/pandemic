SOURCES = main.c graph.c slider.c
SRC = $(addprefix src/, $(SOURCES))
OBJ = $(addsuffix .o, $(addprefix bin/, $(basename $(notdir $(SRC)))));
INCLUDE = -I include
CFLAGS = -W -O2 #-D_DEBUG_ # -pg
DEPS = -lm -lraylib#-lGL -lglfw -lX11 -lpthread -lXrandr -lXi -ldl

all: simulator

again: clean simulator

simulator: $(OBJ)
	gcc -W $^ $(DEPS) -o $@

bin/%.o : src/%.c
	gcc $(INCLUDE) $(CFLAGS) -c $< -o $@


clean:
	rm bin/*.o simulator

install:
	echo "Can't install surry"

try: simulator
	./simulator

run:
	./simulator
