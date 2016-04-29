//Albert Su
//Very basic platformer, just jump around

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include "ShaderProgram.h"
#include "Matrix.h"
#include "Entity.h"
#include "vector"
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define LEVEL_HEIGHT 25
#define LEVEL_WIDTH 25


SDL_Window* displayWindow;
float lastFrameTicks = 0.0f;
float FIXED_TIMESTEP = 0.0166666f;
int MAX_TIMESTEPS = 6;

float TILE_SIZE = 0.5;
int SPRITE_COUNT_X = 20;
int SPRITE_COUNT_Y = 13;

unsigned char** levelData;
int mapWidth = 0;
int mapHeight = 0;
vector<float> vertexData;
vector<float> texCoordData;
int tileCount = 0;

GLuint LoadTexture(const char *image_path){
	SDL_Surface *surface = IMG_Load(image_path);
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SDL_FreeSurface(surface);
	return textureID;
}

bool readHeader(std::ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height"){
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data, done elsewhere
		levelData = new unsigned char*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned char[mapWidth];
		}
		return true;
	}
}



bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned char val = (unsigned char)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}

void readFile()
{
	ifstream infile("level.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				break;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
	}

	for (int y = 0; y < LEVEL_HEIGHT; y++) {
		for (int x = 0; x < LEVEL_WIDTH; x++) {
			if (levelData[y][x] != 0) {
				tileCount++;
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
				vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});
				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
	}
}

void updatePlayer(Entity* player)
{
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_RIGHT])
		player->acceleration_x = 2;
	if (keys[SDL_SCANCODE_LEFT])
		player->acceleration_x = -2;
	if (keys[SDL_SCANCODE_UP] && player->collidedBottom)
		player->velocity_y = 3.0f;

	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;

	float fixedElapsed = elapsed;
	if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
		fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
	}
	while (fixedElapsed >= FIXED_TIMESTEP) {
		fixedElapsed -= FIXED_TIMESTEP;
		player->Update(FIXED_TIMESTEP);
	}
	player->Update(fixedElapsed);
}

void checkPlayer(Entity* player)
{
	int gridX, gridY;
	//bottom collision
	worldToTileCoordinates(player->x, player->y - player->height / 2, &gridX, &gridY);
	if (levelData[gridY][gridX] != 0)
	{
		player->velocity_y = 0;
		player->collidedBottom = true;
	}
	//top collision
	worldToTileCoordinates(player->x, player->y + player->height / 2, &gridX, &gridY);
	if (levelData[gridY][gridX] != 0)
	{
		player->velocity_y = 0;
		player->collidedTop = true;
	}
	//left collision
	worldToTileCoordinates(player->x - player->width / 2, player->y, &gridX, &gridY);
	if (levelData[gridY][gridX] != 0)
	{
		player->velocity_x = 0;
		player->collidedLeft = true;
	}
	//right collision
	worldToTileCoordinates(player->x + player->width / 2, player->y, &gridX, &gridY);
	if (levelData[gridY][gridX] != 0)
	{
		player->velocity_x = 0;
		player->collidedRight = true;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif
	SDL_Event event;
	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint playerTexture = LoadTexture("player.png");
	GLuint spriteSheetTexture = LoadTexture("sheet.png");
	GLuint ballTexture = LoadTexture("ball.png");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	SheetSprite playerSprite(&program, playerTexture, 0, 0, 1, 1, 0.3);
	SheetSprite ballSprite(&program, ballTexture, 0, 0, 1, 1, 0.3);
	Entity player(playerSprite, 1.0, -11.5, .5, .4, 0, 0, 0, 0);
	Entity player2(playerSprite, 11.5, -11.8, .5, .4, 0, 0, 0, 0);
	Entity ball(ballSprite, 5.0, -11.7, .5, .4, 0, 0, 0, 0);

	projectionMatrix.setOrthoProjection(-6.5f, 6.5f, -7.0f, 7.0f, -7.0f, 1.0f);
	program.setProjectionMatrix(projectionMatrix);

	readFile();

	glUseProgram(program.programID);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		//draw level
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		glUseProgram(program.programID);
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
		glEnableVertexAttribArray(program.positionAttribute);
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
		glEnableVertexAttribArray(program.texCoordAttribute);
		glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
		glDrawArrays(GL_TRIANGLES, 0, tileCount * 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//update and draw player
		updatePlayer(&player);
		checkPlayer(&player);
		player.Render(&program);
		ball.Render(&program);
		player2.Render(&program);
		
		//camera following player
		viewMatrix.identity();
		viewMatrix.Translate(-6.0, 6.0, 0);
		program.setViewMatrix(viewMatrix);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
