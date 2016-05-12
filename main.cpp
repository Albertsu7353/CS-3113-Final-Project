//Albert Su
//Score goals

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
const Uint8 *keys = SDL_GetKeyboardState(NULL);

//ParticleEmitter particles = ParticleEmitter(100);


enum{ START, GAME, VICTORY1, VICTORY2 };
int state;

int p1Score = 0;
int p2Score = 0;
int level = 0;

Mix_Chunk *collision;
Mix_Chunk *score;

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

void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		float texture_x = (float)(((int)text[i]) % 16) / 16.0f;
		float texture_y = (float)(((int)text[i]) / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glUseProgram(program->programID);
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
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

void readFile(string level)
{
	ifstream infile(level);
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

void updateEntities(Entity* player, Entity* player2, Entity* ball)
{
	if (keys[SDL_SCANCODE_D])
		player->acceleration_x = 5;
	if (keys[SDL_SCANCODE_A])
		player->acceleration_x = -5;
	if (keys[SDL_SCANCODE_W] && player->collidedBottom)
	{
		Mix_PlayChannel(-1, collision, 0);
		player->velocity_y = 7.0f;
	}
		
	if (ball->type == 3)
	{
		if (keys[SDL_SCANCODE_S])
		{
			ball->velocity_x = player->velocity_x + 5.0;
			ball->velocity_y = player->velocity_y;
			ball->type = 1;
			player->type = 0;
		}
	}

	if (keys[SDL_SCANCODE_RIGHT])
		player2->acceleration_x = 5;
	if (keys[SDL_SCANCODE_LEFT])
		player2->acceleration_x = -5;
	if (keys[SDL_SCANCODE_UP] && player2->collidedBottom)
	{
		Mix_PlayChannel(-1, collision, 0);
		player2->velocity_y = 7.0f;
	}
		
	if (ball->type == 4)
	{
		if (keys[SDL_SCANCODE_DOWN])
		{
			ball->velocity_x = player2->velocity_x - 5.0;
			ball->velocity_y = player2->velocity_y;
			ball->type = 1;
			player2->type = 5;
		}
	}


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
		player2->Update(FIXED_TIMESTEP);
		ball->Update(FIXED_TIMESTEP);
		//particles.Update(FIXED_TIMESTEP);
	}
	player->Update(fixedElapsed);
	player2->Update(fixedElapsed);
	ball->Update(fixedElapsed);
	//particles.Update(fixedElapsed);
}

void checkPlayer(Entity* player)
{
	int gridX, gridY;
	if (player->type == 1)
	{
		worldToTileCoordinates(player->x, player->y - player->height / 2, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			if (level == 1)
				player->velocity_y = -(player->velocity_y)*0.999;
			else if (level == 2)
			{
			player->velocity_y = -(player->velocity_y)*1.001;
			}
			else if (level == 3)
			{
				player->velocity_y = 0.1;
				player->friction_x = 1.001;
			}
			
			player->collidedBottom = true;
		}
		//top collision
		worldToTileCoordinates(player->x, player->y + player->height / 2, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			player->velocity_y = -0.5;
			player->collidedTop = true;
				
		}
		//left collision
		worldToTileCoordinates(player->x - player->width / 2, player->y, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			player->velocity_x = -(player->velocity_x) + 0.01;
			player->collidedLeft = true;
		}
		//right collision
		worldToTileCoordinates(player->x + player->width / 2, player->y, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			player->velocity_x = -(player->velocity_x) - 0.01;
			player->collidedRight = true;	
		}
	}
	else
	{
		//bottom collision
		worldToTileCoordinates(player->x, player->y - player->height / 2, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			if (level == 1)
				player->velocity_y = 0.1;
			else if (level == 2)
				player->velocity_y = -(player->velocity_y) * 1.01;
			else if (level == 3)
			{
				player->velocity_y = 0.1;
				player->friction_x = 1.1;
			}
			player->collidedBottom = true;
		}
		//top collision
		worldToTileCoordinates(player->x, player->y + player->height / 2, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			player->velocity_y = -0.5;
			player->collidedTop = true;
		}
		//left collision
		worldToTileCoordinates(player->x - player->width / 2, player->y, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			player->velocity_x = 0.1;
			player->collidedLeft = true;
		}
		//right collision
		worldToTileCoordinates(player->x + player->width / 2, player->y, &gridX, &gridY);
		if (levelData[gridY][gridX] != 0)
		{
			player->velocity_x = -0.1;
			player->collidedRight = true;
		}
	}
}
void checkEntity(Entity* e1, Entity* e2,Entity* ball)
{
	if (e1->type == 2 && e2->type == 5)//player 2 resets player 1
	{
		if (e1->x < e2->x && e1->x + 0.3 > e2->x - 0.3)
		{
			if (e1->y > e2->y && e1->y - 0.3 < e2->y + 0.3)
			{
				e1->x = 1;
				e1->y = -1.5;
				e1->type = 0;
				e2->type = 2;
				e1->velocity_x = 0;
				ball->type = 4;
			}
			else if (e1->y < e2->y && e1->y + 0.3 > e2->y - 0.3)
			{
				e1->x = 1;
				e1->y = -1.5;
				e1->type = 0;
				e2->type = 2;
				e1->velocity_x = 0;
				ball->type = 4;
			}
		}
		if (e1->x > e2->x && e1->x - 0.3 < e2->x + 0.3)
		{
			if (e1->y > e2->y && e1->y - 0.3 < e2->y + 0.3)
			{
				e1->x = 1;
				e1->y = -1.5;
				e1->type = 0;
				e2->type = 2;
				e1->velocity_x = 0;
				ball->type = 4;
			}
			else if (e1->y < e2->y && e1->y + 0.3 > e2->y - 0.3)
			{
				e1->x = 1;
				e1->y = -1.5;
				e1->type = 0;
				e2->type = 2;
				e1->velocity_x = 0;
				ball->type = 4;
			}
		}
	}
	if (e1->type == 0 && e2->type == 2)//player 1 resets player 2
	{
		if (e1->x < e2->x && e1->x + 0.3 > e2->x - 0.3)
		{
			if (e1->y > e2->y && e1->y - 0.3 < e2->y + 0.3)
			{
				e2->x = 11.5;
				e2->y = -1.5;
				e2->type = 5;
				e1->type = 2;
				e2->velocity_x = 0;
				ball->type = 3;
			}
			else if (e1->y < e2->y && e1->y + 0.3 > e2->y - 0.3)
			{
				e2->x = 11.5;
				e2->y = -1.5;
				e2->type = 5;
				e1->type = 2;
				e2->velocity_x = 0;
				ball->type = 3;
			}
		}
		else if (e1->x > e2->x && e1->x - 0.3 < e2->x + 0.3)
		{
			if (e1->y > e2->y && e1->y - 0.3 < e2->y + 0.3)
			{
				e2->x = 11.5;
				e2->y = -1.5;
				e2->type = 5;
				e1->type = 2;
				e2->velocity_x = 0;
				ball->type = 3;
			}
			else if (e1->y < e2->y && e1->y + 0.3 > e2->y - 0.3)
			{
				e2->x = 11.5;
				e2->y = -1.5;
				e2->type = 5;
				e1->type = 2;
				e2->velocity_x = 0;
				ball->type = 3;
			}
		}
	}
	if (e1->type == 0 && ball->type == 1)  //player1 picks up ball
	{
		if (e1->x < ball->x && e1->x + 0.3 > ball->x - 0.2)
		{
			if (e1->y > ball->y && e1->y - 0.3 < ball->y + 0.2)
			{
				ball->type = 3;
				e1->type = 2;
			}
			else if (e1->y < ball->y && e1->y + 0.3 > ball->y - 0.2)
			{
				ball->type = 3;
				e1->type = 2;
			}
		}
		else if (e1->x > ball->x && e1->x - 0.3 < ball->x + 0.2)
		{
			if (e1->y > ball->y && e1->y - 0.3 < ball->y + 0.2)
			{
				ball->type = 3;
				e1->type = 2;
			}
			else if (e1->y < ball->y && e1->y + 0.3 > ball->y - 0.2)
			{
				ball->type = 3;
				e1->type = 2;
			}
		}
	}
	if (e2->type == 5 && ball->type == 1)  //player2 picks up ball
	{
		if (e2->x < ball->x && e2->x + 0.3 > ball->x - 0.2)
		{
			if (e2->y > ball->y && e2->y - 0.3 < ball->y + 0.2)
			{
				ball->type = 4;
				e2->type = 2;
			}
			else if (e2->y < ball->y && e2->y + 0.3 > ball->y - 0.2)
			{
				ball->type = 4;
				e2->type = 2;
			}
		}
		else if (e2->x > ball->x && e2->x - 0.3 < ball->x + 0.2)
		{
			if (e2->y > ball->y && e2->y - 0.3 < ball->y + 0.2)
			{
				ball->type = 4;
				e2->type = 2;
			}
			else if (e2->y < ball->y && e2->y + 0.3 > ball->y - 0.2)
			{
				ball->type = 4;
				e2->type = 2;
			}
		}
	}
}

void checkGoal(Entity* p1, Entity* p2, Entity* ball)
{
	if (p1->type == 2 && p1->x > 11 && p1->y < -1.4 && p1->y >-3.4)//2 points player 1
	{
		ball->type = 1;
		ball->x = 6;
		ball->y = -1.0;
		ball->velocity_x = 0;
		ball->velocity_y = 0;
		p1->type = 0;
		p1->x = 1;
		p1Score += 2;
		p2->x = 11;
		p1->x = 1;
		p1->y = -2;
		p2->y = -2;
		p1->velocity_x = 0;
		p2->velocity_x = 0;
		Mix_PlayChannel(-1, score, 0);
		if (level == 3)
		{
			ball->y = -11.0;
		}
		if (p1Score >= 5)
			state = VICTORY1;
	}
	if (ball->type == 1 && ball->x > 11 && ball->y < -1.4 && ball->y >-3.4)//1 point player 1
	{
		ball->x = 6;
		ball->y = -1.0;
		ball->velocity_x = 0;
		ball->velocity_y = 0;
		p1Score += 1;
		p2->x = 11;
		p1->x = 1;
		p1->y = -2;
		p2->y = -2;
		p1->velocity_x = 0;
		p2->velocity_x = 0;
		Mix_PlayChannel(-1, score, 0);
		if (level == 3)
		{
			ball->y = -11.0;
		}
		if (p1Score >= 5)
			state = VICTORY1;
	}
	if (p2->type == 2 && p2->x < 1.4 && p2->y < -1.4 && p2->y >-3.4)//2 points player 2
	{
		ball->type = 1;
		ball->x = 6;
		ball->y = -1.0;
		ball->velocity_x = 0;
		ball->velocity_y = 0;
		p2->type = 5;
		p2Score += 2;
		p2->x = 11;
		p1->x = 1;
		p1->y = -2;
		p2->y = -2;
		p1->velocity_x = 0;
		p2->velocity_x = 0;
		Mix_PlayChannel(-1, score, 0);
		if (level == 3)
		{
			ball->y = -11.0;
		}

		if (p2Score >= 5)
			state = VICTORY2;
	}
	if (ball->type == 1 && ball->x < 1.4 && ball->y < -1.4 && ball->y >-3.4)//1 points player 1
	{
		ball->x = 6;
		ball->y = -1.0;
		ball->velocity_x = 0;
		ball->velocity_y = 0;
		p2->x = 11;
		p1->x = 1;
		p1->y = -2;
		p2->y = -2;
		p1->velocity_x = 0;
		p2->velocity_x = 0;
		p2Score += 1;
		Mix_PlayChannel(-1, score, 0);
		if (level == 3)
		{
			ball->y = -11.0;
		}
		if (p2Score >= 5)
			state = VICTORY2;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 450, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif
	SDL_Event event;
	glViewport(0, 0, 850, 500);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	GLuint playerTexture = LoadTexture("player.png");
	GLuint playerTexture2 = LoadTexture("player2.png");
	GLuint spriteSheetTexture = LoadTexture("sheet.png");
	GLuint ballTexture = LoadTexture("ball.png");
	GLuint font = LoadTexture("font1.png");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	SheetSprite playerSprite(&program, playerTexture, 0, 0, 1, 1, 0.6);
	SheetSprite player2Sprite(&program, playerTexture2, 0, 0, 1, 1, 0.6);
	SheetSprite ballSprite(&program, ballTexture, 0, 0, 1, 1, 0.3);

	Entity player(playerSprite, 1.0, -10.0, .7, .6, 0, 0, 0, 0, 1.3, 0);
	Entity player2(player2Sprite, 11.4, -10.0, .7, .6, 0, 0, 0, 0, 1.3,5);
	Entity ball(ballSprite, 6, -10.7, .5, .4, 0, 0, 0, 0, 0.5, 1);

	projectionMatrix.setOrthoProjection(-6.5f, 6.5f, -7.0f, 7.0f, -7.0f, 1.0f);
	program.setProjectionMatrix(projectionMatrix);

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Music *music;
	music = Mix_LoadMUS("music.wav");
	Mix_PlayMusic(music, -1);
	
	collision = Mix_LoadWAV("bounce.wav");
	score = Mix_LoadWAV("score.wav");

	glUseProgram(program.programID);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	/*
	particles.position.x = 2.0;
	particles.position.y = -5.0;
	particles.Reset();
	*/

	state = START;
	

	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		if (keys[SDL_SCANCODE_Q])
		{
			SDL_Quit();
			return 0;
		}

		string score1 = to_string(p1Score);
		string score2 = to_string(p2Score);

		switch (state){
		case START:
			//main menu
			modelMatrix.identity();
			modelMatrix.Translate(-4.0, 1, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "5 points to win", 0.7f, -.3f);
			modelMatrix.Translate(0, -1, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "1 for Basic", 0.7f, -.3f);
			modelMatrix.Translate(0, -1, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "2 for Bouncy", 0.7f, -.3f);
			modelMatrix.Translate(0, -1, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "3 for Ice", 0.7f, -.3f);
			modelMatrix.Translate(0, -1, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Q for Quit", 0.7f, -.3f);

			if (keys[SDL_SCANCODE_1])
			{
				//gets the starting tick
				lastFrameTicks = (float)SDL_GetTicks() / 1000.0f;
				readFile("level.txt");
				level = 1;
				state = GAME;
			}
			else if (keys[SDL_SCANCODE_2])
			{
				//gets the starting tick
				lastFrameTicks = (float)SDL_GetTicks() / 1000.0f;
				readFile("level2.txt");
				level = 2;
				player.y = -7.0;
				player2.y = -7.0;
				state = GAME;
			}
			else if (keys[SDL_SCANCODE_3])
			{
				//gets the starting tick
				lastFrameTicks = (float)SDL_GetTicks() / 1000.0f;
				readFile("level3.txt");
				level = 3;
				state = GAME;
			}
			break;

		case GAME:
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
			updateEntities(&player, &player2, &ball);
			checkPlayer(&player);
			checkPlayer(&player2);
			checkPlayer(&ball);

			checkEntity(&player, &player2, &ball);

			checkGoal(&player, &player2, &ball);

			if (ball.type == 3 && player.type == 2)
			{
				ball.x = player.x + 0.4;
				ball.y = player.y;
			}
			if (ball.type == 4 && player2.type == 2)
			{
				ball.x = player2.x - 0.4;
				ball.y = player2.y;
			}
			
			modelMatrix.identity();
			modelMatrix.Translate(0.75, -1.5, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, score1, 0.7f, -.3f);


			modelMatrix.Translate(11.0, 0, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, score2, 0.7f, -.3f);

			ball.Render(&program);
			player.Render(&program);
			player2.Render(&program);
			//particles.Render();

			viewMatrix.identity();
			viewMatrix.Translate(-6.65, 5.55, 0);
			program.setViewMatrix(viewMatrix);
			break;
		case VICTORY1:
			viewMatrix.identity();
			modelMatrix.identity();
			modelMatrix.Translate(-4.0, 1.0, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Player 1 Victory!", 0.7f, -.3f);
			modelMatrix.Translate(0, -1.0, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Space to restart", 0.7f, -.3f);
			if (keys[SDL_SCANCODE_SPACE])
			{
				state = START;
			}
			break;
		case VICTORY2:
			viewMatrix.identity();
			modelMatrix.identity();
			modelMatrix.Translate(-4.0, 1.0, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Player 2 Victory!", 0.7f, -.3f);
			modelMatrix.Translate(0, -1.0, 0);
			program.setModelMatrix(modelMatrix);
			DrawText(&program, font, "Space to restart", 0.7f, -.3f);
			if (keys[SDL_SCANCODE_SPACE])
			{
				state = START;
			}
			break;
		}
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
