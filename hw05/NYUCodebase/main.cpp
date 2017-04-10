/*
Xueyang (Sean) Wang
XW1154
04/06/2017
HW05
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

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

using namespace std;

#define FIXED_TIMESTEP 0.01666667f //FIXED_TIMESTEP FOR COLLISION
#define MAX_TIMESTEPS 6

enum State { MENU, GAME_MODE};

//GLOBAL VARIABLES TO SAVE PARAMETERS
SDL_Window* displayWindow;
ShaderProgram* program; 
GLuint GTexture;
GLuint TTexture;

//Corner positions (x,y)

//X,Y,Z
class Vector3 { 
public:
	Vector3() {}
	Vector3(float a, float b, float c) : x(a), y(b), z(c) {}
	float x;
	float y;
	float z;

	float distance() const {
		return sqrt(x*x + y*y + z*z);
	}

	Vector3 operator*(const Matrix& v) {
		Vector3 rotated;
		rotated.x = v.m[0][0] * x + v.m[1][0] * y + v.m[2][0] * z + v.m[3][0] * 1;
		rotated.y = v.m[0][1] * x + v.m[1][1] * y + v.m[2][1] * z + v.m[3][1] * 1;
		rotated.z = v.m[0][2] * x + v.m[1][2] * y + v.m[2][2] * z + v.m[3][2] * 1;
		return rotated;
	}

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


bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector3> &points1, const vector<Vector3> &points2, Vector3 &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	sort(e1Projected.begin(), e1Projected.end());
	sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(const Vector3 &p1, const Vector3 &p2) {
	return p1.distance() < p2.distance();
}

bool checkSATCollision(const vector<Vector3> &e1Points, const vector<Vector3> &e2Points, Vector3 &penetration) {
	vector<Vector3> penetrations;
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector3 penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector3 penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector3 e1Center;
	for (int i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector3 e2Center;
	for (int i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector3 ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}
	return true;
}

//Gravity, useful later
float lerp(float v0, float v1, float t) {
	return (float)(1.0 - t)*v0 + t*v1;
}

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

	Entity(float x, float y, float width, float height, float speed_X, float speed_Y, float rotate, float scale) {
		position.x = x;
		position.y = y;
		velocity.x = speed_X;
		velocity.y = speed_Y;
		dimension.t = y + height / 2;
		dimension.b = y - height / 2;
		dimension.l = x - width / 2;
		dimension.r = x + width / 2;
		//Taken out acceleration and friction since its gonna be in space
		//Newton's law constant force in one direction -> :)
		angle = rotate;
		unitScale = scale;

		//6 coordinates, rectangles
		coordinates.push_back(Vector3(-0.2, -0.1, 1));
		coordinates.push_back(Vector3(0.2, -0.1, 1));
		coordinates.push_back(Vector3(0.2, 0.1, 1));
		coordinates.push_back(Vector3(-0.2, -0.1, 1));
		coordinates.push_back(Vector3(0.2, 0.1, 1));
		coordinates.push_back(Vector3(-0.2, 0.1, 1));

	}

	bool collidedWith(const Entity &object) { //CONDITIONS, but we have SATCollisionDetection now
		return checkSATCollision(ToCompareToOriginal, object.ToCompareToOriginal, penetration);
		/*if (!(dimension.b > object.dimension.t ||
			dimension.t < object.dimension.b ||
			dimension.r < object.dimension.l ||
			dimension.l > object.dimension.r)) {
			return true;
			*/
	}

	void Render() { //MOVE MATRIX AND RENDER(DRAW)
		entityMotion.identity();
		entityMotion.Translate(position.x, position.y, 0);
		entityMotion.Rotate(angle * (3.1415926f / 180.0f));
		entityMotion.Scale(unitScale,unitScale,unitScale);
		program->setModelMatrix(entityMotion);
		sprite.Draw();
	}

	void To_update(float elapsed) {
		angle += 15.0f * elapsed;
		position.x += velocity.x * elapsed;
		dimension.l += velocity.x * elapsed;
		dimension.r += velocity.x * elapsed;

		//Can make then bounce back if necessary
		/*if (position.x >= 5.33f || position.x <= -5.33f) {
			velocity.x = velocity.x * -1.0f;
		}*/
		position.y += velocity.y * elapsed;
		dimension.b += velocity.y * elapsed;
		dimension.t += velocity.y * elapsed;
		/*if (position.y >= 3.0f || position.y <= -3.0f) {
			velocity.y = velocity.y * -1.0f;
		}*/
		for (int i = 0; i < ToCompareToOriginal.size(); i++) {
			ToCompareToOriginal[i] = coordinates[i] * entityMotion;
		}
	}

	void UpdateX(float elapsed) {
		//velocity.x += Acceleration.x * elapsed;
		position.x += velocity.x * elapsed;
		dimension.l += velocity.x * elapsed;
		dimension.r += velocity.x * elapsed;
	}

	void UpdateY(float elapsed) {
		//velocity.y += Acceleration.y * elapsed;
		position.y += velocity.y * elapsed;
		dimension.t += velocity.y * elapsed;
		dimension.b += velocity.y * elapsed;
	}

	SheetSprite sprite; //from sprite sheet

	Vector3 position;
	Vector4 dimension;
	Vector3 velocity;
	// Vector3 Acceleration; Constant velocity movement

	//NEW
	vector<Vector3> ToCompareToOriginal = vector<Vector3>(6);
	vector<Vector3> coordinates;

	Vector3 penetration;

	//Basic
	Matrix entityMotion;
	float angle;
	float unitScale;

	// bool isStatic; useful: dynamic and static
	// EntityType Type; To differentiate and to characterize entity, later use!

	bool collidedTop = false;
	bool collidedBottom = false;
	bool collidedLeft = false;
	bool collidedRight = false;
};

//position initiation, GLOBAL VARIABLES TO DEFINE MATRIX
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix viewMatrix;

//in-game ELEMENTS
float elapsed; //TIME_TRACK
SheetSprite nyan;
vector<Entity> Objects;

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

//Menu
void RenderMenu() {
	modelMatrix.identity();
	modelMatrix.Translate(-3.2f, 1.5f, 0.0f); //Top, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "Collision Testing", 0.4f, 0.0001f);

	modelMatrix.identity();
	modelMatrix.Translate(-4.2f, -1.0f, 0.0f); //Bottom, left to right
	program->setModelMatrix(modelMatrix);
	DrawText(program, "PRESS TO FLOW IN SPACE", 0.4f, 0.0001f);
}

//Game_Mode, possible for more levels
void RenderGame() {
	for (size_t i = 0; i < Objects.size(); i++) {
		Objects[i].Render();
	}
	viewMatrix.identity();
	program->setViewMatrix(viewMatrix);
};

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
	}
	SDL_GL_SwapWindow(displayWindow);
}

void UpdateObject(float elapsed) {
	for (size_t i = 0; i < Objects.size(); i++) { //LOOP THROUGH ALL OBJECTS
		Objects[i].To_update(elapsed);
		for (size_t j = i + 1; j < Objects.size(); j++) {
			if (Objects[i].collidedWith(Objects[j])) {

				Objects[j].position.x -= Objects[j].penetration.x / 2.0f;
				Objects[j].position.y -= Objects[j].penetration.y / 2.0f;

				Objects[i].position.x += Objects[i].penetration.x / 2.0f;
				Objects[i].position.y += Objects[i].penetration.y / 2.0f;


				//GOING OPPOSITE DIRECTION <- * ->
				Objects[i].velocity.x = Objects[i].velocity.x * -1.0f;
				Objects[i].velocity.y = Objects[i].velocity.y * -1.0f;

				Objects[j].velocity.x = Objects[j].velocity.x * -1.0f;
				Objects[j].velocity.y = Objects[j].velocity.y * -1.0f;

			}
		}
	}
}

void Update(float elapsed) { //CAN ADD DIFFERENT FUNCTIONS TO UPDATE MENU
	switch (state) {
	case MENU:
		break;
	case GAME_MODE:
		UpdateObject(elapsed);
		break;
	}
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Collision Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;

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

	//THREE COLLIDING OBJECTS
	Entity Object_a(-1.0f, 1.0f, 0.75f, 0.5f, 2.0f, 0.1f, 45.0f, 0.75f);
	Entity Object_b(1.5f, -1.0f, 0.75f, 0.5f, 2.5f, 0.25f, 90.0f, 2.0f);
	Entity Object_c(1.0f, 1.0f, 0.75f, 0.5f, -2.5f, 0.5f, 60.0f, 1.25f);

	Object_a.sprite = nyan;
	Object_b.sprite = nyan;
	Object_c.sprite = nyan;

	Objects.push_back(Object_a);
	Objects.push_back(Object_b);
	Objects.push_back(Object_c);

	bool done = false;
	float lastFrameTicks = 0.0f;

	while (!done) {
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
		Update(elapsed);
		Render();
	}
	SDL_Quit();
	return 0;
}