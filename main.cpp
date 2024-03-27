// Spy Hunter. Wykonal:
// Jakub Bronowski, 193208
// Inforatyka, I sem, gr 2; 14012023_213700

#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<cstdlib>
#include<cstdio>
#include<ctime>
#include<stdlib.h>
#include<float.h> // DBL_MAX

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH	800
#define SCREEN_HEIGHT	600
#define MAX_SAVES 3000
const int MARGIN = 5;
const int MOVE_CAR_X = 10;
const int ROADSIDE_OBJECTS_MAXCOUNT = 18;
const int ENEMY_CARS_MAXCOUNT = 2;
const int NEUTRAL_CARS_MAXCOUNT = 2;
const double DEFAULT_CAR_SPEED = 60.0;
const int ROAD_BORDER_MIN = 200;
const int ROAD_BORDER_MAX = 500;
const double ROAD_RESIZE_SPEED = 2.0;
const double ROAD_RESIZE_START_AT = 10.0;
const char FILEERROR_NO_ERRORS = 'N';
const char FILEERROR_LOAD_ERROR = 'L';
const char FILEERROR_SAVE_ERROR = 'S';
const int POINTS_SHOT = 200;
const double POWERUP_TIMETOLIVE = 7.0;
const double DEFAULT_SHOT_RANGE = 150.0;
const double POWERUP_GENERATE_TIME_CONDITION = 10.0;
const double POWERUP_ACTIVE_SHOT_RANGE = DEFAULT_SHOT_RANGE * 2;

typedef struct SDL_vars_T {
	SDL_Event event;
	SDL_Surface* screen, * charset;
	SDL_Surface* roadsiteObj[3];
	SDL_Surface* car, * enemy_car, * neutral_car, * collision, *powerup;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
};

typedef struct data_T {
	double worldTime;
	double fps;
	long long int points;
	int collision;
	int collisionPlaceX;
	double collisionPlaceY;
	bool canShoot;
	bool powerupExists = false;
	int powerupX;
	double powerupY;
	double powerupCreationTime = -1.0;
	double powerupWhenCaught = -1.0;
};

typedef struct car_T {
	bool isEmpty = true;
	bool dontDisplay = false;
	int carType;
	int currPosX; // current position ON SCREEN in px
	double currPosY; // current position ON SCREEN in px
	int width; // car width
	int height; // car height
	double speed;
};

typedef struct road_T {
	int borderLeft;
	int borderRight;
};

typedef struct roadsite_obj_T {
	bool isEmpty = true;
	int currPosX = -1;
	double currPosY = -1.0;
};

typedef struct pair_T {
	int x;
	double y;
};

enum colors_name {
	black = 0,
	blue = 1,
	green = 2,
	red = 3,
	white = 4
};

enum commands {
	start = 1000,
	head,
	implemented,
	neutral,
	enemy,
	player,
	collision_none,
	collision_offroad,
	collision_neutral,
	collision_enemy,
};

enum exit_codes {
	EXIT_PROGRAM = -1,
	EXIT_NEWGAME = 1,
	EXIT_BY_USER = 777,
	GAME_PAUSE = 2,
	GAME_SAVE = 3,
	GAME_LOAD = 4,
	GAME_END = 5,
	SHOT = 6
};

long long min(long long a, long long b);
long long max(long long a, long long b);

///////////////////////////////////////////////////////////////////////////////////////////
//					F U N C T I O N S	 F R O M	 T E M P L A T E
///////////////////////////////////////////////////////////////////////////////////////////
// narysowanie napisu txt na powierzchni screen, zaczynaj¹c od punktu (x, y)
// charset to bitmapa 128x128 zawieraj¹ca znaki
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	}
}

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt œrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
}

// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
}

// rysowanie linii o d³ugoœci l w pionie (gdy dx = 0, dy = 1) b¹dŸ poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	}
}

// rysowanie prostok¹ta o d³ugoœci boków l i k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
}

void BMPLoad_errorHandler(char* filename, SDL_vars_T* SDL_vars)
{
	printf("SDL_LoadBMP(%s) error: %s\n", filename, SDL_GetError());
	SDL_FreeSurface(SDL_vars->screen);
	SDL_DestroyTexture(SDL_vars->scrtex);
	SDL_DestroyWindow(SDL_vars->window);
	SDL_DestroyRenderer(SDL_vars->renderer);
	SDL_Quit();
}

int loadBMPs(SDL_vars_T* SDL_vars)
{
	SDL_vars->charset = SDL_LoadBMP("./data_images/cs8x8.bmp");
	if (SDL_vars->charset == NULL) {
		BMPLoad_errorHandler("./data_images/cs8x8.bmp", SDL_vars);
		return 1;
	}
	else
	{
		SDL_SetColorKey(SDL_vars->charset, true, 0x000000);
		printf("loadBMPs(): cs8x8.bmp loaded\n");
	}

	SDL_vars->roadsiteObj[0] = SDL_LoadBMP("./data_images/roadsite_0.bmp");
	if (SDL_vars->roadsiteObj[0] == NULL) {
		BMPLoad_errorHandler("./data_images/roadsite_0.bmp", SDL_vars);
		return 1;
	}
	printf("loadBMPs(): roadsite_0.bmp loaded\n");

	SDL_vars->roadsiteObj[1] = SDL_LoadBMP("./data_images/roadsite_1.bmp");
	if (SDL_vars->roadsiteObj[1] == NULL) {
		BMPLoad_errorHandler("./data_images/roadsite_1.bmp", SDL_vars);
		return 1;
	}
	printf("loadBMPs(): roadsite_1.bmp loaded\n");

	SDL_vars->roadsiteObj[2] = SDL_LoadBMP("./data_images/roadsite_2.bmp");
	if (SDL_vars->roadsiteObj[2] == NULL) {
		BMPLoad_errorHandler("./data_images/roadsite_2.bmp", SDL_vars);
		return 1;
	}
	printf("loadBMPs(): roadsite_2.bmp loaded\n");

	SDL_vars->car = SDL_LoadBMP("./data_images/car.bmp");
	if (SDL_vars->car == NULL) {
		BMPLoad_errorHandler("./data_images/car.bmp", SDL_vars);
		return 1;
	}
	else printf("loadBMPs(): car.bmp loaded\n");

	SDL_vars->enemy_car = SDL_LoadBMP("./data_images/enemy_car.bmp");
	if (SDL_vars->enemy_car == NULL) {
		BMPLoad_errorHandler("./data_images/enemy_car.bmp", SDL_vars);
		return 1;
	}
	printf("loadBMPs(): enemy_car.bmp loaded\n");

	SDL_vars->neutral_car = SDL_LoadBMP("./data_images/neutral_car.bmp");
	if (SDL_vars->neutral_car == NULL) {
		BMPLoad_errorHandler("./data_images/neutral_car.bmp", SDL_vars);
		return 1;
	}
	printf("loadBMPs(): neutral_car.bmp loaded\n");

	SDL_vars->collision = SDL_LoadBMP("./data_images/collision.bmp");
	if (SDL_vars->collision == NULL) {
		BMPLoad_errorHandler("./data_images/collision.bmp", SDL_vars);
		return 1;
	}
	printf("loadBMPs(): collision.bmp loaded\n");

	SDL_vars->powerup = SDL_LoadBMP("./data_images/powerup.bmp");
	if (SDL_vars->powerup == NULL) {
		BMPLoad_errorHandler("./data_images/powerup.bmp", SDL_vars);
		return 1;
	}
	printf("loadBMPs(): powerup.bmp loaded\n");
}

int getWindow(SDL_vars_T* SDL_vars)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	// fullscreen? int rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, &window, &renderer);
	int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &(SDL_vars->window), &(SDL_vars->renderer));
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(SDL_vars->renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(SDL_vars->renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(SDL_vars->window, "Spy Hunter");

	SDL_vars->screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	SDL_vars->scrtex = SDL_CreateTexture(SDL_vars->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	// wy³¹czenie widocznoœci kursora myszy
	SDL_ShowCursor(SDL_DISABLE);

	printf("getWindow(): window created\n");
	printf("getWindow(): calling loadBMPs\n");
	if (loadBMPs(SDL_vars) == 1)
	{
		printf("\n\nFATAL ERROR! loadBMPs returned 1\n\n");
		return 1;
	}
}

int getColor(const int colorName, SDL_vars_T* SDL_vars)
{
	int colors[5];
	colors[black] = SDL_MapRGB(SDL_vars->screen->format, 0x00, 0x00, 0x00);
	colors[green] = SDL_MapRGB(SDL_vars->screen->format, 0x00, 0xFF, 0x00);
	colors[red] = SDL_MapRGB(SDL_vars->screen->format, 0xFF, 0x00, 0x00);
	colors[blue] = SDL_MapRGB(SDL_vars->screen->format, 0x11, 0x11, 0xCC);
	colors[white] = SDL_MapRGB(SDL_vars->screen->format, 0xFE, 0xFE, 0xFE);

	return colors[colorName];
}

////////////////////////////////////////////////////////////////////////////////////
//				D R A W I N G    F U N C T I O N S
////////////////////////////////////////////////////////////////////////////////////

void printAuthorControls(const int command, SDL_vars_T* SDL_vars, const data_T* data)
{
	char text[1000];
	switch (command)
	{
	case start:
		printAuthorControls(head, SDL_vars, data);
		printAuthorControls(implemented, SDL_vars, data);
		break;

	case head:
		DrawRectangle(SDL_vars->screen, 4, 4, SCREEN_WIDTH - 8, 48, getColor(red, SDL_vars), getColor(blue, SDL_vars));
		sprintf(text, "Jakub Bronowski, 193208");
		DrawString(SDL_vars->screen, SDL_vars->screen->w / 2 - strlen(text) * 8 / 2, 8, text, SDL_vars->charset);

		sprintf(text, "Game time %.1lf, FPS counter: %.0lf / s", data->worldTime, data->fps);
		DrawString(SDL_vars->screen, SDL_vars->screen->w / 2 - strlen(text) * 8 / 2, 24, text, SDL_vars->charset);

		sprintf(text, "Collected points: %lld", data->points);
		DrawString(SDL_vars->screen, SDL_vars->screen->w / 2 - strlen(text) * 8 / 2, 38, text, SDL_vars->charset);
		break;

	case implemented:
		int sh = 10 * 10; // height of list
		DrawRectangle(SDL_vars->screen, SCREEN_WIDTH - 16 * 8 - MARGIN, SCREEN_HEIGHT - 10 * 10 - MARGIN, 16 * 8, 10 * 10, getColor(black, SDL_vars), getColor(blue, SDL_vars));
		char list[8][15] = { "arrows: move" , "esc: exit", "n: new game", "s: save game", "l: load game", "p: pause game", "space: shoot", "f: end game" };
		for (int i = 0; i < 8; i++)
		{
			DrawString(SDL_vars->screen, SCREEN_WIDTH - 15 * 8 - MARGIN, SCREEN_HEIGHT - sh + MARGIN, list[i], SDL_vars->charset);
			sh -= 2 * MARGIN;
		}
		break;
	}
}

void drawRoad(SDL_vars_T* SDL_vars, road_T* road, const data_T* data)
{
	int width = road->borderRight - road->borderLeft;
	static double lastTimeRoadChanged = 0.0;
	const int borderLeft_max = 300;
	const int borderRight_min = 400;
	const int widthMin = borderRight_min - borderLeft_max;
	const int widthMax = ROAD_BORDER_MAX - ROAD_BORDER_MIN;
	const int change = /*int(ROAD_RESIZE_SPEED)*/ 5;
	static int decrease = -1;
	static int counter = 0;

	if (data->worldTime - lastTimeRoadChanged >= int(ROAD_RESIZE_SPEED) && data->worldTime >= ROAD_RESIZE_START_AT )
	{
		if (counter < (borderLeft_max - ROAD_BORDER_MIN) / change)
			counter++;
		else
		{
			decrease *= (-1);
			counter = 0;
		}
		if (widthMin <= width && width <= widthMax)
		{
			road->borderLeft -= (decrease * change);
			road->borderRight += (decrease * change);
			lastTimeRoadChanged = data->worldTime;
		}
	}

	DrawRectangle(SDL_vars->screen, road->borderLeft, 0, width, SCREEN_HEIGHT, getColor(black, SDL_vars), getColor(black, SDL_vars));
	DrawRectangle(SDL_vars->screen, road->borderLeft + width / 2, 75, 16, 90, getColor(white, SDL_vars), getColor(white, SDL_vars)); // white stripes
	DrawRectangle(SDL_vars->screen, road->borderLeft + width / 2, 230, 16, 90, getColor(white, SDL_vars), getColor(white, SDL_vars));
	DrawRectangle(SDL_vars->screen, road->borderLeft + width / 2, 385, 16, 90, getColor(white, SDL_vars), getColor(white, SDL_vars));
	DrawRectangle(SDL_vars->screen, road->borderLeft + width / 2, 540, 16, 60, getColor(white, SDL_vars), getColor(white, SDL_vars));
}

bool onRoad(const int x, const int y, const int picW, const road_T* road)
{
	if (road->borderLeft + picW / 2 <= x && x <= road->borderRight - picW / 2)
		return true;
	return false;
}

void drawRoadside(const SDL_vars_T* SDL_vars, const road_T* road, const double carSpeed, const double deltaTime, roadsite_obj_T* queue_roadsiteObj, int* queue_roadsiteObj_size, const int frames)
{
	int roadsiteL_width = road->borderLeft;
	int roadsiteR_width = road->borderRight - road->borderLeft + 1;
	int obj_width = SDL_vars->roadsiteObj[0]->w;
	int obj_height = SDL_vars->roadsiteObj[0]->h;
	int moveY = 2;
	if (frames >= 30)
		moveY *= -1;

	if (*queue_roadsiteObj_size < ROADSIDE_OBJECTS_MAXCOUNT)
	{
		for (int i = 0; i < ROADSIDE_OBJECTS_MAXCOUNT; i++)
		{
			if (queue_roadsiteObj[i].isEmpty == true)
			{
				queue_roadsiteObj[i].isEmpty = false;
				queue_roadsiteObj[i].currPosY = 90.0 + i % (ROADSIDE_OBJECTS_MAXCOUNT / 3) * (obj_height + 7 * MARGIN);
				if (i < 6)
					queue_roadsiteObj[i].currPosX = road->borderLeft - 5 * MARGIN - rand() % 50;
				else if (i >= 6 && i < 12)
					queue_roadsiteObj[i].currPosX = road->borderRight + 5 * MARGIN + rand() % 50;
				else
					queue_roadsiteObj[i].currPosX = road->borderRight + 10 * MARGIN + obj_width + rand() % 50;

				*queue_roadsiteObj_size++;
			}
		}
	}
	for (int i = 0; i < ROADSIDE_OBJECTS_MAXCOUNT; i++)
	{
		int index = 0;
		if (i >= 6 && i < 12)
			index = 1;
		if (i >= 12)
			index = 2;
		DrawSurface(SDL_vars->screen, SDL_vars->roadsiteObj[index], queue_roadsiteObj[i].currPosX, int(floor(queue_roadsiteObj[i].currPosY)) + moveY);
		if (queue_roadsiteObj[i].currPosY + carSpeed * deltaTime >= double(SCREEN_HEIGHT) ||
			onRoad(queue_roadsiteObj[i].currPosX, queue_roadsiteObj[i].currPosY, obj_width, road))
		{
			*queue_roadsiteObj_size--;
			queue_roadsiteObj[i].isEmpty = true;
		}
	}
}

bool offroad(const car_T* car, const road_T* road, const int iter)
{
	if (car[iter].isEmpty == false)
	{
		if (car[iter].currPosX < road->borderLeft || car[iter].currPosX > road->borderRight)
		{
			printf("offroad(): \t CAR OFFROAD! Hell yeah!\n");
			return true;
		}
		else return false;
	}
	else return true;
}

//		S E T T I N G    N O N - P L A Y E R    C A R S
int getAvaliableX(bool *avaliableX, const road_T *road, int width)
{
	width += 2*MARGIN;
	int x = -1;
	for (int i = road->borderLeft+1; i < road->borderRight; i++)
	{
		bool ok = true;
		for (int j = -width/2; j <= width/2; j++)
		{
			if (i+j >= road->borderLeft && avaliableX[i + j] == false)
			{
				ok = false;
				i += (width);
				break;
			}
		}
		if (ok == true && i+width/2 < road->borderRight && i-width/2 >= road->borderLeft)
		{
			x = i;
			for (int j = -width/2; j <= width/2; j++)
				avaliableX[i + j] = false;
			break;
		}
	}
	return x;
}

void freeAvaliableX(bool* avaliableX, const int pos, int width)
{
	width += 2 * MARGIN;
	for (int i = -width/2; i <= width/2; i++)
	{
		if(pos+i >= 0 && pos+i < SCREEN_WIDTH)
			avaliableX[pos + i] = true;
	}
}

void drawCars(const SDL_vars_T* SDL_vars, const road_T* road, const double carSpeed, const double deltaTime, car_T* car, 
	const car_T* player, const int type, bool* avaliableX, const double* lastKillTime)
{
	SDL_Surface* SDL_car;
	if (type == neutral)
		SDL_car = SDL_vars->neutral_car;
	else SDL_car = SDL_vars->enemy_car;
	int carsCount = (type == neutral ? NEUTRAL_CARS_MAXCOUNT : ENEMY_CARS_MAXCOUNT);
	int roadWidth = road->borderRight - road->borderLeft + 1 - SDL_car->w / 2;

	for (int i = 0; i < carsCount; i++)
	{
		double speed;
		if (carSpeed >= DEFAULT_CAR_SPEED)
			speed = 1.5 * carSpeed;
		else speed = 3.0 * carSpeed;
		bool outOfScreen = (car[i].currPosY + speed * deltaTime > double(SCREEN_HEIGHT) ? true : false);
		if (outOfScreen)
			freeAvaliableX(avaliableX, car[i].currPosX, car[i].width);
		if (car[i].dontDisplay == true)
		{
			car[i].dontDisplay = false;
			car[i].isEmpty = true;
			freeAvaliableX(avaliableX, car[i].currPosX, car[i].width);
		}
		if ( (road->borderRight - road->borderLeft > 2 * SDL_car->w) && (car[i].isEmpty == true || outOfScreen == true || offroad(car, road, i) == true) )
		{
			car[i].isEmpty = false;
			car[i].carType = type;
			car[i].width = SDL_car->w;
			car[i].height = SDL_car->h;
			car[i].currPosX = getAvaliableX(avaliableX, road, car[i].width);
			if (car[i].currPosX < 0)
			{
				car[i].isEmpty = true;
				continue;
			}
			if (car[i].carType == neutral)
				car[i].currPosY = 2.0;
			else car[i].currPosY = 70.0;
		}
		if(car[i].currPosY > double(SCREEN_HEIGHT-100))
			freeAvaliableX(avaliableX, car[i].currPosX, car[i].width);

		if (SDL_car != NULL)
		{
			DrawSurface(SDL_vars->screen, SDL_car, car[i].currPosX, int(floor(car[i].currPosY)));
			car[i].currPosY += (speed * deltaTime);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//								H E L P F U L 
////////////////////////////////////////////////////////////////////////////////////////////////////

long long min(const long long a, const long long b) {
	return a > b ? b : a;
}

long long max(const long long a, const long long b) {
	return a < b ? b : a;
}

int keyboardHandler(SDL_vars_T* SDL_vars, double* carSpeed, car_T* carPos, bool *shotActvate)
{
	while (SDL_PollEvent(&SDL_vars->event))
	{
		const Uint8* state = SDL_GetKeyboardState(NULL);
		switch (SDL_vars->event.type)
		{
			case SDL_QUIT:
				return EXIT_PROGRAM;
			break;

			case SDL_KEYDOWN:
			{
				if (state[SDL_SCANCODE_SPACE])
					*shotActvate = true;
				if (state[SDL_SCANCODE_P])
					return GAME_PAUSE;

				if (state[SDL_SCANCODE_S])
					return GAME_SAVE;

				if (state[SDL_SCANCODE_L])
					return GAME_LOAD;

				if (state[SDL_SCANCODE_N])
					return EXIT_NEWGAME;

				if (state[SDL_SCANCODE_F])
					return GAME_END;

				if (state[SDL_SCANCODE_ESCAPE])
					return EXIT_PROGRAM;

			break;
			}

			case SDL_KEYUP:
			{
				if (!state[SDL_SCANCODE_SPACE])
					*shotActvate = false;
			break;
			}
		
		}

		if (state[SDL_SCANCODE_UP])
			*carSpeed = 2.0 * DEFAULT_CAR_SPEED;

		if (state[SDL_SCANCODE_DOWN])
			*carSpeed = 0.65 * DEFAULT_CAR_SPEED;

		if (!state[SDL_SCANCODE_UP] && !state[SDL_SCANCODE_DOWN])
			*carSpeed = DEFAULT_CAR_SPEED;

		if (state[SDL_SCANCODE_LEFT])
		{
			carPos->currPosX = carPos->currPosX - MOVE_CAR_X;
			//printf("Car pos: %d\n", carPos->currPosX);
		}

		if (state[SDL_SCANCODE_RIGHT])
		{
			carPos->currPosX = carPos->currPosX + MOVE_CAR_X;
			//printf("Car pos: %d\n", carPos->currPosX);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//								S A V E  /  L O A D  G A M E
////////////////////////////////////////////////////////////////////////////////////////////////////

void fileError_handler(const char fileError, bool* pauseGame, SDL_vars_T* SDL_vars)
{
	printf("fileError_handler(): printed error message, type = %c\n", fileError);
	*pauseGame = true;
	char message1[50], message2[70];
	if (fileError == FILEERROR_LOAD_ERROR)
		sprintf(message1, "FAILED TO LOAD DATA FROM FILE!");
	else
		sprintf(message1, "FAILED TO SAVE DATA TO FILE!");
	sprintf(message2, "Press N to start new game or Esc to exit.");

	DrawRectangle(SDL_vars->screen, 5 * MARGIN, SCREEN_HEIGHT / 2, SCREEN_WIDTH - 10 * MARGIN, 48, getColor(red, SDL_vars), getColor(red, SDL_vars));
	DrawString(SDL_vars->screen, SDL_vars->screen->w / 2 - strlen(message1) * 8 / 2, MARGIN + SCREEN_HEIGHT / 2, message1, SDL_vars->charset);
	DrawString(SDL_vars->screen, SDL_vars->screen->w / 2 - strlen(message2) * 8 / 2, 16 + MARGIN + SCREEN_HEIGHT / 2, message2, SDL_vars->charset);
}

bool saveGame(double *lastTime, double *lastTimePts, double *carSpeed, double* lastRoadDraw, double *shotRange, double* collisionTime, 
	double* collisionOrKillNeutralTime, double* lastKillTime, data_T* data, road_T *road, car_T *carPos, car_T* enemyPos, car_T *neutralPos, 
	roadsite_obj_T *queue_roadsiteObj, int *queue_roadsiteObj_size, pair_T *occupied, bool* avaliableX, bool *pauseGame, bool* shotActivate)
{
	printf("saveGame(): trying to save game\n");
	static char previousFilename[50] = ".";
	char timeStr[50];
	const char ENTER = '\n';
	char extension[] = "gdt";
	char saveName[] = "SpyHunter";
	char filename[60];
	time_t now = time(0);
	tm* gmtm = localtime(&now); // supports DST
	sprintf(timeStr, "%d%02d%02d_%02d%02d%02d", 1900 + gmtm->tm_year, 1 + gmtm->tm_mon, gmtm->tm_mday, gmtm->tm_hour, gmtm->tm_min, gmtm->tm_sec);
	sprintf(filename, "data_saves/%s_%s.%s", saveName, timeStr, extension);

	FILE* f = fopen(filename, "wb");
	FILE* list = fopen("data_saves/filelist.txt", "a+");
	if (f == NULL || list == NULL)
	{
		printf("saveGame(): FATAL ERROR, file cannot be open! f = %a, list = %a\n", f, list);
		return false;
	}
	
	fwrite(lastTime, sizeof(*lastTime), 1, f); 
	fwrite(lastTimePts, sizeof(*lastTimePts), 1, f); 
	fwrite(carSpeed, sizeof(*carSpeed), 1, f); 
	fwrite(lastRoadDraw, sizeof(*lastRoadDraw), 1, f); 
	fwrite(shotRange, sizeof(*shotRange), 1, f); 
	fwrite(collisionTime, sizeof(*collisionTime), 1, f); 
	fwrite(collisionOrKillNeutralTime, sizeof(*collisionOrKillNeutralTime), 1, f);
	fwrite(lastKillTime, sizeof(*lastKillTime), 1, f);
	fwrite(pauseGame, sizeof(*pauseGame), 1, f);
	fwrite(data, sizeof(*data), 1, f); 
	fwrite(road, sizeof(*road), 1, f); 
	fwrite(carPos, sizeof(*carPos), 1, f); 
	fwrite(queue_roadsiteObj_size, sizeof(*queue_roadsiteObj_size), 1, f);
	for (int i = 0; i < ROADSIDE_OBJECTS_MAXCOUNT; i++)
		fwrite(queue_roadsiteObj + i, sizeof(queue_roadsiteObj[0]), 1, f);

	for (int i = 0; i < SCREEN_WIDTH; i++)
		fwrite(avaliableX + i, sizeof(avaliableX[0]), 1, f);

	for (int i = 0; i < ENEMY_CARS_MAXCOUNT; i++)
		fwrite(enemyPos + i, sizeof(enemyPos[0]), 1, f);
	
	for (int i = 0; i < NEUTRAL_CARS_MAXCOUNT; i++)
		fwrite(neutralPos + i, sizeof(neutralPos[0]), 1, f);
	
	for (int i = 0; i < NEUTRAL_CARS_MAXCOUNT + ENEMY_CARS_MAXCOUNT; i++)
		fwrite(occupied + i, sizeof(occupied[0]), 1, f);

	if (strcmp(previousFilename, filename) != 0)
	{
		fwrite(filename, sizeof(filename[0]) * strlen(filename), 1, list);

		fwrite(&ENTER, sizeof(char), 1, list);
		sprintf(previousFilename, filename);
	}

	fclose(f);
	fclose(list);
	printf("saveGame(): %s saved sucessfully at %s\n", filename, timeStr);
	return true;
}

int getSavename(SDL_vars_T* SDL_vars, char* chosenFilename, const bool pauseGame) 
{
	FILE* list = fopen("data_saves/filelist.txt", "r");
	int countSaves = 0;
	int firstFileName = 0, offset = 0;
	const int listLen = 8;
	unsigned long long int chosen = 0;
	bool fileChosen = false;
	char list_filenames[MAX_SAVES][42];

	while (fgets(list_filenames[countSaves], 42, list) != NULL && countSaves + 1 < MAX_SAVES)
	{
		list_filenames[countSaves][40] = '\0'; // get rid of '\n'
		countSaves++;
	}

	char cursor[3] = "->";
	while (fileChosen == false)
	{
		while(SDL_PollEvent(&SDL_vars->event))
		{
			const Uint8* state = SDL_GetKeyboardState(NULL);
			switch (SDL_vars->event.type)
			{
			case SDL_KEYDOWN:
			{
				switch(SDL_vars->event.key.keysym.sym)
				{
				case SDLK_UP:
					chosen = max(chosen - 1, 0);
					break;
				
				case SDLK_DOWN:
					chosen = min(chosen + 1, countSaves-1);
				}

				if (state[SDL_SCANCODE_RETURN])
					fileChosen = true;

				if (state[SDL_SCANCODE_ESCAPE])
					return EXIT_BY_USER;
			}
			break;
			}
		}
		DrawRectangle(SDL_vars->screen, 5*MARGIN, SCREEN_HEIGHT/2, SCREEN_WIDTH - 10*MARGIN, 20*listLen, getColor(blue, SDL_vars), getColor(black, SDL_vars));
		char message[] = "Select save to load and press Enter.";
		DrawString(SDL_vars->screen, 200 + 7*MARGIN, 3*MARGIN + SCREEN_HEIGHT/2, message, SDL_vars->charset);
		for (int i = 0; i < min(listLen, countSaves); i++)
		{
			offset = chosen / min(listLen, countSaves);
			if (i + offset*min(listLen, countSaves) < countSaves)
			{
				if (i + offset * min(listLen, countSaves) == chosen)
					DrawString(SDL_vars->screen, 7*MARGIN, (i + 2)*3*MARGIN + SCREEN_HEIGHT/2, cursor, SDL_vars->charset);
				DrawString(SDL_vars->screen, 11*MARGIN, (i + 2)*3*MARGIN + SCREEN_HEIGHT/2, list_filenames[i + offset*min(listLen, countSaves)], SDL_vars->charset);
			}
		}
		SDL_UpdateTexture(SDL_vars->scrtex, NULL, SDL_vars->screen->pixels, SDL_vars->screen->pitch);
		SDL_RenderCopy(SDL_vars->renderer, SDL_vars->scrtex, NULL, NULL);
		SDL_RenderPresent(SDL_vars->renderer);
	}
	sprintf(chosenFilename, list_filenames[chosen]);
	printf("getSavename(): filename chosen.\n");
	return EXIT_SUCCESS;
}

int loadGame(SDL_vars_T *SDL_vars, double* lastTime, double* lastTimePts, double* carSpeed, double* lastRoadDraw, double* shotRange, double* collisionTime,
	double* collisionOrKillNeutralTime, double* lastKillTime, data_T* data, road_T* road, car_T* carPos, car_T* enemyPos, car_T* neutralPos,
	roadsite_obj_T* queue_roadsiteObj, int* queue_roadsiteObj_size, pair_T* occupied, bool* avaliableX, bool* pauseGame, bool* shotActivate)
{
	printf("loadGame(): trying to load game\n");

	char savename[42] = ";";
	int result = getSavename(SDL_vars, savename, *pauseGame);
	if (result == EXIT_BY_USER)
	{
		printf("loadGame(): operation cancelled.\n");
		return EXIT_BY_USER;
	}
	printf("loadGame(): save name is %s\n", savename);

	FILE* f = fopen(savename, "rb");
	if (f == NULL)
	{
		printf("loadGame(): FATAL ERROR, file cannot be open!\n");
		return EXIT_FAILURE;
	}

	fread(lastTime, sizeof(*lastTime), 1, f);
	fread(lastTimePts, sizeof(*lastTimePts), 1, f);
	fread(carSpeed, sizeof(*carSpeed), 1, f);
	fread(lastRoadDraw, sizeof(*lastRoadDraw), 1, f);
	fread(shotRange, sizeof(*shotRange), 1, f);
	fread(collisionTime, sizeof(*collisionTime), 1, f);
	fread(collisionOrKillNeutralTime, sizeof(*collisionOrKillNeutralTime), 1, f);
	fread(lastKillTime, sizeof(*lastKillTime), 1, f);
	fread(pauseGame, sizeof(*pauseGame), 1, f);
	fread(data, sizeof(*data), 1, f);
	fread(road, sizeof(*road), 1, f);
	fread(carPos, sizeof(*carPos), 1, f);
	fread(queue_roadsiteObj_size, sizeof(*queue_roadsiteObj_size), 1, f);
	for (int i = 0; i < ROADSIDE_OBJECTS_MAXCOUNT; i++)
		fread(queue_roadsiteObj + i, sizeof(queue_roadsiteObj[0]), 1, f);

	for (int i = 0; i < SCREEN_WIDTH; i++)
		fread(avaliableX + i, sizeof(avaliableX[0]), 1, f);

	for (int i = 0; i < ENEMY_CARS_MAXCOUNT; i++)
		fread(enemyPos + i, sizeof(enemyPos[0]), 1, f);

	for (int i = 0; i < NEUTRAL_CARS_MAXCOUNT; i++)
		fread(neutralPos + i, sizeof(neutralPos[0]), 1, f);

	for (int i = 0; i < NEUTRAL_CARS_MAXCOUNT + ENEMY_CARS_MAXCOUNT; i++)
		fread(occupied + i, sizeof(occupied[0]), 1, f);

	
	if (*pauseGame == true)
		*pauseGame = false;

	fclose(f);
	printf("loadGame(): data loaded sucessfully\n");
	return EXIT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//							G A M E P L A Y
////////////////////////////////////////////////////////////////////////////////////////////////
void isCollision(const SDL_vars_T* SDL_vars, data_T* data, const car_T* carPos, const road_T* road, car_T* enemyPos, car_T *neutralPos)
{
	if (carPos->currPosX < road->borderLeft || carPos->currPosX > road->borderRight)
	{
		if (carPos->currPosX < SCREEN_WIDTH * 10 && int(floor(carPos->currPosY)) < SCREEN_HEIGHT * 10)
		{
			data->collision = collision_offroad;
			data->canShoot = false;
		}
		else 
			data->collision = collision_none;
		
		return;
	}
	int width = SDL_vars->enemy_car->w;
	int height = SDL_vars->enemy_car->h;
	for (int i = 0; i < ENEMY_CARS_MAXCOUNT; i++)
	{
		if (enemyPos[i].isEmpty == false && (enemyPos[i].currPosX - width/2 <= carPos->currPosX && carPos->currPosX <= enemyPos[i].currPosX + width/2))
		{
			if (floor(enemyPos[i].currPosY)-height/2 <= floor(carPos->currPosY) && floor(carPos->currPosY) <= floor(enemyPos[i].currPosY) + height/2) // collision from leftside or rightside
			{
				data->collision = collision_enemy;
				enemyPos[i].dontDisplay = true;
				printf("isCollision(): Collision with enemy\n");
				return;
			}
		}
	}
	width = SDL_vars->neutral_car->w;
	height = SDL_vars->neutral_car->h;
	for (int i = 0; i < NEUTRAL_CARS_MAXCOUNT; i++)
	{
		if (neutralPos[i].isEmpty == false && (neutralPos[i].currPosX - width / 2 <= carPos->currPosX && carPos->currPosX <= neutralPos[i].currPosX + width / 2))
		{
			if (floor(neutralPos[i].currPosY) - height / 2 <= floor(carPos->currPosY) && floor(carPos->currPosY) <= floor(neutralPos[i].currPosY) + height / 2) // collision from leftside or rightside
			{
				data->collision = collision_neutral;
				neutralPos[i].dontDisplay = true;
				printf("isCollision(): Collision with neutral\n");
				return;
			}
		}
	}
	data->collision = collision_none;
	data->canShoot = true;
}

void collision_handler(const car_T* enemyPos, car_T* neutralPos, const pair_T* occupied, data_T* data, car_T* carPos, double* collisionTime,
	const int PLAYER_POSX, const int PLAYER_POSY, const road_T* road, double * collisionOrKillNeutralTime)
{
	switch (data->collision)
	{
	case collision_none:
		return;
		break;

	case collision_offroad: case collision_enemy: case collision_neutral:
		data->collisionPlaceX = carPos->currPosX;
		data->collisionPlaceY = carPos->currPosY;
		carPos->currPosX = SCREEN_WIDTH * 1000;
		carPos->currPosY = SCREEN_HEIGHT * 1000;
		if (data->collision == collision_neutral)
			*collisionOrKillNeutralTime = data->worldTime;
		data->collision = collision_none;
		*collisionTime = data->worldTime;
		printf("Collision time: %.02lf\n", *collisionTime);
		break;
	}
}

void shot(SDL_vars_T* SDL_vars, const int x, const int y, car_T* enemyPos, car_T* neutralPos, data_T* data, double* lastKillTime, double *killNeutralTime, const double shotRange)
{
	if (data->collision != collision_none || data->canShoot == false || x > SCREEN_WIDTH || y > SCREEN_HEIGHT)
		return;

	int shotCarType = 0, index = -1;
	int nearestTargetY = INT_MAX;
	int nearestTargetX = INT_MAX;
	int width = 0;

	if (data->worldTime - *lastKillTime < 0.25)
	{
		nearestTargetY = y - 50;
		DrawRectangle(SDL_vars->screen, x, nearestTargetY, MARGIN, y - (SDL_vars->car->h / 2) - nearestTargetY, getColor(red, SDL_vars), getColor(red, SDL_vars));
		return;
	}

	for (int i = 0; i < ENEMY_CARS_MAXCOUNT; i++)
	{
		if (enemyPos[i].isEmpty == false)
		{
			width = enemyPos[i].width;
			if (enemyPos[i].currPosX - width / 2 <= x && x <= enemyPos[i].currPosX + width / 2)
			{
				if (nearestTargetY > y - int(floor(enemyPos[i].currPosY)) && y - int(floor(enemyPos[i].currPosY)) < shotRange)
				{
					nearestTargetY = y - int(floor(enemyPos[i].currPosY));
					shotCarType = enemy;
					index = i;
				}
			}
		}
	}
	for (int i = 0; i < NEUTRAL_CARS_MAXCOUNT; i++)
	{
		if (neutralPos[i].isEmpty == false)
		{
			width = neutralPos[i].width;
			if (neutralPos[i].currPosX - width / 2 <= x && x <= neutralPos[i].currPosX + width / 2)
			{
				if (nearestTargetY > y - int(floor(neutralPos[i].currPosY)) && y - int(floor(neutralPos[i].currPosY)) < shotRange)
				{
					nearestTargetY = y - int(floor(neutralPos[i].currPosY));
					shotCarType = neutral;
					index = i;
				}
			}
		}
	}

	if (nearestTargetY != INT_MAX)
	{
		if (shotCarType == enemy)
		{
			enemyPos[index].dontDisplay = true;
			nearestTargetY = int(floor(enemyPos[index].currPosY));
			data->points += POINTS_SHOT;
			printf("shot(): Enemy car killed at (%d, %d), distance is: %d\n", enemyPos[index].currPosX, int(floor(enemyPos[index].currPosY)), y - int(floor(enemyPos[index].currPosY)));
		}
		else
		{
			neutralPos[index].dontDisplay = true;
			nearestTargetY = int(floor(neutralPos[index].currPosY));
			printf("shot(): Neutral car killed at (%d, %d), distance is: %d\n", neutralPos[index].currPosX, int(floor(neutralPos[index].currPosY)), y - int(floor(neutralPos[index].currPosY)));
			*killNeutralTime = data->worldTime;
		}
		*lastKillTime = data->worldTime;
	}
	else
		nearestTargetY = y - 10;

	DrawRectangle(SDL_vars->screen, x, nearestTargetY, MARGIN, y - (SDL_vars->car->h / 2) - nearestTargetY, getColor(white, SDL_vars), getColor(white, SDL_vars));
}

void generatePowerup(SDL_vars_T* SDL_vars, data_T* data, const road_T* road, const double shotRange)
{
	if (data->powerupExists == false && shotRange == DEFAULT_SHOT_RANGE && data->worldTime - data->powerupCreationTime > POWERUP_GENERATE_TIME_CONDITION)
	{
		data->powerupExists = true;
		int currtime = int(floor(data->worldTime));
		int mod = 1 + rand() % ((SCREEN_WIDTH + SCREEN_HEIGHT) / (ENEMY_CARS_MAXCOUNT * MARGIN + NEUTRAL_CARS_MAXCOUNT));
		if (currtime % mod == 0)
		{
			data->powerupCreationTime = data->worldTime;
			int roadWidth = road->borderRight - road->borderLeft;
			data->powerupX = road->borderLeft + rand() % roadWidth;
			data->powerupY = double(min(64  + rand() % (SCREEN_HEIGHT - SDL_vars->car->h - 64), SCREEN_HEIGHT-SDL_vars->powerup->h)); // 64 is width of info window
			printf("generatePowerup(): powerup generated\n");
		}
	}
	else
	{
		if (data->worldTime - data->powerupCreationTime > POWERUP_TIMETOLIVE || int(floor(data->powerupY)) >= SCREEN_HEIGHT)
			data->powerupExists = false;
		else
		{
			DrawSurface(SDL_vars->screen, SDL_vars->powerup, data->powerupX, data->powerupY);
			data->powerupY += 1;
		}
	}
}

double catchPowerup(SDL_vars_T *SDL_vars, car_T* carPos, data_T* data, double *shotRange)
{
	static double timeWhenCaught = -1.0;
	if (data->powerupExists == true)
	{
		int carWidth = SDL_vars->car->w;
		int carHeight = SDL_vars->car->h;
		if (carPos->currPosX - carWidth/2 <= data->powerupX && data->powerupX <= carPos->currPosX + carWidth/2 &&
			carPos->currPosY - carHeight/2 <= data->powerupY && data->powerupY <= carPos->currPosY + carHeight/2)
		{
			timeWhenCaught = data->worldTime;
			data->powerupExists = false;
			printf("catchPowerup(): powerup caught\n");
		}
	}
	if (timeWhenCaught > 0.0)
	{
		if (data->worldTime - timeWhenCaught < POWERUP_TIMETOLIVE)
		{
			(*shotRange) = POWERUP_ACTIVE_SHOT_RANGE;
			char message[] = "Powerup activated!";
			DrawString(SDL_vars->screen, 3 * SCREEN_WIDTH / 4, 56, message, SDL_vars->charset);
		}
		else
		{
			*(shotRange) = DEFAULT_SHOT_RANGE;
			data->powerupExists = false;
			timeWhenCaught = -1.0;
			printf("catchPowerup(): powerup expired\n");
		}
	}
	return timeWhenCaught;
}

void addPoints(data_T *data, double *lastTimePts, const double collisionTime, const double collisionOrKillNeutralTime, const double carSpeed)
{
	bool addPoints = false;
	if (data->worldTime - (*lastTimePts) >= 1.0)
		addPoints = true;
	if (addPoints == true && data->collision == collision_none)
		addPoints = true;
	else
		addPoints = false;
	if (addPoints == true && data->worldTime - collisionTime >= 2.0 && data->worldTime - collisionOrKillNeutralTime >= 5.0)
		addPoints = true;
	else
		addPoints = false;

	if (addPoints == true)
	{
		if (carSpeed / DEFAULT_CAR_SPEED > 1.0)
			data->points += 100; // same as in original game
		else
			data->points += 50; // same as in original game

		(*lastTimePts) = data->worldTime;
	}
}

/////////////////////////////////////////////////////////////////////////////////
//							E N D    G A M E 
/////////////////////////////////////////////////////////////////////////////////
void endgameHandler(SDL_vars_T *SDL_vars, const double worldTime, const long long points)
{
	printf("endgameHandler(): printing results\n");
	DrawRectangle(SDL_vars->screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, getColor(blue, SDL_vars));
	char text[100];
	sprintf(text, "Your score is: %lld. Your playtime is: %.02lf s", points, worldTime);
	DrawString(SDL_vars->screen, SCREEN_WIDTH / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2, text, SDL_vars->charset);
	char text2[] = "Window will close in 10 seconds from now.";
	DrawString(SDL_vars->screen, SCREEN_WIDTH - 8 * strlen(text2) - 10 * MARGIN, 3 * SCREEN_HEIGHT / 4, text2, SDL_vars->charset);
	SDL_UpdateTexture(SDL_vars->scrtex, NULL, SDL_vars->screen->pixels, SDL_vars->screen->pitch);
	SDL_RenderCopy(SDL_vars->renderer, SDL_vars->scrtex, NULL, NULL);
	SDL_RenderPresent(SDL_vars->renderer);
	double time = SDL_GetTicks() * 0.001;
	while (0.001 * SDL_GetTicks() - time < 10.0) // waiting time = 10s
		continue;
}

/////////////////////////////////////////////////////////////////////////////////
//							N E W    G A M E 
/////////////////////////////////////////////////////////////////////////////////
int newGame(SDL_vars_T* SDL_vars)
{
	printf("newGame() has started\n");
	int t1, t2, frames = 0;
	double delta, lastTime = 0, lastTimePts = 0, fpsTimer = 0;
	double carSpeed = DEFAULT_CAR_SPEED, shotRange = DEFAULT_SHOT_RANGE;
	double lastRoadDraw = 0.0, collisionTime = -10.0, collisionOrKillNeutralTime = -10.0, lastKillTime = -10.0;
	data_T data = { 0.0, 0.0, 0, false, -1, -1.0, true, 0, 0.0 };
	road_T road = { ROAD_BORDER_MIN, ROAD_BORDER_MAX };
	const int PLAYER_POSX = road.borderLeft + (road.borderRight - road.borderLeft) / 2 + 8;
	const double PLAYER_POSY = double(SCREEN_HEIGHT - MARGIN - SDL_vars->car->h);
	car_T carPos = { true, false, player, PLAYER_POSX, PLAYER_POSY, SDL_vars->car->w,  SDL_vars->car->h };
	car_T enemyPos[ENEMY_CARS_MAXCOUNT], neutralPos[NEUTRAL_CARS_MAXCOUNT];
	roadsite_obj_T queue_roadsiteObj[ROADSIDE_OBJECTS_MAXCOUNT];
	int queue_roadsiteObj_size = 0;
	pair_T occupied[NEUTRAL_CARS_MAXCOUNT + ENEMY_CARS_MAXCOUNT];
	bool avaliableX[SCREEN_WIDTH] = { 0 };
	for (int i = 0; i < SCREEN_WIDTH; i++)
		avaliableX[i] = true;
	bool pauseGame = false, shotActivate = false;
	char fileError = FILEERROR_NO_ERRORS;
	bool pauseChanged = false;

	t1 = SDL_GetTicks();
	while (true)
	{
		int returnKeyboardHandler = keyboardHandler(SDL_vars, &carSpeed, &carPos, &shotActivate);
		switch (returnKeyboardHandler)
		{
			case EXIT_NEWGAME:
				return EXIT_NEWGAME;
			break;

			case EXIT_PROGRAM:
				return EXIT_PROGRAM;
			break;
			
			case GAME_PAUSE:
				pauseGame ^= 1;
			break;

			case GAME_SAVE:
			{
				bool changed = false;
				if (pauseGame == false)
				{
					pauseGame = true;
					changed = true;
				}
				if (saveGame(&lastTime, &lastTimePts, &carSpeed, &lastRoadDraw, &shotRange, &collisionTime, &collisionOrKillNeutralTime, &lastKillTime,
					&data, &road, &carPos, enemyPos, neutralPos, queue_roadsiteObj, &queue_roadsiteObj_size,
					occupied, avaliableX, &pauseGame, &shotActivate) == false)
				{
					fileError = FILEERROR_SAVE_ERROR;
				}
				if (changed == true)
				{
					pauseGame = false;
					t1 = SDL_GetTicks();
				}
			break;
			}

			case GAME_LOAD:
			{
				int exitcode = loadGame(SDL_vars, &lastTime, &lastTimePts, &carSpeed, &lastRoadDraw, &shotRange, &collisionTime, &collisionOrKillNeutralTime, &lastKillTime,
					&data, &road, &carPos, enemyPos, neutralPos, queue_roadsiteObj, &queue_roadsiteObj_size,
					occupied, avaliableX, &pauseGame, &shotActivate);
				
				if (exitcode == EXIT_FAILURE)
					fileError = FILEERROR_LOAD_ERROR;

			break;
			}

			case GAME_END:
			{
				endgameHandler(SDL_vars, data.worldTime, data.points);
				return EXIT_PROGRAM;
			}
			break;
		}
		if (pauseGame == false)
		{
			// time handler
			t2 = SDL_GetTicks();
			delta = (t2 - t1) * 0.001;
			t1 = t2;
			data.worldTime += delta;
			fpsTimer += delta;
			if (fpsTimer > 0.5) 
			{
				data.fps = frames * 2;
				frames = 0;
				fpsTimer -= 0.5;
			}

			SDL_FillRect(SDL_vars->screen, NULL, getColor(black, SDL_vars));
			if (fileError != FILEERROR_NO_ERRORS)
				fileError_handler(fileError, &pauseGame, SDL_vars);
			else
			{
				DrawRectangle(SDL_vars->screen, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, getColor(green, SDL_vars)); // background
				drawRoad(SDL_vars, &road, &data);
				drawRoadside(SDL_vars, &road, carSpeed, delta, queue_roadsiteObj, &queue_roadsiteObj_size, frames);
				// printing player's car
				if (data.worldTime - collisionTime >= 2.0)
				{
					if (carPos.currPosX > SCREEN_WIDTH && carPos.currPosY > SCREEN_HEIGHT)
					{
						printf("Player back in game!\n");
						carPos.currPosX = PLAYER_POSX;
						carPos.currPosY = PLAYER_POSY;
					}
					if (carSpeed == 0)
						carSpeed = DEFAULT_CAR_SPEED;
					DrawSurface(SDL_vars->screen, SDL_vars->car, carPos.currPosX, SCREEN_HEIGHT - MARGIN - carPos.height);
				}
				else
				{
					DrawSurface(SDL_vars->screen, SDL_vars->collision, data.collisionPlaceX, data.collisionPlaceY);
					carSpeed = 0;
				}
				// printing other cars
				drawCars(SDL_vars, &road, carSpeed, delta, enemyPos, &carPos, enemy, avaliableX, &lastKillTime);
				drawCars(SDL_vars, &road, carSpeed, delta, neutralPos, &carPos, neutral, avaliableX, &lastKillTime);
				printAuthorControls(start, SDL_vars, &data);
				
				isCollision(SDL_vars, &data, &carPos, &road, enemyPos, neutralPos);
				if (data.worldTime > 10.0 && shotRange == DEFAULT_SHOT_RANGE)
					generatePowerup(SDL_vars, &data, &road, shotRange);
				catchPowerup(SDL_vars, &carPos, &data, &shotRange);
				if (shotActivate == true)
					shot(SDL_vars, carPos.currPosX, carPos.currPosY, enemyPos, neutralPos, &data, &lastKillTime, &collisionOrKillNeutralTime, shotRange);
				if (data.collision != collision_none)
					collision_handler(enemyPos, neutralPos, occupied, &data, &carPos, &collisionTime, PLAYER_POSX, PLAYER_POSY, &road, &collisionOrKillNeutralTime);
				
				addPoints(&data, &lastTimePts, collisionTime, collisionOrKillNeutralTime, carSpeed);
				frames++;
			}
			SDL_UpdateTexture(SDL_vars->scrtex, NULL, SDL_vars->screen->pixels, SDL_vars->screen->pitch);
			SDL_RenderCopy(SDL_vars->renderer, SDL_vars->scrtex, NULL, NULL);
			SDL_RenderPresent(SDL_vars->renderer);
		}
		else
		{
			t1 = SDL_GetTicks();
		}
	}
}

int main(int argc, char** argv)
{
	printf("######################################\nStarting main() a.k.a driver function\n");
	srand(time(NULL));
	// W³¹czenie konsoli: project -> szablon2 properties -> Linker -> System -> Subsystem: zmieniæ na "Console"
	SDL_vars_T SDL_vars;

	if (getWindow(&SDL_vars) == 1)
	{
		printf("\n\n FATAL ERROR! getWindow() returned 1\n\n");
		return 1;
	}
	bool clearMemAndExit = false;
	int debugCntNewGames = 0;

	while (!clearMemAndExit)
	{
		int EXIT_CODE = newGame(&SDL_vars);
		debugCntNewGames++;
		printf("Number of new games: %d\n", debugCntNewGames);
		switch (EXIT_CODE)
		{
		case EXIT_PROGRAM:
			clearMemAndExit = true;
			break;
		case EXIT_NEWGAME:
			break;
		}
	}

	printf("main(): trying to free SDL_Surfaces\n");
	SDL_FreeSurface(SDL_vars.charset);
	SDL_FreeSurface(SDL_vars.screen);
	SDL_FreeSurface(SDL_vars.roadsiteObj[0]);
	SDL_FreeSurface(SDL_vars.roadsiteObj[1]);
	SDL_FreeSurface(SDL_vars.roadsiteObj[2]);
	SDL_FreeSurface(SDL_vars.car);
	SDL_FreeSurface(SDL_vars.enemy_car);
	SDL_FreeSurface(SDL_vars.neutral_car);
	SDL_FreeSurface(SDL_vars.collision);
	SDL_FreeSurface(SDL_vars.powerup);
	SDL_DestroyTexture(SDL_vars.scrtex);
	SDL_DestroyRenderer(SDL_vars.renderer);
	SDL_DestroyWindow(SDL_vars.window);
	SDL_Quit();
	
	printf("main(): Done. Program closed correctly\n");
	return 0;
}
