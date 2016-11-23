#pragma once

#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJmesh.h"
#include "../../nclgl/HeightMap.h"
#include "../../nclgl/Light.h"
#include "../../nclgl/TextMesh.h"
#include "../../nclgl/ParticleEmitter.h"

# define LIGHTNUM 8 // We ’ll generate LIGHTNUM squared lights ...
#define SHADOWSIZE 2048 

class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	virtual ~Renderer(void);

	virtual void RenderScene();
	virtual void UpdateScene(float msec);
	void	DrawText(const std::string &text, const Vector3 &position, const float size = 10.0f, const bool perspective = false);
protected:
	void	SetShaderParticleSize(float f);
	void DrawShadowScene();
	void FillBuffers(); //G- Buffer Fill Render Pass
	void DrawPointLights(); // Lighting Render Pass
	void CombineBuffers(); // Combination Render Pass
	void DrawWater();
	void DrawSkybox();
	void DrawParticles();
	// Make a new texture ...
	void GenerateScreenTexture(GLuint & into, bool depth = false);
	Shader * sceneShader; // Shader to fill our GBuffers
	Shader * pointlightShader; // Shader to calculate lighting
	Shader * combineShader; // shader to stick it all together
	Shader * lightShader;
	Shader * reflectShader;
	Shader * skyboxShader;
	Shader * shadowShader;
	Shader * txtShader;
	Shader * particalShader;

	Light * lightsta;
	Light * pointLights; // Array of lighting data
	Mesh * heightMap; // Terrain !
	OBJMesh * sphere; // Light volume
	Mesh * quad; // To draw a full - screen quad
	Camera * camera; // Our usual camera

	Font*	basicFont;

	float rotation; // How much to increase rotation by
	ParticleEmitter*	emitter;

	GLuint bufferFBO; // FBO for our G- Buffer pass
	GLuint bufferColourTex; // Albedo goes here
	GLuint bufferNormalTex; // Normals go here
	GLuint bufferDepthTex; // Depth goes here
	GLuint cubeMap;
	GLuint cubemap1;
	GLuint shadowTex;
	GLuint shadowFBO;
	GLuint pointLightFBO; // FBO for our lighting pass
	GLuint lightEmissiveTex; // Store emissive lighting
	GLuint lightSpecularTex; // Store specular lighting	
	float waterRotate;
	float dayandnight;
	float skyFade;
	bool  isGoingDark;
};