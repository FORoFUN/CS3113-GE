/*
Xueyang (Sean) Wang
XW1154
02/21/2017
HW02
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

#include <Windows.h>

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

SDL_Window* displayWindow;

//Could've been entity class, but separated into two because of functionality
//Also, this code can be modified further to improve each class
class Paddle {
public:
	Paddle(float position, GLuint picture) : x(position), textureID(picture) {};

	//initial position
	float x;
	float y = 0.0f; //middle
	GLuint textureID;

	//object information
	float width = 2.0f;
	float height = 2.0f;

	float Left_side;
	float Right_side;
	float Top;
	float Bot;

	//movement
	float speed = 3.5f;
	float direction_y = 0.0f; //up and down
	const float direction_x = 0.0f; //constant
};

class Pong { //a similar class with collision and moving 45 angle
public:
	Pong(GLuint picture, float A) :textureID(picture), angle(A) {};

	//initial
	float angle;
	float x = cos(angle);
	float y = sin(angle);
	GLuint textureID;

	//object information
	float width = 0.5;
	float height = 0.5;

	//collision information
	float Left_side;
	float Right_side;
	float Top;
	float Bot;
};

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

void setup() {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, 640, 360);

}

//There are corner cases for Collision which will not taken into consideration
bool Collision(Pong ball, Paddle left, Paddle right) {
	//if any of these are false, we hit each other
	//Collision checked
	//right side to right paddle's left
	//left side to left paddle's right
	if (!(ball.Bot > right.Top ||
		ball.Top < right.Bot ||
		ball.Right_side < right.Left_side)) {
		return true;
	}

	if (!(ball.Bot > left.Top ||
		ball.Top < left.Bot ||
		ball.Left_side > left.Right_side)) {
		return true;
	}
	return false;
}

int main(int argc, char *argv[]) {
	//setup SDL, OpenGL, Shader
	setup();
	SDL_Event event;
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//loading images
	GLuint PaddleTexture = LoadTexture("Nyan-Cat-PNG-Image.png");
	GLuint PongTexture = LoadTexture("Heart.png");

	//position initiation
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	//window projection
	projectionMatrix.setOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);

	Paddle Left(-5.33f, PaddleTexture);
	Paddle Right(5.33f, PaddleTexture);
	Pong Center(PongTexture, 45);

	//units initiation
	float lastFrameTicks = 0.0f;
	int dir_y = 1;//pos, vector + direction
	int dir_x = 1;//pos, vector + direction
	int winning = 0;//keep track

	bool done = false;
	while (!done) {
		//time track per frame
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//input event
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}

		//background clear and color
		glClear(GL_COLOR_BUFFER_BIT);

		//A winning condition is met
		//Red for player 1; Blue for player 2
		if (winning == 1) {
			glClearColor(1.0, 0.0, 0.0, 1.0);
		}
		else if (winning == 2) {
			glClearColor(0.0, 1.0, 0.0, 1.0);
		}

		//Pulling input
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_DOWN]) {
			if (Right.direction_y > -3.0) {
				Right.direction_y -= elapsed * Right.speed; //Player 2 down
			}
		}
		else if (keys[SDL_SCANCODE_UP]) {
			if (Right.direction_y < 3.0) {
				Right.direction_y += elapsed * Right.speed; //Player 2 up
			}
		}

		if (keys[SDL_SCANCODE_S]) {
			if (Left.direction_y > -3.0) {
				Left.direction_y -= elapsed * Left.speed; //Player 1 down
			}
		}
		else if (keys[SDL_SCANCODE_W]) {
			if (Left.direction_y < 3.0) {
				Left.direction_y += elapsed * Left.speed; //Player 1 up
			}
		}

		//Matrix setting
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		//Left Paddle Red
		modelMatrix.identity();
		modelMatrix.Translate(0, Left.direction_y, 0);

		Left.Top = Left.direction_y + Left.height;
		Left.Bot = Left.direction_y - Left.height;
		Left.Left_side = Left.x;
		Left.Right_side = Left.x + Left.width;

		program.setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, Left.textureID);
		//coordinates top_right, top_left, bot_left, top_right, bot_left, bot_right
		float vertices[] = {
			Left.x + Left.width, Left.y + Left.height,
			Left.x,Left.y + Left.height,
			Left.x,Left.y - Left.height,
			Left.x + Left.width, Left.y + Left.height,
			Left.x,Left.y - Left.height,
			Left.x + Left.width, Left.y - Left.height
		};

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		//draw Left texture
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Right Paddle Blue
		modelMatrix.identity();
		modelMatrix.Translate(0, Right.direction_y, 0);

		Right.Top = Right.direction_y + Right.height;
		Right.Bot = Right.direction_y - Right.height;
		Right.Left_side = Right.x - Right.width;
		Right.Right_side = Right.x;

		program.setModelMatrix(modelMatrix);
		float vertices_A[] = {
			Right.x,Right.y + Right.height,
			Right.x - Right.width, Right.y + Right.height,
			Right.x - Right.width, Right.y - Right.height,
			Right.x,Right.y + Right.height,
			Right.x - Right.width, Right.y - Right.height,
			Right.x,Right.y - Right.height,

		};

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_A);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords_A[] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords_A);
		glEnableVertexAttribArray(program.texCoordAttribute);

		//draw Right texture
		glDrawArrays(GL_TRIANGLES, 0, 6);


		//Center, ball
		//reset identity so it does not move along previous Matrix movements
		modelMatrix.identity();
		Center.x += Center.x * elapsed * dir_x;
		Center.y += Center.y * elapsed * dir_y;

		Center.Top = Center.y + Center.height / 2;
		Center.Bot = Center.y - Center.height / 2;
		Center.Left_side = Center.x + Center.width / 2;
		Center.Right_side = Center.x + Center.width / 2;

		bool CollisionTest = false;
		CollisionTest = Collision(Center, Left, Right);

		if (Center.Top > 3) {
			dir_y *= -1;
		}
		else if (Center.Bot < -3) {
			dir_y *= -1;
		}
		else if (CollisionTest) {
			dir_x *= -1;
			CollisionTest = false;
		}
		else if (Center.Left_side < -5.33) {
			winning = 2;
		}
		else if (Center.Left_side > 5.33) {
			winning = 1;
		}

		modelMatrix.Translate(Center.x, Center.y, 1);
		program.setModelMatrix(modelMatrix);

		//blinding new texture
		glBindTexture(GL_TEXTURE_2D, Center.textureID);

		float vertices_B[] = {
			0.25, 0.25,
			-0.25,0.25,
			-0.25, -0.25,
			0.25, 0.25,
			-0.25, -0.25,
			0.25, -0.25
		};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_B);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords_B[] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords_B);
		glEnableVertexAttribArray(program.texCoordAttribute);

		//draw second texture
		glDrawArrays(GL_TRIANGLES, 0, 6);

		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}