# Put this makefile inside project folder

G++ = g++
SRC = $(wildcard src/*.cpp src/include/*.cpp)
CFLAG = -Wno-missing-field-initializers -Wall -std=c++23 -o # Using flag -Wno-missing-field-initializers if use header <raymath.h> in C++
LIBPATH = -I"C:/raylib/raylib/build/raylib/include" -L"C:/raylib/raylib/build/raylib" # Set your environtment library path raylib here
# RAYFLAGS = $(LIBPATH) -lraylib -lm -ldl -lpthread -lGL # For Linux/MacOS
RAYFLAGS = $(LIBPATH) -lraylib -lopengl32 -lm -lgdi32 -lwinmm # Default build for windows
# Note: Using flag -mwindows for hide cmd For Windows
DIR_EXE = bin
NAME = $(DIR_EXE)/main

all:
	@echo "[G++] Build c++ with raylib."
	@$(G++) $(SRC) $(RAYFLAGS) $(CFLAG) $(NAME)
	@echo "[G++] Complete build c++ with raylib."
	@echo "[OS] Running game/app."
	@./$(NAME)
	@echo "[OS] Success running game/app."

help:
	@echo "Example build: \"make\" or \"make <flag>\""
	@echo "All flag:"
	@echo "help, debug, run, clean"

debug:
	@echo "[OS] Command Running:"
	$(G++) $(SRC) $(RAYFLAGS) $(CFLAG) $(NAME)
	./$(NAME)
	@echo "[APP] Debug app view terminal:"

run:
	@echo "[OS] Running game/app."
	@./$(NAME)
	@echo "[OS] Success running game/app."

clean:
	@echo "[OS] Delete game/app."
	@rm $(NAME).exe