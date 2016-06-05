#pragma once
#pragma once
#ifndef SCENERY_H
#define SCENERY_H

#include "Window.h"
#include "Definitions.h"
#include "Terrain.h"
#include "Water.h"

class Scenery
{
private:
	//Contain the width and height of the scenery.
	int width;
	int height;
	glm::vec2 boundaries;
	//Access elements from the scenery class.
	std::vector<Terrain*> terrains;
	std::vector<Water*> waters;
	//Terrains
	void generateTerrains();
	void stitchTerrains();
	int getTerrain(glm::vec3 position);
	//Water
	void generateWater();

public:
	//Constructor methods.
	Scenery(int width, int height);
	~Scenery();

	float getHeight(glm::vec3 position);
	glm::vec2 getBounds();

	void toggleDrawMode();

	void draw_terrain(GLuint shaderProgram);
	void draw_water(GLuint shaderProgram);
};
#endif