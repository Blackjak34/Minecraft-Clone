LIN_CC = gcc
WIN_CC = x86_64-pc-cygwin-gcc
OSX_CC = 
LIN_LIBS = -lGL -lGLU -lGLEW -lglfw -lpng -lfreetype -lm -lopenal -lvorbisfile -lcore
WIN_LIBS = -lopengl32 -lglu32 -lglew32 -lglfw3dll -lpng16 -lfreetype.dll -lm -lopenal -lvorbisfile -lcore
OSX_LIBS = 
LIN_BIN = game
WIN_BIN = game.exe
OSX_BIN =
ALL_FLAGS = -g -Wall -mtune=generic -Og
LIN_FLAGS = 
WIN_FLAGS = -std=c11 -DGLFW_DLL
OSX_FLAGS = 

SRC_FILES := $(wildcard src/*.c)
LIN_BUILD := $(patsubst src/%.c, build/linux/%.o, $(SRC_FILES))
WIN_BUILD := $(patsubst src/%.c, build/win/%.o, $(SRC_FILES))
OSX_BUILD := $(patsubst src/%.c, build/osx/%.o, $(SRC_FILES))
.PHONY: mods linux win osx clean
mods:
	make -C modules/core $(MAKECMDGOALS)
	$(foreach dir,$(shell find modules/ -maxdepth 1 -mindepth 1 ! -path modules/core),make -C $(dir) $(MAKECMDGOALS);)
linux: mods $(LIN_BUILD) dist
	$(LIN_CC) -o dist/linux/$(LIN_BIN) ${LIN_BUILD} -Llib/linux/ -Lmodules/core/dist/linux/ $(LIN_LIBS)
win: mods $(WIN_BUILD) dist
	$(WIN_CC) -o dist/win/$(WIN_BIN) $(WIN_BUILD) -Llib/win/ -Lmodules/core/dist/win/ $(WIN_LIBS)
osx: mods $(OSX_BUILD) dist
	$(OSX_CC) -o dist/osx/$(OSX_BIN) $(OSX_BUILD) -Llib/osx/ -Lmodules/core/dist/osx/ $(OSX_LIBS)
build/linux/%.o: src/%.c build
	$(LIN_CC) -c $(LIN_FLAGS) $(ALL_FLAGS) -o $@ $< -Iinclude/linux/ -Imodules/core/src/
build/win/%.o: src/%.c build
	$(WIN_CC) -c $(WIN_FLAGS) $(ALL_FLAGS) -o $@ $< -Iinclude/win/ -Imodules/core/src/ -I/usr/include/freetype2/
build/osx/%.o: src/%.c build
	$(OSX_CC) -c $(OSX_FLAGS) $(ALL_FLAGS) -o $@ $< -Iinclude/osx/ -Imodules/core/src/
build:
	mkdir -p build/linux/
	mkdir -p build/win/
	mkdir -p build/osx/
dist:
	mkdir -p dist/linux/
	cp -r res/ dist/linux/res/
	mkdir -p dist/win/
	cp -r res/ dist/win/res/
	mkdir -p dist/osx/
	cp -r res/ dist/osx/res/
clean: mods
	-rm -rf build/
	-rm -rf dist/
