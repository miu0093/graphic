#pragma once

#include <string>
#include <iostream>
#include <fstream>

#include "Mesh.h"

#define RAW_WIDTH 1081
#define RAW_HEIGHT 1081

#define HEIGHTMAP_X 16.0f
#define HEIGHTMAP_Z 16.0f
#define HEIGHTMAP_Y 1.25f*8
#define HEIGHTMAP_TEX_X 1.0f / 16.0f
#define HEIGHTMAP_TEX_Z 1.0f / 16.0f

class HeightMap : public Mesh {
	public:
	HeightMap(std::string name);
	~HeightMap(void) {};};