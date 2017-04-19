/*
Xueyang (Sean) Wang
XW1154
04/18/2017
HW03//Modified HW06 with sound
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
#include <SDL_mixer.h>

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

#define FIXED_TIMESTEP 0.001f //FIXED_TIMESTEP FOR COLLISION
#define MAX_TIMESTEPS 6
#define MAX_BRATE 0.5 //LIMIT NUMBERS OF BULLETS BASED ON TIME

//TYPES
enum EntityType { ENEMY, BULLETS, PLAYER };
enum State { MENU, GAME_MODE, WIN, LOSE };

//GLOBAL VARIABLES TO SAVE PARAMETERS
SDL_Window* displayWindow;
ShaderProgram* program; 
GLuint GTexture;
GLuint TTexture;
Mix_Chunk *killed;
Mix_Chunk *pbullets;
Mix_Chunk *winning;
Mix_Chunk *lost;
Mix_Music *background;

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

//SPRITE INFORMATION AND DRAWING, TAKEN FROM CLASS SLIDE
class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(GLuint textureID, float u, float v, float width, float height, float size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {};
	void Draw() {
		glBindTexture(GL_TEXTURE_2D, GTexture);
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

	SheetSprite sprite; //from sprite sheet

	Vector3 position;
	Vector4 dimension;
	Vector3 velocity;
	Matrix entityMotion;
	// Vector3 acceleration; will be used for next homework

	bool isStatic; //useful: dynamic and static
	EntityType Type; //To differentiate and to characterize entity, later use!

	//Boolean, check collision, useful for next homework
	bool collidedT;
	bool collidedB;
	bool collidedL;
	bool collidedR;
};

//position initiation, GLOBAL VARIABLES TO DEFINE MATRIX
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

//in-game objects
bool Firebullet = false; //SPACE_BAR
float lastE_Bullet = 0.0; //ENEMY FIRING RATE
float elapsed; //TIME_TRACK

//IMAGE + OBJECT + COLLECTION OF OBJECTS
SheetSprite nyan;
SheetSprite love;
SheetSprite dislike;
SheetSprite pickle;
Entity player;
Entity enemy;
Entity bullet;
vector<Entity> enemies;
vector<Entity> E_bullets;
vector<Entity> P_bullets;

//Game State
int state = 0;

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

//Gravity, useful later
float lerp(float v0, float v1, float t) { 
	return (float)(1.0 - t)*v0 + t*v1;
}

//Menu
void RenderMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-2.5f, 1.5f, 0.0f); //Top, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "SPACE INVADERS", 0.4f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-3.7f, -1.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "PRESS ENTER TO START", 0.4f, 0.0001f);
}

//Game_Mode, possible for more levels
void RenderGame() {
	player.Render();
	for (size_t i = 0; i < enemies.size(); i++) {
		enemies[i].Render();
	}
	for (size_t i = 0; i < E_bullets.size(); i++) {
		E_bullets[i].Render();
	}
	for (size_t i = 0; i < P_bullets.size(); i++) {
		P_bullets[i].Render();
	}
};

//WIMMING SCREEN
void RenderWin() {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-3.8f, 0.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "YOU WIN! Nyan LOVES you!", 0.35f, 0.0001f);
}

//LOSING SCREEN
void RenderLose() {
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	modelMatrix.identity();
	modelMatrix.Translate(-4.25f, 0.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "YOU LOSE! NO LOVE from Nyan!", 0.32f, 0.0001f);
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
	case WIN:
		RenderWin();
		break;
	case LOSE:
		RenderLose();
		break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

void UpdateEnemy() {
	for (size_t i = 0; i < enemies.size(); i++) { //LOOP THROUGH A COLLECTION OF EMEMIES
		enemies[i].position.x += enemies[i].velocity.x * elapsed;
		enemies[i].dimension.l += enemies[i].velocity.x * elapsed;
		enemies[i].dimension.r += enemies[i].velocity.x * elapsed;

		enemies[i].position.y -= enemies[i].velocity.y * elapsed;
		enemies[i].dimension.t -= enemies[i].velocity.y * elapsed;
		enemies[i].dimension.b -= enemies[i].velocity.y * elapsed;

		if (enemies[i].dimension.r > 5.33f ||
			enemies[i].dimension.l < -5.33f) { //EDGES SWITCH SIDE
			for (size_t i = 0; i < enemies.size(); i++) { //UNIFORMALLY
				enemies[i].velocity.x = -enemies[i].velocity.x;
			}
		}
		if (enemies[i].dimension.b < -3.00f) { //REACHED BOTTOM
			state = 3;
		}
		else if (enemies[i].collidedWith(player)) { //REACHED PLAYER
			state = 3;
		}
	}
}

void UpdatePBullet() {
	P_bullets.push_back(Entity()); //PLACE HOLDER TO AVOID EMPTY ERASE
	if (Firebullet == true) { // ADD PLAYER BULLET
		bullet = Entity(player.position.x, player.position.y, 0.25f, 0.25f, 0.0f, 1.0f);
		bullet.sprite = love;
		P_bullets.push_back(bullet);
		Firebullet = false;
		Mix_PlayChannel(-1, pbullets, 0);
	}
	for (size_t i = 0; i < P_bullets.size(); ++i) { //MOVING
		P_bullets[i].position.y += P_bullets[i].velocity.y * elapsed;
		P_bullets[i].dimension.t += P_bullets[i].velocity.y * elapsed;
		P_bullets[i].dimension.b += P_bullets[i].velocity.y * elapsed;

		for (size_t j = 0; j < enemies.size(); j++) { //CHECKING COLLISION
			if (enemies[j].collidedWith(P_bullets[i])) {
				Mix_PlayChannel(-1, killed, 0);
				enemies.erase(enemies.begin() + j);
				P_bullets.erase(P_bullets.begin() + i); //YES!
			}
		}

	}
}
	
void UpdateEBullet() {
	lastE_Bullet += elapsed; //EMEMY BULLETS FIRING TIME
	if (lastE_Bullet > MAX_BRATE * 1.5) { //CAN BE CHANGE TO INCREASE DIFFICULTY
		int enemy_bullet = rand() % enemies.size(); //RANDOMLY GENERATE POSITION AND BULLET SHOOTER
		bullet = Entity(enemies[enemy_bullet].position.x, enemies[enemy_bullet].position.y, 0.25f, 0.25f, 0.0f, -1.0f);
		bullet.sprite = dislike;
		E_bullets.push_back(bullet);
		lastE_Bullet = 0;
	}
	for (size_t i = 0; i < E_bullets.size(); i++) { //UPDATE BULLETS POSITIONS
		E_bullets[i].position.y += E_bullets[i].velocity.y * elapsed;
		E_bullets[i].dimension.t += E_bullets[i].velocity.y * elapsed;
		E_bullets[i].dimension.b += E_bullets[i].velocity.y * elapsed;
		if (E_bullets[i].collidedWith(player)) { //IF HIT PLAYER
			state = 3;
			Mix_FreeMusic(background);
			Mix_PlayChannel(-1, lost, 0);
		}
		if (E_bullets[i].dimension.t < -3.00f) { //CLEAN TO SAVE MEMORY
			E_bullets.erase(E_bullets.begin() + i);
		}
	}
}

void UpdateGame(float elapsed) { //UPDATE ALL ENTITIES
	UpdateEnemy();
	UpdatePBullet();
	UpdateEBullet();

	if (enemies.size() == 0) { //WINNING CONDITION CHECK
		state = 2;
		Mix_FreeMusic(background);
		Mix_PlayChannel(-1, winning, 0);
	}
}

void Update(float elapsed) { //CAN ADD DIFFERENT FUNCTIONS TO UPDATE MENU;WINNING TO NEXT STAGE;LOSE TO RETRY
	switch (state)
	{
	case MENU:
		break;
	case GAME_MODE:
		UpdateGame(elapsed);
		break;
	case WIN:
		break;
	case LOSE:
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

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096); //OPEN AUDIO_SOUND
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GTexture = LoadTexture("sprites.png");
	TTexture = LoadTexture("Text.png");

	nyan = SheetSprite(GTexture, 0.0f/1024.0f, 0.0f/512.0f, 347.0f / 1024.0f, 148.0f / 512.0f, 0.6f);
	love = SheetSprite(GTexture, 669.0f/1024.0f, 0.0f/512.0f, 198.0f / 1024.0f, 180.0f / 512.0f, 0.5f);
	pickle = SheetSprite(GTexture, 349.0f/1024.0f, 0.0f/512.0f, 318.0f / 1024.0f, 400.0f / 512.0f, 0.8f);
	dislike = SheetSprite(GTexture, 0.0f / 1024.0f, 150.0f / 512.0f, 237.0f / 1024.0f, 223.0f / 512.0f, 0.6f);

	player = Entity(0.0f, -2.5f, 0.75f, 0.5f, 1.5f, 0.0f);
	player.sprite = nyan;

	for (size_t i = 0; i < 30; i++) { //CAN BE CHANGED TO INCREASE DIFFICULTY + # OF ENEMIES
		enemy = Entity(-2.0f + (i % 10) * 0.4f, 3.0f - i / 10 * 0.7f, 0.5f, 0.5f, 0.8f, 0.08f);
		enemy.sprite = pickle;
		enemies.push_back(enemy);
	}

	//units initiation
	float lastFrameTicks = 0.0f;
	float lastP_Bullet = 0.0f;
	float lastE_Bullet = 0.0f;
	bool done = false;

	killed = Mix_LoadWAV("kill.wav");
	pbullets = Mix_LoadWAV("pshoot.wav");
	winning = Mix_LoadWAV("winning.wav");
	lost = Mix_LoadWAV("lost.wav");
	background = Mix_LoadMUS("BGM.mp3");
	Mix_PlayMusic(background, -1);

	while (!done) {
		/*velocity_x = lerp(velocity_x, 0.0f, elapsed * friction_x);
		velocity_y = lerp(velocity_y, 0.0f, elapsed * friction_y);
		velocity_x += acceleration_x * elapsed;
		velocity_y += acceleration_y * elapsed;
		x += velocity_x * elapsed;
		y += velocity_y * elapsed
		For next homework, could implement with 0/1 input now, but too lazy
		*/

		//input event
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
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
			if (player.dimension.l > -5.33) {
				player.position.x -= player.velocity.x * elapsed;
				player.dimension.l -= player.velocity.x * elapsed;
				player.dimension.r -= player.velocity.x * elapsed;
			}
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			if (player.dimension.r < 5.33) {
				player.position.x += player.velocity.x * elapsed;
				player.dimension.l += player.velocity.x * elapsed;
				player.dimension.r += player.velocity.x * elapsed;
			}
		}

		//CHANGE FIREBULLET STATE
		if (keys[SDL_SCANCODE_SPACE]) {
			if (state == 1) {
				if (lastP_Bullet > MAX_BRATE) {
					Firebullet = true;
					lastP_Bullet = 0;
				}
			}
		}

		//time track per frame
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		lastP_Bullet += elapsed;

		float fixedElapsed = elapsed;
		if (fixedElapsed > FIXED_TIMESTEP * MAX_TIMESTEPS) {
			fixedElapsed = FIXED_TIMESTEP * MAX_TIMESTEPS;
		}

		while (fixedElapsed >= FIXED_TIMESTEP) {
			fixedElapsed -= FIXED_TIMESTEP;
			//Update(FIXED_TIMESTEP); NOT USED FOR NOW, FIXEDFRAME FOR NEXT HOMEWORK
		}
		Update(elapsed);
		Render();
	}
	Mix_FreeChunk(pbullets);
	Mix_FreeChunk(killed);
	Mix_FreeChunk(winning);
	Mix_FreeChunk(lost);
	Mix_FreeMusic(background);
	SDL_Quit();
	return 0;
}