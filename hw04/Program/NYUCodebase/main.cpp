/*
Xueyang (Sean) Wang
XW1154
03/26/2017
HW04
CS3113
*/

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <iostream>
#include "ShaderProgram.h"
#include "Matrix.h"
#include <vector>
#include <algorithm>
#include <Windows.h>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

#define FIXED_TIMESTEP 0.0166666f //FIXED_TIMESTEP FOR COLLISION
#define MAX_TIMESTEPS 6
//TYPES
enum EntityType { isStatic, isDynamic };
enum State { MENU, GAME_MODE, INTERACT};

//GLOBAL VARIABLES TO SAVE PARAMETERS
SDL_Window* displayWindow;
ShaderProgram* program; 
GLuint GTexture; //General
GLuint TTexture; //Text
GLuint BTexture; //Blocks

//Game State
int state = 0;

//X,Y,Z
class Vector3 { 
public:
	Vector3() {}
	Vector3(float a, float b, float c) : x(a), y(b), z(c) {}
	float x;
	float y;
	float z;
}; 

//TOP, LEFT, BOTTOM, RIGHT -> COLLISION
class Vector4 { 
public:
	Vector4() {}
	Vector4(float a, float b, float c, float d) : t(a), b(b), l(c), r(d) {}
	float t;
	float b;
	float l;
	float r;
};

//Gravity, useful
float lerp(float v0, float v1, float t) {
	return (float)(1.0 - t)*v0 + t*v1;
}

//SPRITE INFORMATION AND DRAWING, TAKEN FROM CLASS SLIDE
class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(GLuint textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {};
	void Draw() {
		glBindTexture(GL_TEXTURE_2D, textureID);
		float texCoords[] = {
			u, v + height,
			u + width, v,
			u, v,
			u + width, v,
			u, v + height,
			u + width, v + height
		};
		float aspect = width / height;
		float vertices[] = {
			-0.5f * size * aspect, -0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, 0.5f * size,
			0.5f * size * aspect, 0.5f * size,
			-0.5f * size * aspect, -0.5f * size ,
			0.5f * size * aspect, -0.5f * size
		};
		glUseProgram(program->programID);

		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);

		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	};
	float size;
	GLuint textureID;
	float u;
	float v;
	float width;
	float height;
};

//ENTITY INFORMATION AND POSITION + MATRIX
class Entity {
public:
	Entity() {}

	Entity(float x, float y, float width, float height, float speed_X, float speed_Y) {
		position.x = x;
		position.y = y;
		velocity.x = speed_X;
		velocity.y = speed_Y;
		dimension.t = y + height / 2;
		dimension.b = y - height / 2;
		dimension.l = x - width / 2;
		dimension.r = x + width / 2;
		Acceleration.x = 0.0f;
		Acceleration.y = 0.0f;
		friction.x = 0.2f;
		friction.y = 0.0f;
		size.x = width;
		size.y = height;
	}

	bool collidedWith(const Entity &object) { //CONDITIONS
		if (!(dimension.b > object.dimension.t ||
			dimension.t < object.dimension.b ||
			dimension.r < object.dimension.l ||
			dimension.l > object.dimension.r)) {
			return true;
		}
		return false;
	}

	void Render() { //MOVE MATRIX AND RENDER(DRAW)
		entityMotion.identity();
		entityMotion.Translate(position.x, position.y, 0);
		program->setModelMatrix(entityMotion);
		sprite.Draw();
	}

	void Updated(float elapsed) {
		if (!isStatic) {
			velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
			velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);
			velocity.x += Acceleration.x * elapsed;
			velocity.y += Acceleration.y * elapsed;
			position.x += velocity.x * elapsed;
			dimension.l += velocity.x * elapsed;
			dimension.r += velocity.x * elapsed;
			position.y += velocity.y * elapsed;
			dimension.b += velocity.y * elapsed;
			dimension.t += velocity.y * elapsed;
		}
	}

	void UpdateX(float elapsed) {
		if (!isStatic) {
			velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
			if (velocity.x < 5.0f && velocity.x > -5.0f) {
				velocity.x += Acceleration.x * elapsed;
			}
			position.x += velocity.x * elapsed;
			dimension.l += velocity.x * elapsed;
			dimension.r += velocity.x * elapsed;
		}
	}
	void UpdateY(float elapsed) {
		if (!isStatic) {
			velocity.y += Acceleration.y * elapsed;
			position.y += velocity.y * elapsed;
			dimension.t += velocity.y * elapsed;
			dimension.b += velocity.y * elapsed;
		}
	}

	SheetSprite sprite; //from sprite sheet

	Vector3 size;
	Vector3 position;
	Vector4 dimension;
	Vector3 velocity;
	Vector3 Acceleration;
	Vector3 friction;
	Matrix entityMotion;
	// Vector3 acceleration; will be used for next homework

	bool isStatic = true; //useful: dynamic and static
	bool collidedTop = false;
	bool collidedBottom = false;
	bool collidedLeft = false;
	bool collidedRight = false;
};

//position initiation, GLOBAL VARIABLES TO DEFINE MATRIX
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

//in-game objects
float lastE_Bullet = 0.0; //ENEMY FIRING RATE
float elapsed; //TIME_TRACK

//IMAGE + OBJECT + COLLECTION OF OBJECTS
SheetSprite nyan;
SheetSprite love;
SheetSprite pickle;
SheetSprite block_one;
SheetSprite block_two;
Entity player;
Entity partner;
Entity stone;
vector<Entity> stones;

//input image
GLuint LoadTexture(const char *filePath) { 
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	
	if (image == NULL) { 
		cout << "Unable to load image. Make sure the path is correct\n";         
		assert(false); 
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image);
	return retTexture;
}

//ToDrawSprtiefromSheet, TAKEN FROM CLASS SLIDE
void DrawSpriteSheetSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0f / (float)spriteCountX;
	float spriteHeight = 1.0f / (float)spriteCountY;
	GLfloat texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth,
		v + spriteHeight };
	float vertices[] = {
		-0.5f, -0.5f,
		0.5f, 0.5f,
		-0.5f, 0.5f,
		0.5f, 0.5f,
		-0.5f, -0.5f,
		0.5f, -0.5f };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);

	glEnableVertexAttribArray(program->texCoordAttribute);
	glBindTexture(GL_TEXTURE_2D, GTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	//Clean
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

//ToDrawTextfromSheet, TAKEN FROM CLASS SLIDE
void DrawText(ShaderProgram *program, string text, float size, float spacing) {
	float texture_size = 1.0 / 16.0f;
	vector<float> vertexData;
	vector<float> texCoordData;

	for (size_t i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
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
	glBindTexture(GL_TEXTURE_2D, TTexture);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6); //6 coordinates * nnumbers of letters

	//Clean memory
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

//Menu
void RenderMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-3.5f, 1.5f, 0.0f); //Top, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "HOW TO FALL IN LOVE", 0.4f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-3.2f, -1.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "PRESS ENTER TO START", 0.35f, 0.0001f);
}

//Game_Mode, possible for more levels
void RenderGame() {
	player.Render();
	partner.Render();
	for (size_t i = 0; i < stones.size(); i++) {
		stones[i].Render();
	}
	viewMatrix.identity();
	if (player.position.x > 3.0f) {
		viewMatrix.Translate(-(player.position.x - 3.0f), 0.0f, 0.0f);
	}
	else if (player.position.x < -3.0f) {
		viewMatrix.Translate(-(player.position.x + 3.0f), 0.0f, 0.0f);
	}
	program->setViewMatrix(viewMatrix);
};

//Ending SCREEN
void RenderLove() {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	program->setModelMatrix(modelMatrix);
	DrawText(program, "YOU have found LOVE!", 0.35f, 0.0001f);
}


//To separate stages, CAN ADD LEVELS
void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	switch (state) {
	case MENU:
		RenderMenu();
		break;
	case GAME_MODE:
		RenderGame();
		break;
	case INTERACT:
		RenderLove();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

void UpdateGame(float elapsed) {
	float penetration;

	player.collidedTop = false;
	player.collidedBottom = false;
	player.collidedLeft = false;
	player.collidedRight = false;

	player.UpdateY(elapsed);
	for (size_t i = 0; i < stones.size(); i++) {
		if (player.collidedWith(stones[i])) {
			float y_distance = fabs(player.position.y - stones[i].position.y);
			penetration = fabs(y_distance - player.size.y / 2.0f - stones[i].size.y / 2.0f) + 0.00001f;
			if (player.position.y > stones[i].position.y) {
				player.position.y += penetration;
				player.dimension.b += penetration;
				player.dimension.t += penetration;
				player.collidedBottom = true;
				player.velocity.y = 0.0f;
			}
			else {
				player.position.y -= penetration;
				player.dimension.b -= penetration;
				player.dimension.t -= penetration;
				player.collidedTop = true;
				player.velocity.y = 0;
			}
			player.velocity.y = 0.0f;
		}
	}

	player.UpdateX(elapsed);
	for (size_t i = 0; i < stones.size(); i++) {
		if (player.collidedWith(stones[i])) {
			float x_distance = fabs(player.position.x - stones[i].position.x);
			penetration = fabs(x_distance - player.size.x / 2.0f - stones[i].size.x / 2.0f) + 0.00001f;
			if (player.position.x > stones[i].position.x) {
				player.position.x += penetration;
				player.dimension.l += penetration;
				player.dimension.r += penetration;
				player.collidedLeft = true;
				player.velocity.x = 0;
				player.Acceleration.x = 0.0f;
			}
			else {
				player.position.x -= penetration;
				player.dimension.l -= penetration;
				player.dimension.r -= penetration;
				player.collidedRight = true;
				player.velocity.x = 0;
				player.Acceleration.x = 0.0f;
			}
			player.velocity.x = 0.0f;
			player.Acceleration.x = 0.0f;
		}
	}

	partner.UpdateY(elapsed);
	for (size_t i = 0; i < stones.size(); i++) {
		if (partner.collidedWith(stones[i])) {
			float catch_love = 1.0f;
			if (partner.position.y > stones[i].position.y) {
				partner.position.y += catch_love;
				partner.dimension.b += catch_love;
				partner.dimension.t += catch_love;
				partner.collidedBottom = true;
				partner.velocity.y = 0.0f;
			}
			else {
				partner.position.y -= catch_love;
				partner.dimension.b -= catch_love;
				partner.dimension.t -= catch_love;
				partner.collidedTop = true;
				partner.velocity.y = 0;
			}
			player.velocity.y = 0.0f;
		}
	}

	if (player.collidedWith(partner)) {
		state = 2;
	}
}


void Update(float elapsed) { //CAN ADD DIFFERENT FUNCTIONS TO UPDATE
	switch (state)
	{
	case MENU:
		break;
	case GAME_MODE:
		UpdateGame(elapsed);
		break;
	case INTERACT:
		break;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Space Invader", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, 1280, 720);
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//window projection
	projectionMatrix.setOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);
	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	GTexture = LoadTexture("sprites.png");
	TTexture = LoadTexture("Text.png");
	BTexture = LoadTexture("platformIndustrial_sheet.png");

	nyan = SheetSprite(GTexture, 0.0f/1024.0f, 0.0f/512.0f, 347.0f / 1024.0f, 148.0f / 512.0f, 0.65f);
	love = SheetSprite(GTexture, 669.0f/1024.0f, 0.0f/512.0f, 198.0f / 1024.0f, 180.0f / 512.0f, 0.5f);
	pickle = SheetSprite(GTexture, 349.0f/1024.0f, 0.0f/512.0f, 318.0f / 1024.0f, 400.0f / 512.0f, 1.0f);
	block_one = SheetSprite(BTexture, 0.0f / 620.0f, 400.0f / 620.0f, 70.0f / 620.0f, 70.0f / 620.0f, 0.5f);
	block_two = SheetSprite(BTexture, 280.0f / 620.0f, 504.0f / 620.0f, 70.0f / 620.0f, 70.0f / 620.0f, 0.5f);

	player = Entity(-2.5f, 0.5f, 0.75f, 0.5f, 0.0f, 0.0f);
	player.sprite = nyan;
	player.isStatic = false;
	player.Acceleration.y = -1.5f;

	partner = Entity(7.0f, 0.5f, 0.4f, 0.8f, 0.0f, 0.0f);
	partner.sprite = pickle;
	partner.isStatic = false;
	partner.Acceleration.y = -1.5f;
	for (size_t i = 0; i < 50; i++) { //BASIC MAP
		stone = Entity(-5.33f + (i) * 0.47f, 0.0f, 0.5f, 0.5f, 0.8f, 0.08f);
		stone.sprite = block_one;
		stones.push_back(stone);
	}

	for (size_t i = 0; i < 5; i++) { //BASIC MAP
		stone = Entity(-3.5f + (i)* 0.47f, 2.0f, 0.5f, 0.5f, 0.8f, 0.08f);
		stone.sprite = block_two;
		stones.push_back(stone);
	}

	//units initiation
	float lastFrameTicks = 0.0f;
	bool done = false;

	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYUP) {
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT || event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					player.Acceleration.x = 0.0f;
				}
			}
			else if (event.type == SDL_KEYDOWN) { //MENU SWITCH TO GAME
				if (event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
					state = 1;
				}
			}
		}

		//Pulling input
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		//PLAYER MOVEMENT, CAN BE COMBINED INTO UPDATE BUT THIS IS MOST CURRENT
		if (keys[SDL_SCANCODE_LEFT]) {
			player.Acceleration.x = -1.5f;
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			player.Acceleration.x = 1.5;
		}
		else if (keys[SDL_SCANCODE_SPACE]) {
			// if (player.collidedBottom) {
			player.velocity.y = 1.5f;
			//}
		}

		//time track per frame
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}

		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			Update(FIXED_TIMESTEP);
		}
		Update(fixedElapsed);
		Render();
	}
	SDL_Quit();
	return 0;
}