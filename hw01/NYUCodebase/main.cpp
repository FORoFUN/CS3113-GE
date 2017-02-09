/*
Xueyang (Sean) Wang
XW1154
02/07/2017
HW01
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

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

SDL_Window* displayWindow;

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


int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//loading images
	GLuint catTexture = LoadTexture("Nyan-Cat-PNG-Image.png");
	GLuint midTexture = LoadTexture("top.png");
	GLuint leftTexture = LoadTexture("left.png");
	GLuint rightTexture = LoadTexture("bot_right.png");

	//position initiation
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	//window projection
	projectionMatrix.setOrthoProjection(-5.33f, 5.33f, -3.0f, 3.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);
	
	//units initiation
	float lastFrameTicks = 0.0f;
	float angle = 0.0f;
	float directionX = cos(45.0 * (3.1415926 / 180.0)); // 
	float directionY = sin(-45.0 * (3.1415926 / 180.0));//
	float positionX = 0.0f;
	float positionY = 0.0f;

	//frame refreashing rate
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
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_UP) {
					positionY += 0.25f;
				}
				else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) {
					positionY -= 0.25f;
				}
			}
		}
		
		//background clear and color
		glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//Matrix setting
		modelMatrix.identity();
		modelMatrix.Scale(2.5f, 2.5f, 1.0f);

		//Pulling input
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_LEFT]) {
			positionX -= elapsed * 1.0; //direction applies
		}
		else if (keys[SDL_SCANCODE_RIGHT]) {
			positionX += elapsed * 1.0; //direction applies
		}

		//first animated tecture, bottom most layer? seems like it

		angle += 270 * elapsed  * (3.1415926 / 180);

		modelMatrix.Translate(positionX, positionY, 0.0f);
		modelMatrix.Rotate(angle);
		
		//send to rendering
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		//blinding the texture(images)
		glBindTexture(GL_TEXTURE_2D, catTexture);

		//coordinates top_right, top_left, bot_left, top_right, bot_left, bot_right
		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		//draw first texture
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//second non-animated tecture
		//reset identity so it does not move along previous Matrix movements
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);

		//blinding new texture
		glBindTexture(GL_TEXTURE_2D, midTexture);

		float vertices_A[] = { 2.0, 3.0, -2.0, 3.0, -2.0, 1.0, 2.0, 3.0, -2.0, 1.0, 2.0, 1.0};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_A);
		glEnableVertexAttribArray(program.positionAttribute);

		//draw second texture
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//third non-animated texture
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, rightTexture);
		//repeat above

		float vertices_B[] = { 3.0, 0.0, 1.0, 0.0, 1.0, -2.0, 3.0, 0.0, 1.0, -2.0, 3.0, -2.0};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_B);
		glEnableVertexAttribArray(program.positionAttribute);

		//draw third texture
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		//forth non-animated texture for practice
		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);
		glBindTexture(GL_TEXTURE_2D, leftTexture);
		//changes can be added to lines above to make texture animated

		float vertices_C[] = { -1.0, 1.0, -3.0, 1.0, -3.0, -1.0, -1.0, 1.0, -3.0, -1.0, -1.0, -1.0};
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_C);
		glEnableVertexAttribArray(program.positionAttribute);

		//draw forth texture
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//free up the memory(space)
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		//swap to the next frame? need to ask
		//maybe refresh frame
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}