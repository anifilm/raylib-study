CC = gcc
RAYLIB_PATH = /opt/homebrew/Cellar/raylib/5.5
INCLUDE_PATH = -I$(RAYLIB_PATH)/include
LIB_PATH = -L$(RAYLIB_PATH)/lib
LIBS = -lraylib -lm -lpthread

SRC = src/main.c src/maze.c src/player.c src/game.c src/draw.c
OBJ = $(SRC:.c=.o)
TARGET = maze_runner

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ) $(LIB_PATH) $(LIBS)

%.o: %.c
	$(CC) $(INCLUDE_PATH) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
