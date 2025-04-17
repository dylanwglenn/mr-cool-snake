/*
 * snake.c
 *
 * A simple snake game in sdl2
 *
 * Compile with:
 * gcc snake_game.c -o snake_game -lSDL2
 */

// #include <SDL2/SDL.h>
#include "SDL_FontCache-master/SDL_FontCache.h" // SDL2 is included with this
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define TILE_SIZE 30
#define AREA_WIDTH 30
#define AREA_HEIGHT 25
#define FRUIT_COLOR 255,0,0,255
#define WALL_COLOR 60,60,60,255
#define STARTING_LENGTH 4 // note: the tail size includes the head
#define FRUIT_TO_ADVANCE 10
#define FRUIT_TAIL_GROWTH 4
#define MAX_LENGTH FRUIT_TO_ADVANCE*FRUIT_TAIL_GROWTH + STARTING_LENGTH
#define STARTING_LIVES 5

typedef struct{
	int r,g,b,a;
} Color;

typedef struct{
  int x, y;
} Position;

typedef enum{
	UP,
	DOWN,
	LEFT,
	RIGHT
} Direction;

typedef struct{
	Direction direction;
	int length;
	Position* tailArray;
	int fruit;
	bool moving;
	Position pos;
} Snake;

typedef struct{
	Position fruitPos;
	Snake snake;
	int levelNum;
	int lives;
	int difficulty;
} Game;

typedef enum{
	EMPTY,
	WALL,
	FRUIT,
	SNAKE_HEAD,
	SNAKE_TAIL
} CellType;

typedef struct{
	SDL_Window* window;
	SDL_Renderer* renderer;
	FC_Font* font;
	Game game;
} Context;

// funtion prototypes
void main_loop(void*);
Game* update_game_state(Game*);
Position find_fruit_pos(Game*);
SDL_Window* init_sdl_window();
SDL_Renderer* init_sdl_renderer(SDL_Window*);
void draw_graphics(Game*, SDL_Renderer*, CellType*, FC_Font*);
CellType* get_graphics_matrix(Game*);
Position find_fruit_pos(Game*);
Position find_starting_pos(Game*);
Game set_init_game_state();
Position* reset_tail(Game*);
int get_game_speed(int);
char* get_level(int);

// Colors:
static Color bgColor = {0,0,0,255};
static Color wallColor = {70,70,70,255};
static Color fruitColor = {255,0,0,255};
static Color snakeHeadColor = {0,240,0,255};
static Color snakeTailColor = {0,190,0,255};

// set these as global for emscripten reasons
static bool quit = false;
static SDL_Event event;
bool actionOnFrame;
int timeSnapshot = 0;

int main(int argc, char *argv[]){
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Init(SDL_INIT_EVENTS);
	
	Context context;
	context.window = init_sdl_window();
	context.renderer = init_sdl_renderer(context.window);
	context.font = FC_CreateFont();
	context.game = set_init_game_state();
	
	#ifdef __EMSCRIPTEN__
		FC_LoadFont(context.font, context.renderer,"FreeMono.ttf", 20, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);
	#else 
		FC_LoadFont(context.font, context.renderer,"/usr/share/fonts/truetype/freefont/FreeMono.ttf", 20, FC_MakeColor(255,255,255,255), TTF_STYLE_NORMAL);
	#endif
	
	// draw initial state
	CellType* graphicsMatrix = get_graphics_matrix(&context.game);
	draw_graphics(&context.game, context.renderer, graphicsMatrix, context.font);
	
	#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop, &context, 120, true);
	#else
		while(!quit){ main_loop(&context); }
	#endif
	
	SDL_DestroyRenderer(context.renderer);
	SDL_DestroyWindow(context.window);
	SDL_Quit();
	
	return(0);
}

void main_loop(void* arg) {
	Context* context = arg;
	
	int gameSpeed = get_game_speed(context->game.difficulty);
	bool frameAdvance = SDL_GetTicks() - timeSnapshot >= gameSpeed; // set game speed (lower is faster)
	if(frameAdvance){	
		//update game state
		context->game = *update_game_state(&context->game);
		// get game state to graphics matrix
		CellType* graphicsMatrix = get_graphics_matrix(&context->game);
		// draw the game state
		// draw background
		SDL_SetRenderDrawColor(context->renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		SDL_RenderClear(context->renderer);
		// draw graphics matrix
		draw_graphics(&context->game, context->renderer, graphicsMatrix, context->font);
		// reset `actionOnFrame` to allow new inputs
		actionOnFrame = false;
		// update timer
		timeSnapshot = SDL_GetTicks();
	}
	
	// Watch for keyboard events
	while(SDL_PollEvent(&event)){
		switch(event.type){
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
					case SDLK_UP:
					case SDLK_w:
						if(context->game.snake.direction != DOWN && context->game.snake.moving && !actionOnFrame){context->game.snake.direction = UP;}
						actionOnFrame = true;
						break;
					case SDLK_DOWN:
					case SDLK_s:
						if(context->game.snake.direction != UP && context->game.snake.moving && !actionOnFrame){context->game.snake.direction = DOWN;}
						actionOnFrame = true;
						break;
					case SDLK_LEFT:
					case SDLK_a:
						if(context->game.snake.direction != RIGHT && context->game.snake.moving && !actionOnFrame){context->game.snake.direction = LEFT;}
						actionOnFrame = true;
						break;
					case SDLK_RIGHT:
					case SDLK_d:
						if(context->game.snake.direction != LEFT && context->game.snake.moving && !actionOnFrame){context->game.snake.direction = RIGHT;}
						actionOnFrame = true;
						break;
						
					case SDLK_1:
						context->game.difficulty = 1;
						break;
					case SDLK_2:
						context->game.difficulty = 2;
						break;
					case SDLK_3:
						context->game.difficulty = 3;
						break;
					case SDLK_4:
						context->game.difficulty = 4;
						break;
					case SDLK_5:
						context->game.difficulty = 5;
						break;
					case SDLK_SPACE:
						context->game.snake.moving = !context->game.snake.moving;
						break;
					#ifdef __GNCC__
					case SDLK_ESCAPE:
						quit = true;
						break;
					#endif
				}
				break;
		}
	}
	SDL_Delay(10); // prevent hogging cpu resources
	
		
	#ifdef __EMSCRIPTEN__
		if(quit){
			emscripten_cancel_main_loop();  /* this should "kill" the app. */
		}
	#endif
}

Game* update_game_state(Game* game){
	char* levelDat = get_level(game->levelNum);
	
	// update tail position
	if(game->snake.moving){
		game->snake.tailArray[0] = game->snake.pos;
		// shift tailArray values from back to front
		for(int i=game->snake.length; i>0; i--){
			game->snake.tailArray[i] = game->snake.tailArray[i-1];
		}
	}
	
	// update head position
	if(game->snake.moving){
		switch(game->snake.direction){
			case UP:
				game->snake.pos.x = game->snake.pos.x;
				game->snake.pos.y= game->snake.pos.y - 1;
				break;
			case DOWN:
				game->snake.pos.x = game->snake.pos.x;
				game->snake.pos.y= game->snake.pos.y + 1;
				break;
			case LEFT:
				game->snake.pos.x = game->snake.pos.x - 1;
				game->snake.pos.y= game->snake.pos.y;
				break;
			case RIGHT:
				game->snake.pos.x = game->snake.pos.x + 1;
				game->snake.pos.y = game->snake.pos.y;
				break;
		}
	}
		
	// check for collision with tail or wall
	bool collideWithTail = false;
	for(int i=3; i<game->snake.length; i++){ // start at 3 because you can't collide with tail before that
		if(game->snake.pos.x == game->snake.tailArray[i].x && game->snake.pos.y == game->snake.tailArray[i].y){
			collideWithTail = true;
		}
	}
	
	if(levelDat[game->snake.pos.x + AREA_WIDTH*game->snake.pos.y] == '#' || collideWithTail){
		game->lives--;
		game->snake.pos = find_starting_pos(game);
		game->snake.length = STARTING_LENGTH;
		game->snake.direction = RIGHT;
		game->snake.tailArray = reset_tail(game);
		game->fruitPos = find_fruit_pos(game);
		game->snake.moving = false;
		game->snake.fruit = 0;
	}
	
	// check for fruit collision
	if(game->snake.pos.x == game->fruitPos.x && game->snake.pos.y == game->fruitPos.y){
		game->snake.fruit++;
		game->snake.length = game->snake.length + FRUIT_TAIL_GROWTH;
		game->fruitPos = find_fruit_pos(game);
	}
	
	// check for level advance
	if(game->snake.fruit >= FRUIT_TO_ADVANCE){
		game->lives++;
		game->levelNum++;
		game->snake.moving = false;
		game->snake.direction = RIGHT;
		game->snake.fruit = 0;
		game->snake.length = STARTING_LENGTH;
		game->snake.pos = find_starting_pos(game);
		game->snake.tailArray = reset_tail(game);
		game->fruitPos = find_fruit_pos(game);
	}
	
	// check for game over
	if(game->lives == 0){
		game->levelNum = 0;
		game->lives = STARTING_LIVES;
	}
	
	return(game);
}

void draw_graphics(Game* game, SDL_Renderer* renderer, CellType* graphics_matrix, FC_Font* font){
	SDL_Rect rect;
	Color tileColor;
	rect.w = TILE_SIZE;
	rect.h = TILE_SIZE; 
	for(int i=0; i<AREA_WIDTH*AREA_HEIGHT; i++){
		switch(graphics_matrix[i]){
			case WALL:
				tileColor = wallColor;
				break;
			case FRUIT:
				tileColor = fruitColor;
				break;
			case SNAKE_HEAD:
				tileColor = snakeHeadColor;
				break;
			case SNAKE_TAIL:
				tileColor = snakeTailColor;
				break;
			default:
				continue;
		}
		rect.x = (i%AREA_WIDTH) * TILE_SIZE;
		rect.y = floor(i/(float)AREA_WIDTH) * TILE_SIZE;
		SDL_SetRenderDrawColor(renderer,tileColor.r,tileColor.g,tileColor.b,tileColor.a);
		SDL_RenderFillRect(renderer, &rect);
	}
	
	// Draw font
	FC_Draw(font,renderer,30,5,"Lives");
	FC_Draw(font,renderer, AREA_WIDTH*TILE_SIZE-200,5,"Difficulty: %d", game->difficulty);
	
	// draw life icons
	rect.w = TILE_SIZE / 3;
	rect.h = TILE_SIZE / 3;
	rect.y = TILE_SIZE / 3;
	for(int i=0; i<game->lives; i++){
		rect.x = 110 + i*20;
		tileColor = snakeHeadColor;
		SDL_SetRenderDrawColor(renderer,tileColor.r,tileColor.g,tileColor.b,tileColor.a);
		SDL_RenderFillRect(renderer, &rect);
	}
	
	SDL_RenderPresent(renderer);
}

CellType* get_graphics_matrix(Game* game){
	CellType* graphicsMatrix = calloc(AREA_WIDTH*AREA_HEIGHT+1, sizeof(int));
	char* levelDat = get_level(game->levelNum);
	
	for(int r=0; r<AREA_HEIGHT; r++){
		for(int c=0; c<AREA_WIDTH; c++){
			
			// check for tail
			for(int i=0; i<game->snake.length; i++){
				if(game->snake.tailArray[i].x == c && game->snake.tailArray[i].y == r){
					graphicsMatrix[c+(AREA_WIDTH*r)] = SNAKE_TAIL;
				}
			}
			
			// check for level walls
			if(levelDat[c+(AREA_WIDTH*r)] == '#'){
				graphicsMatrix[c+(AREA_WIDTH*r)] = WALL;
			}
			// check for snake head
			else if(game->snake.pos.x == c && game->snake.pos.y == r){
				graphicsMatrix[c+(AREA_WIDTH*r)] = SNAKE_HEAD;
			}
			// check for fruit
			else if(game->fruitPos.x == c && game->fruitPos.y == r){
				graphicsMatrix[c+(AREA_WIDTH*r)] = FRUIT;
			}

		}
	}
	
	return(graphicsMatrix);
	free(levelDat);
	free(graphicsMatrix);
}


Game set_init_game_state(){
	Game game;
	game.levelNum = 0;
	game.lives = STARTING_LIVES;
	game.snake.direction = RIGHT;
	game.snake.length = STARTING_LENGTH;
	game.snake.fruit = 0;
	game.snake.moving = false;
	game.snake.pos = find_starting_pos(&game);
	game.snake.tailArray = reset_tail(&game);
	game.fruitPos = find_fruit_pos(&game);
	game.difficulty = 3;
	
	return(game);
}

Position find_fruit_pos(Game* game){
	Position* validPosArray = calloc(AREA_HEIGHT*AREA_WIDTH, sizeof(Position));
	CellType* graphicsMatrix = get_graphics_matrix(game);
	
	int validPosNum=0;
	for(int i=0; i<AREA_WIDTH*AREA_HEIGHT; i++){
		if(graphicsMatrix[i] == EMPTY){
			validPosArray[validPosNum].x = i % AREA_WIDTH;
			validPosArray[validPosNum].y = floor(i/(float)AREA_WIDTH);
			validPosNum++;
		}
	}
	
	Position fruitPos = validPosArray[rand() % validPosNum];
		
	free(validPosArray);
	free(graphicsMatrix);

	return(fruitPos);
}

Position* reset_tail(Game* game){
	// note: the tail extends to inlcude the head position as well
	Position* tailArray = calloc(MAX_LENGTH, sizeof(Position));
	for(int i=0; i<=STARTING_LENGTH; i++){
		tailArray[i].x = game->snake.pos.x - i;
		tailArray[i].y = game->snake.pos.y;
	}
	return(tailArray);
	free(tailArray);
}

Position find_starting_pos(Game* game) {
  Position pos;
  char* levelDat = get_level(game->levelNum);
  pos.x = floor(AREA_WIDTH / 2.0);
  pos.y = floor(AREA_HEIGHT / 2.0);
  while(levelDat[pos.x+AREA_WIDTH*pos.y] == '#'){
		pos.y++;
	}

  return (pos);
}

int get_game_speed(int difficulty){
	switch(difficulty){
		case 1:
			return(140);
			break;
		case 2:
			return(100);
			break;
		case 3:
			return(60);
			break;
		case 4:
			return(45);
			break;
		case 5:
			return(38);
			break;
		default:
			return(50);
			break;
	}
}

SDL_Window* init_sdl_window(){
	// create a window object
	// https://wiki.libsdl.org/SDL2/SDL_CreateWindow
	SDL_Window* window = SDL_CreateWindow("Mr. Cool Snake",
											 SDL_WINDOWPOS_UNDEFINED, // win x pos
											 SDL_WINDOWPOS_UNDEFINED, // win y pos
											 AREA_WIDTH * TILE_SIZE, // win width
											 AREA_HEIGHT * TILE_SIZE, // win height
											 SDL_WINDOW_SHOWN);
	return(window);
}

SDL_Renderer* init_sdl_renderer(SDL_Window* window){
	// create renderer object
	// https://wiki.libsdl.org/SDL2/SDL_CreateRenderer		 
	SDL_Renderer* renderer = SDL_CreateRenderer(window,
																							-1, // use the first available renderer
																							SDL_RENDERER_ACCELERATED);
	
	return(renderer);
}

char* get_level(int levelNum){
	char* levelDat = calloc(AREA_WIDTH*AREA_HEIGHT+1, sizeof(char)); // levelDat will be the same size every level
	switch(levelNum){
		case 0:
			strcpy(levelDat,
			"##############################"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"##############################");
			break;
		case 1:
			strcpy(levelDat,
			"##############################"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#          ########          #"
			"#          ########          #"
			"#          ########          #"
			"#          ########          #"
			"#          ########          #"
			"#          ########          #"
			"#          ########          #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#                            #"
			"##############################");
			break;
		case 2:
			strcpy(levelDat,
			"##############################"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#                            #"
			"#                            #"
			"#                            #"
			"##########          ##########"
			"#                            #"
			"#                            #"
			"#                            #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"#              #             #"
			"##############################");
			break;
		case 3:
			strcpy(levelDat,
			"##############################"
			"#####                    #####"
			"#####                    #####"
			"#####                    #####"
			"#####                    #####"
			"#                            #"
			"#                            #"
			"#        ###########         #"
			"#        #         #         #"
			"#        #         #         #"
			"#        #         #         #"
			"#                            #"
			"#                            #"
			"#                            #"
			"#        #         #         #"
			"#        #         #         #"
			"#        #         #         #"
			"#        ###########         #"
			"#                            #"
			"#                            #"
			"#####                    #####"
			"#####                    #####"
			"#####                    #####"
			"#####                    #####"
			"##############################");
			break;
		case 4:
			strcpy(levelDat,
			"##############################"
			"#                            #"
			"#                            #"
			"#   #  #  #  #  #  #  #  #   #"
			"#   #  #  #  #  #  #  #  #   #"
			"#   #  #  #  #  #  #  #  #   #"
			"#   #  #  #  #  #  #  #  #   #"
			"#                            #"
			"#                            #"
			"#   ######################   #"
			"#                            #"
			"#                            #"
			"#   ######################   #"
			"#                            #"
			"#                            #"
			"#   ######################   #"
			"#                            #"
			"#                            #"
			"#   #  #  #  #  #  #  #  #   #"
			"#   #  #  #  #  #  #  #  #   #"
			"#   #  #  #  #  #  #  #  #   #"
			"#   #  #  #  #  #  #  #  #   #"
			"#                            #"
			"#                            #"
			"##############################");
			break;
		default:
			free(levelDat);
			return(NULL);
	}
	return(levelDat);
	free(levelDat);
}