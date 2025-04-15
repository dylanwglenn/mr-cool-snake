# Mr. Cool Snake
A simple snake/nibbles clone loosely based off my memories playing [Nibbles 68k by John David Ratcliffe for the TI-89 graphing calulator](https://technoplaza.net/downloads/details.php?program=1).

##Controls
- Arrows or WASD for movement
- Spacebar for starting/pausing
- Brackets for changing difficulty

## Compiling
### Dependencies
- SDL2
- SDL2_ttf
- [SDL_FontCache](https://github.com/grimfang4/SDL_FontCache) (included in this repo)

### Using GCC
```
gcc -Wall -o snake snake.c SDL_FontCache-master/SDL_FontCache.o -lm -lSDL2 -lSDL2_ttf`
```


### Using Emscripten
```
emcc snake.c SDL_FontCache-master/SDL_FontCache_emcc.o -o snake.html -s USE_SDL=2 -s USE_SDL_TTF=2 -s WASM=1 --preload-file "/usr/share/fonts/truetype/freefont/FreeMono.ttf"@FreeMono.ttf -O2
```

Change the path to the FreeMono.ttf file if necessary.
