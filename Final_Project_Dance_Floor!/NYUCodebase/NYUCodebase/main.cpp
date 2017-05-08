/*
Xueyang (Sean) Wang
XW1154
04/25/2017
FINAL PROJECT
CS3113

All rights reserved to Xueyang (Sean) Wang

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
#include <SDL_mixer.h>
// #include "kiss_fft.h"

using namespace std;

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define FIXED_TIMESTEP 0.0166666f //FIXED_TIMESTEP FOR COLLISION
#define MAX_TIMESTEPS 6
#define CONST_FALLING 1.0f //constantly speed of falling
#define MAX_BRATE 1.5f //Generate boxes
#define MAX_HEALTH_C 5.0f //Heath of character
#define MAX_HEALTH_E 400.0f //Heath of ememy

SDL_Window* displayWindow;
ShaderProgram* program;
ShaderProgram* untextured_program;
int color_uniform;

//Texture inforamtion
GLuint TTexture; //Text
vector<GLuint> ATexture; //Arrows, UP0, DOWN1, LEFT2, RIGHT3
GLuint Hitbox; //Contact
GLuint Player_one;
GLuint Player_two;
GLuint Enemy;
GLuint Background;

//Music informaiton
Mix_Music *challenge;
Mix_Chunk *combo;
Mix_Chunk *miss;
Mix_Chunk *lost;
Mix_Chunk *won;
Mix_Chunk *click;

//position initiation, GLOBAL VARIABLES TO DEFINE MATRIX
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

//Animation informaiton
// render which animation to run;
bool check_hit_one = false;
bool check_hit_two = false;

const int runAnimation_Reg[] = {0,1,2,3,0,0};
const int runAnimation_Hit[] = {0,18,19,20,21,23};
const int numFrames = 6;
float animationElapsed = 0.0f;
float framesPerSecond = 12.0f;
int currentIndex = 0;
float animationTime = 0.0f;


float elapsed; //TIME_TRACK
float lastBox;
int hit = 0;
int not_hit = 0;
int const_hit = 0;
int state = 0;
/*
0 = MENU
1 = GAMING
2 = WINNING // NEXT LEVEL
3 = LOSING
4 = COMPLETE 3 levels
*/

int Level = 1; //Load differnet music

//import image
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

//ToDrawSprtiefromSheet
void DrawSpriteSheetSprite(GLuint GTexture, int index, int spriteCountX, int spriteCountY) {
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

float lerp(float from, float to, float time) {
	float tVal;
	if (time > 0.5) {
		float oneMinusT = 1.0f - ((0.5f - time)*-2.0f);
		tVal = 1.0f - ((oneMinusT * oneMinusT * oneMinusT * oneMinusT *
			oneMinusT) * 0.5f);
	}
	else {
		time *= 2.0;
		tVal = (time*time*time*time*time) / 2.0;
	}
	return (1.0f - tVal)*from + tVal*to;
}

float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
	float retVal = dstMin + ((value - srcMin) / (srcMax - srcMin) * (dstMax - dstMin));
	if (retVal < dstMin) {
		retVal = dstMin;
	}
	if (retVal > dstMax) {
		retVal = dstMax;
	}
	return retVal;
}

//ENTITY INFORMATION AND POSITION + MATRIX
class Entity {
public:
	Entity() {}
	Entity(float x, float y, float spriteU, float spriteV, float spriteWidth, float spriteHeight, GLuint spriteTexture, float ratio) {
		position[0] = x;
		position[1] = y;
		entityMatrix.identity();
		entityMatrix.Translate(x, y, 0);
		size = ratio;
		dimension[0] = y + 0.05f * size * 2; //TOP
		dimension[1] = y - 0.05f * size * 2; //BOT
		dimension[2] = x - 0.05f * size * 2; //LEFT
		dimension[3] = x + 0.05f * size * 2; //RIGHT

		u = spriteU;
		v = spriteV;
		width = spriteWidth;
		height = spriteHeight;
		picture = spriteTexture;
	}

	bool collidedWith(const Entity &object) { //CONDITIONS
		if (!isStatic && !object.isStatic) {
			if (!(dimension[1] > object.dimension[0] ||
				dimension[0] < object.dimension[1] ||
				dimension[3]< object.dimension[2] ||
				dimension[2] > object.dimension[3])) {
				return true;
			}
		}
		return false;
	}

	void Render() { //IDENTITY MATRIX AND RENDER(DRAW)
		entityMatrix.identity();
		entityMatrix.Translate(position[0], position[1], 0.0f);
		entityMatrix.Scale(scale,scale,1.0f);
		program->setModelMatrix(entityMatrix);

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

		glBindTexture(GL_TEXTURE_2D, picture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	void Time_to_hit(float animationV) {
		scale = lerp(1, 0, animationV);
	}

	void animationRender() {
		entityMatrix.identity();
		entityMatrix.Translate(position[0], position[1], 0);
		entityMatrix.Scale(size, size, 1.0f);
		program->setModelMatrix(entityMatrix);

		if (check_hit_one) {
			DrawSpriteSheetSprite(picture, runAnimation_Hit[currentIndex], 9, 3);
			return;
		}
		else {
			DrawSpriteSheetSprite(picture, runAnimation_Reg[currentIndex], 9, 3);
			return;
		}

		if (check_hit_two) {
			DrawSpriteSheetSprite(picture, runAnimation_Hit[currentIndex], 9, 3);
			return;
		}
		else {
			DrawSpriteSheetSprite(picture, runAnimation_Reg[currentIndex], 9, 3);
			return;
		}
	}

	void Update_y(float time) {
		position[1] -= CONST_FALLING*time;
		dimension[0] -= CONST_FALLING*time;
		dimension[1] -= CONST_FALLING*time;
	}

	GLuint picture; //from sprite sheet

	float position[2];
	float dimension[4];

	Matrix entityMatrix;

	bool isStatic = true; //useful: dynamic and static
	bool animating = false; //Entity animation

	bool collidedT = false;
	bool collidedB = false;
	bool collidedL = false;
	bool collidedR = false;

	float u;
	float v;
	float width;
	float height;
	float size;
	float scale = 1;
};

Entity player_L;
Entity player_R;
Entity background;
Entity enemy;
vector<Entity> music_elements_P1;
vector<Entity> music_elements_P2;
vector<Entity> collisions_P1;
vector<Entity> collisions_P2;

//Manually generated musicbox, potential FFT algorithms use to expand
void generatebox() {
	int randNum_P1 = (rand() % 4);
	int randNum_P2 = (rand() % 4);

	Entity element_P1(-5.5f + randNum_P1*1.0f, 2.2f, 0.0f, 0.0f, 1.0f, 1.0f, ATexture[randNum_P1], 0.8f);
	element_P1.isStatic = false;
	music_elements_P1.push_back(element_P1);

	Entity element_P2(-1.5f + randNum_P2*1.0f, 2.2f, 0.0f, 0.0f, 1.0f, 1.0f, ATexture[randNum_P2], 0.8f);
	element_P2.isStatic = false;
	music_elements_P2.push_back(element_P2);
}

void healthbar_C() {
	modelMatrix.identity();
	modelMatrix.Translate(-5.8f, 3.0f, 0.0f);
	float damage = 1.0f - (not_hit / MAX_HEALTH_C);
	modelMatrix.Scale(4.5f * damage, 0.8f, 1.0f);

	untextured_program->setModelMatrix(modelMatrix);

	float vert[] = {
		0.0f, -0.5f,
		1.0f, 0.5f,
		0.0f, 0.5f,
		1.0f, 0.5f,
		0.0f, -0.5f,
		1.0f, -0.5f
	};

	glUseProgram(untextured_program->programID);

	glUniform4f(color_uniform, 1.0f, 0.0f, 0.0f, (MAX_HEALTH_C - not_hit) / MAX_HEALTH_C);

	glEnableVertexAttribArray(untextured_program->positionAttribute);
	glVertexAttribPointer(untextured_program->positionAttribute, 2, GL_FLOAT, false, 0, vert);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(untextured_program->positionAttribute);
}

void healthbar_E() {
	modelMatrix.identity();
	modelMatrix.Translate(1.3f, 3.0f, 0.0f);
	float damage = 1.0f - (hit / MAX_HEALTH_E);
	modelMatrix.Scale(4.5f * damage, 0.8f, 1.0f);

	untextured_program->setModelMatrix(modelMatrix);

	float vert[] = {
		0.0f, -0.5f,
		1.0f, 0.5f,
		0.0f, 0.5f,
		1.0f, 0.5f,
		0.0f, -0.5f,
		1.0f, -0.5f
	};

	glUseProgram(untextured_program->programID);

	glUniform4f(color_uniform, 1.0f, 0.0f, 0.0f, (MAX_HEALTH_E - hit) / MAX_HEALTH_E);

	glEnableVertexAttribArray(untextured_program->positionAttribute);
	glVertexAttribPointer(untextured_program->positionAttribute, 2, GL_FLOAT, false, 0, vert);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(untextured_program->positionAttribute);
}

void UpdateBoxes(float current_time) {

	for (size_t i = 0; i < music_elements_P1.size(); i++) {
		music_elements_P1[i].Update_y(current_time);
		if (music_elements_P1[i].dimension[1] < -3.0f) {
			Mix_PlayChannel(-1, miss, 0);
			check_hit_two = false; 	//Animation reset
			not_hit += 1;
			const_hit = 0;
			music_elements_P1.erase(music_elements_P1.begin() + i);
		}
		else if (music_elements_P1[i].dimension[1] < -1.6f) {
			if (music_elements_P1[i].animating == false) {
				music_elements_P1[i].animating = true;
				animationTime = 0.0f;
			}
			else {
				animationTime += current_time;
			}
			float animationValue = mapValue(animationTime, 0.0,
				1.5, 0.0, 1.0);
			music_elements_P1[i].Time_to_hit(animationValue);
		}
	}

	for (size_t i = 0; i < music_elements_P2.size(); i++) {
		music_elements_P2[i].Update_y(current_time);
		if (music_elements_P2[i].dimension[1] < -3.0f) {
			Mix_PlayChannel(-1, miss, 0);
			check_hit_one = false; 	//Animation reset
			not_hit += 1;
			const_hit = 0;
			music_elements_P2.erase(music_elements_P2.begin() + i);
		}
		else if (music_elements_P2[i].dimension[1] < -1.6f) {
			if (music_elements_P2[i].animating == false) {
				music_elements_P2[i].animating = true;
				animationTime = 0.0f;
			}
			else {
				animationTime += current_time;
			}
			float animationValue = mapValue(animationTime, 0.0,
				1.5, 0.0, 1.0);
			music_elements_P2[i].Time_to_hit(animationValue);
		}
	}

	//collision reset
	for (Entity& P1 : collisions_P1) {
		P1.isStatic = true;
	}
	for (Entity& P2 : collisions_P2) {
		P2.isStatic = true;
	}
}

void Clean() {
	Mix_HaltMusic();
	// Mix_FreeMusic(challenge); Some bugs!
	hit = 0;
	not_hit = 0;
	const_hit = 0;
	music_elements_P1.clear();
	music_elements_P2.clear();
}

void CheckCondition() {
	if (not_hit >= 5)
	{
		Mix_PlayChannel(-1, lost, 0);
		Clean();
		state = 3;
	}
	else if (hit >= 400) {
		Mix_PlayChannel(-1, won, 0);
		Clean();
		state = 2;
	}
}

void UpdateGame(float fixed_timeframe) {
	UpdateBoxes(fixed_timeframe);
	CheckCondition();
}


void Update(float elapsed) {
	switch (state)
	{
	case 0:
		Clean();
		break;
	case 1:
		UpdateGame(elapsed);
		break;
	case 2:
		Clean();
		break;
	case 3:
		Clean();
		break;
	case 4:
		Clean();
		break;
	}
}

//MENU
void RenderMenu() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-2.5f, 1.5f, 0.0f); //Top, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "DANCE BATTLE!", 0.4f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-3.7f, -1.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "PRESS ENTER TO START", 0.4f, 0.0001f);
}

//WINNING SCREEN
void RenderWin() {
	glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-3.75f, 0.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "YOU WIN! ENTER TO NEXT LEVEL", 0.40f, -0.1f);
	//CHECK FOR EXIT change state then exit
	//CHECK FOR NEXT LEVEL
}

//LOSING SCREEN
void RenderLose() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-2.25f, 1.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "YOU ARE DEFEATED", 0.32f, 0.0001f);

	modelMatrix.Translate(-1.75f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "ESC TO EXIT, ENTER TO RETRY", 0.32f, 0.0001f);
}

void RenderFinish() {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-5.3f, 0.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "YOU WIN! TRY YOUR OWN MUSIC! PRESS TO EXIT", 0.36f, -0.1f);
}

//Score Tracking
void ScoreTrack() {
	modelMatrix.identity();

	modelMatrix.Translate(2.50f, 1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "HIT:" + to_string(hit), 1.0f, -0.7f);

	modelMatrix.Translate(-0.25f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "COMBO:" + to_string(const_hit), 1.0f, -0.5f);

	modelMatrix.Translate(0.25f, -1.0f, 0.0f);
	program->setModelMatrix(modelMatrix);
	DrawText(program, "MISS:" + to_string(not_hit), 1.0f, -0.7f);
}

void RenderGame() {
	//drawbackground
	background.Render();

	//animation cycle
	player_L.animationRender();
	player_R.animationRender();
	enemy.animationRender();

	//hit box render
	for (Entity x : collisions_P1) {
		x.Render();
	}
	for (Entity y : collisions_P2) {
		y.Render();
	}

	//FALLING ELEMENTS
	for (Entity box : music_elements_P1) {
		box.Render();
	}
	for (Entity element : music_elements_P2) {
		element.Render();
	}

	//COMBO DRAWTEXT
	ScoreTrack();
		
	//Health update
	healthbar_C();
	healthbar_E();

	//juice
}

void Render() {
	glClear(GL_COLOR_BUFFER_BIT);

	switch (state) {
	case 0:
		RenderMenu();
		break;
	case 1:
		RenderGame();
		break;
	case 2:
		RenderWin();
		break;
	case 3:
		RenderLose();
		break;
	case 4:
		RenderFinish();
	}

	SDL_GL_SwapWindow(displayWindow);
}

//Play Music
void play() {
	switch (Level)
	{
	case 0:
		break;
	case 1:
		challenge = Mix_LoadMUS("level_1.mp3");
		break;
	case 2:
		challenge = Mix_LoadMUS("level_2.mp3");
		break;
	case 3:
		challenge = Mix_LoadMUS("level_3.mp3");
		break;
	}

	Mix_PlayMusic(challenge, -1);
}

void collision_checked_pressed() {
	for (size_t i = 0; i < music_elements_P1.size(); i++) {
		for (size_t j = 0; j < collisions_P1.size(); j++) {
			if (collisions_P1[j].collidedWith(music_elements_P1[i])) {
				check_hit_one = true;
				Mix_PlayChannel(-1, combo, 0);
				hit += 1;
				const_hit += 1;
				if (not_hit > 0) {
					not_hit -= 1;
				}
				music_elements_P1.erase(music_elements_P1.begin() + i);
			}
		}
	}

	for (size_t i = 0; i < music_elements_P2.size(); i++) {
		for (size_t j = 0; j < collisions_P2.size(); j++) {
			if (collisions_P2[j].collidedWith(music_elements_P2[i])) {
				check_hit_two = true;
				Mix_PlayChannel(-1, combo, 0);
				hit += 1;
				const_hit += 1;
				if (not_hit > 0) {
					not_hit -= 1;
				}
				music_elements_P2.erase(music_elements_P2.begin() + i);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("DANCE FLOOR!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, 1280, 720);
	program = new ShaderProgram(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	untextured_program = new ShaderProgram(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl"); //second shader
	color_uniform = glGetUniformLocation(untextured_program->programID, "color");

	projectionMatrix.setOrthoProjection(-6.0f, 6.0f, -4.0f, 4.0f, -1.0f, 1.0f);

	//1 + 1 = 2 shaders
	untextured_program->setModelMatrix(modelMatrix);
	untextured_program->setProjectionMatrix(projectionMatrix);
	untextured_program->setViewMatrix(viewMatrix);

	program->setModelMatrix(modelMatrix);
	program->setProjectionMatrix(projectionMatrix);
	program->setViewMatrix(viewMatrix);

	//Texture information
	Background = LoadTexture("background.jpg");
	TTexture = LoadTexture("font.png");
	Hitbox = LoadTexture("circle_hitbox.png");

	ATexture.push_back(LoadTexture("up_key.png"));
	ATexture.push_back(LoadTexture("down_key.png"));
	ATexture.push_back(LoadTexture("left_key.png"));
	ATexture.push_back(LoadTexture("right_key.png"));

	Player_one = LoadTexture("player_tilesheet.png");
	Player_two = LoadTexture("adventurer_tilesheet.png");
	Enemy = LoadTexture("zombie_tilesheet.png");


	//hit box information
	for (size_t i = 0; i < 4; i++) {
		collisions_P1.push_back(Entity(-5.5f + i*1.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, Hitbox, 0.8f));
	}
	for (size_t i = 0; i < 4; i++) {
		collisions_P2.push_back(Entity(-1.5f + i*1.0f, -2.0f, 0.0f, 0.0f, 1.0f, 1.0f, Hitbox, 0.8f));
	}

	//Player information
	background = Entity(-6.0f, 4.0f, 0.0f, 0.0f, 12.0f, 8.0f, Background, 100.0f);
	background.isStatic = true;

	player_L = Entity(-4.0f, -3.0f, 0.0f, 0.0f, 1.0f, 1.0f, Player_one, 1.5f);
	player_L.isStatic = true;
	
	player_R = Entity(0.0f, -3.0f, 0.0f, 0.0f, 1.0f, 1.0f, Player_two, 1.5f);
	player_R.isStatic = true;
	
	enemy = Entity(4.0f, -3.0f, 0.0f, 0.0f, 1.0f, 1.0f, Enemy, 2.0f);
	enemy.isStatic = true;

	//Audio information
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	combo = Mix_LoadWAV("pshoot.wav");
	miss = Mix_LoadWAV("miss.wav");
	lost = Mix_LoadWAV("lost.wav");
	won = Mix_LoadWAV("winning.wav");
	click = Mix_LoadWAV("click.wav");

	SDL_Event event;

	float lastFrameTicks = 0.0f;
	bool done = false;

	//LIT UP THE BUTTOM WHEN PRESSED, POTENTIAL

	while (!done) {
		while (SDL_PollEvent(&event)) {
			//check for key down event
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_RETURN && state == 0) {
					play();
					state = 1;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && state == 0) {
					done = true;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && state == 1) {
					Clean();
					state = 0;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RETURN && state == 2) {
					if (Level == 3) {
						state = 4;
					}
					else {
						Level += 1;
						play();
						state = 1;
					}
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && state == 2) {
					state = 0;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RETURN && state == 3) {
					play();
					state = 1;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE && state == 3) {
					state = 0;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_RETURN && state == 4) {
					done = true;
				}

				//check collision only when pressed
				if (event.key.keysym.scancode == SDL_SCANCODE_UP && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P2[0].isStatic = false;
					collision_checked_pressed();
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_DOWN && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P2[1].isStatic = false;
					collision_checked_pressed();
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_LEFT && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P2[2].isStatic = false;
					collision_checked_pressed();
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P2[3].isStatic = false;
					collision_checked_pressed();
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_W && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P1[0].isStatic = false;
					collision_checked_pressed();
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_S && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P1[1].isStatic = false;
					collision_checked_pressed();
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_A && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P1[2].isStatic = false;
					collision_checked_pressed();
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_D && state == 1) {
					Mix_PlayChannel(-1, click, 0);
					collisions_P1[3].isStatic = false;
					collision_checked_pressed();
				}
			}
		}

		//time track per frame
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		lastBox += elapsed;

		if (lastBox > MAX_BRATE / Level && state == 1) {
			generatebox();
			lastBox = 0.0f;
		}

		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}

		animationElapsed += elapsed;
		if (animationElapsed > 1.0 / framesPerSecond) {
			currentIndex++;
			animationElapsed = 0.0;
			if (currentIndex > numFrames - 1) {
				currentIndex = 0;
			}
		}

		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			Update(FIXED_TIMESTEP); //Do not have to use this
		}
		Update(fixedElapsed);
		Render();
	}

	Mix_FreeChunk(combo);
	Mix_FreeChunk(miss);
	Mix_FreeChunk(lost);
	Mix_FreeChunk(won);
	Mix_FreeMusic(challenge);

	SDL_Quit();
	return 0;
}
