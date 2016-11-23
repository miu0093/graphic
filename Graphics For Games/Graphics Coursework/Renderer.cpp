#include "Renderer.h"
#include "psapi.h"

Renderer::Renderer(Window & parent) : OGLRenderer(parent) {
	rotation = 0.0f;
	camera = new Camera(0.0f, 0.0f, Vector3(RAW_WIDTH * HEIGHTMAP_X / 2.0f, 700, RAW_WIDTH * HEIGHTMAP_X));
	emitter = new ParticleEmitter();
	lightsta = new	Light(Vector3((RAW_HEIGHT*HEIGHTMAP_X), 1500.0f, (RAW_HEIGHT*HEIGHTMAP_Z)),
		                  Vector4(1.0f, 1.0f, 1.0f, 1), 4800.f);

	quad = Mesh::GenerateQuad();

	pointLights = new Light[LIGHTNUM * LIGHTNUM]; //default constructor

	for (int x = 0; x < LIGHTNUM; ++x) {
		for (int z = 0; z < LIGHTNUM; ++z) {
			Light & l = pointLights[(x * LIGHTNUM) + z];
			
			float xPos = (RAW_WIDTH * HEIGHTMAP_X / (LIGHTNUM - 1)) * x;
			float zPos = (RAW_HEIGHT * HEIGHTMAP_Z / (LIGHTNUM - 1)) * z;
			l.SetPosition(Vector3(xPos, 100.0f, zPos));
			
			float r = 0.5f + (float)(rand() % 129) / 128.0f;
			float g = 0.5f + (float)(rand() % 129) / 128.0f;
			float b = 0.5f + (float)(rand() % 129) / 128.0f;
			l.SetColour(Vector4(r, g, b, 1.0f));

			float radius = (RAW_WIDTH * HEIGHTMAP_X / LIGHTNUM);
			l.SetRadius(radius);
		}
	}

	heightMap = new HeightMap(TEXTUREDIR"fujib.raw");

	quad->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"lava.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	quad->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"NormalMap.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	heightMap -> SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"volc2.jpg", SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	
	heightMap -> SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"volc2m.png", SOIL_LOAD_AUTO,SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"blood_ft.tga", TEXTUREDIR"blood_bk.tga",
		                            TEXTUREDIR"blood_up.tga", TEXTUREDIR"blood_dn.tga",
		                            TEXTUREDIR"blood_rt.tga", TEXTUREDIR"blood_lf.tga",
		                            SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	cubemap1 = SOIL_load_OGL_cubemap(TEXTUREDIR"starfield_ft.tga", TEXTUREDIR"starfield_bk.tga",
	                                   TEXTUREDIR"starfield_up.tga", TEXTUREDIR"starfield_dn.tga",
		                               TEXTUREDIR"starfield_rt.tga", TEXTUREDIR"starfield_lf.tga",
		                               SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	basicFont = new Font(SOIL_load_OGL_texture(TEXTUREDIR"tahoma.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT), 16, 16);

	SetTextureRepeating(quad -> GetTexture(), true);
	SetTextureRepeating(quad -> GetBumpMap(), true);
	SetTextureRepeating(heightMap -> GetTexture(), true);
	SetTextureRepeating(heightMap -> GetBumpMap(), true);
	
	sphere = new OBJMesh();

	if (!sphere -> LoadOBJMesh(MESHDIR"ico.obj")) {
		return;
	}

	shadowShader = new Shader(SHADERDIR"lightshaver.glsl", SHADERDIR"lightshafrag.glsl");//shadow
	if (!shadowShader->LinkProgram()) {
		return;
	}

	reflectShader = new Shader(SHADERDIR"PerPixelVertex.glsl", SHADERDIR"ReflectFragment.glsl");
	if (!reflectShader->LinkProgram()) {
		return;
	}

	skyboxShader = new Shader(SHADERDIR"SkyboxVertex.glsl", SHADERDIR"SkyboxFragment.glsl");
	if (!skyboxShader->LinkProgram()) {
		return;
	}

	lightShader = new Shader(SHADERDIR"PerPixelVertex.glsl", SHADERDIR"PerPixelFragment.glsl");
	if (!lightShader->LinkProgram()) {
		return;
	}

	sceneShader = new Shader(SHADERDIR"BumpVertex.glsl",SHADERDIR"bufferFragment.glsl");
	if (!sceneShader -> LinkProgram()) {
		return;
	}

	txtShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	if (!txtShader->LinkProgram()) {
		return;
	}

	combineShader = new Shader(SHADERDIR"combinevert.glsl", SHADERDIR"combinefrag.glsl");
	if (!combineShader -> LinkProgram()) {
		return;
	}

	particalShader = new Shader(SHADERDIR"vertex.glsl",SHADERDIR"fragment.glsl",SHADERDIR"geometry.glsl");//partical
	if (!particalShader->LinkProgram()) {
		return;
	}

	pointlightShader = new Shader(SHADERDIR"pointlightvert.glsl",SHADERDIR"pointlightfrag.glsl");
	if (!pointlightShader -> LinkProgram()) {
		 return;
	}
	glGenTextures(1, &shadowTex);//shadow

	glBindTexture(GL_TEXTURE_2D, shadowTex);//shadow
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glBindTexture(GL_TEXTURE_2D, 0);//end

    glGenFramebuffers(1, &shadowFBO);//generate and bind shadow

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);//attachment
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &bufferFBO);
	
	glGenFramebuffers(1, &pointLightFBO);
	
	GLenum buffers[2];
	buffers[0] = GL_COLOR_ATTACHMENT0;
	buffers[1] = GL_COLOR_ATTACHMENT1;
	
	// Generate our scene depth texture ...
	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightEmissiveTex);
	GenerateScreenTexture(lightSpecularTex);
	// And now attach them to our FBOs
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
	GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
	GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=GL_FRAMEBUFFER_COMPLETE) {
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
	GL_TEXTURE_2D, lightEmissiveTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
	GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=GL_FRAMEBUFFER_COMPLETE) {
		return;
	}
	waterRotate = 0.0f;
	dayandnight = 1.0f;
	skyFade = 0.0f;
	isGoingDark = false;
	projMatrix = Matrix4::Perspective(1.0f, 80000.0f, (float)width / (float)height, 45.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	init = true;
}

Renderer ::~Renderer(void) {
    delete sceneShader;
    delete combineShader;
    delete pointlightShader;
	delete shadowShader;
	delete reflectShader;
	delete txtShader;
	delete skyboxShader;
	delete lightShader;
    delete heightMap;
    delete camera;
    delete sphere;
	delete basicFont;
	delete emitter;
    delete quad;
    delete[] pointLights;
	delete lightsta;

	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightEmissiveTex);
	glDeleteTextures(1, &lightSpecularTex);
	glDeleteTextures(1, &shadowTex);//shadow
	glDeleteFramebuffers(1, &shadowFBO);//shadow
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);
	currentShader = 0;	
}

void Renderer::GenerateScreenTexture(GLuint & into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	glTexImage2D(GL_TEXTURE_2D, 0,depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8,
		         width, height, 0,depth ? GL_DEPTH_COMPONENT : GL_RGBA,GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::UpdateScene(float msec) {
	emitter->Update(msec);
	camera -> UpdateCamera(msec);
	viewMatrix = camera -> BuildViewMatrix();
	rotation = msec * 0.01f;
	waterRotate += msec * 0.001f;
	dayandnight += (msec / 800000)*360.f;
	if (!isGoingDark)//Daylight
	{
		skyFade += msec / 10000;
		if (skyFade >= 1)
		{
			isGoingDark = true;
		}
	}
	else
	{
		skyFade -= msec / 10000;
		if (skyFade <= 0)
		{
			isGoingDark = false;
		}
	}
}

void Renderer::RenderScene() {

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	projMatrix = Matrix4::Perspective(1.0f, 100000.0f, 10.f, 45.0f);

	DrawShadowScene();

	viewMatrix = camera->BuildViewMatrix();
	
	FillBuffers();

	DrawPointLights();

	DrawSkybox();

	CombineBuffers();
	
	DrawText("FPS: ", Vector3(1, 1, 1), 35.0f);
	DrawText("This is volcano !!", Vector3(RAW_WIDTH * HEIGHTMAP_X / 2.5, 3800, RAW_WIDTH * HEIGHTMAP_X / 2), 100.0f, true);

    DrawParticles();
	
	SwapBuffers();
}

void Renderer::DrawSkybox() {
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "cubeTex"), 2);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "cubeTex2"), 7);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "skyFade"), skyFade);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap1);

	viewMatrix = viewMatrix * Matrix4::Rotation(dayandnight, Vector3(0, 1, 0));

	UpdateShaderMatrices();
	quad->Draw();

	glUseProgram(0);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}

void Renderer::DrawParticles() {

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	SetCurrentShader(particalShader);
	
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	SetShaderParticleSize(emitter->GetParticleSize());
	emitter->SetParticleSize(8.0f);
	emitter->SetParticleVariance(1.0f);
	emitter->SetLaunchParticles(200.0f);
	emitter->SetParticleLifetime(1000.0f);
	emitter->SetParticleSpeed(0.05f);

	modelMatrix = Matrix4::Translation(Vector3(RAW_WIDTH * HEIGHTMAP_X / 2.0f, 800, RAW_WIDTH * HEIGHTMAP_X));
	UpdateShaderMatrices();

	emitter->Draw();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

void Renderer::DrawShadowScene() {               //shadow
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	SetCurrentShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(lightsta->GetPosition(), Vector3(0, 0, 0));
	shadowMatrix = biasMatrix *(projMatrix * viewMatrix);
	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	heightMap->Draw();
	
	glUseProgram(0);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawWater() {
	SetCurrentShader(reflectShader);
	SetShaderLight(*lightsta);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float *)& camera->GetPosition());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	float heightX = (RAW_WIDTH * HEIGHTMAP_X / 2.0f);

	float heightY = 256 * HEIGHTMAP_Y / 4.8f;

	float heightZ = (RAW_HEIGHT * HEIGHTMAP_Z / 2.0f);

	modelMatrix =
		Matrix4::Translation(Vector3(heightX, heightY, heightZ)) *
		Matrix4::Scale(Vector3(15000, 1, 15000)) *
		Matrix4::Rotation(90, Vector3(1.0f, 0.0f, 0.0f));

	textureMatrix = Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f)) *
		Matrix4::Rotation(waterRotate, Vector3(0.0f, 0.0f, 1.0f));

	UpdateShaderMatrices();

	quad->Draw();

	glUseProgram(0);
}

void	Renderer::SetShaderParticleSize(float f) {
	glUniform1f(glGetUniformLocation(particalShader->GetProgram(), "particleSize"), f);
}

void Renderer::FillBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glDisable(GL_BLEND);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//glEnable(GL_BLEND);

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	Matrix4 tempMatrix = shadowMatrix*modelMatrix;
	//DrawSkybox();
	SetCurrentShader(sceneShader);
	glUniform1i(glGetUniformLocation(currentShader -> GetProgram(),"diffuseTex"), 0);

	glUniform1i(glGetUniformLocation(currentShader -> GetProgram(),"bumpTex"), 1);

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1,false, *&modelMatrix.values);//shadow

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "shadowMatrix"), 1, false ,*&tempMatrix.values);//shadow

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 6);
	glActiveTexture(GL_TEXTURE6); glBindTexture(GL_TEXTURE_2D, shadowTex);


	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	heightMap -> Draw();
	DrawWater();
	
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::DrawPointLights() {
	glEnable(GL_BLEND);
	SetCurrentShader(pointlightShader);
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glBlendFunc(GL_ONE, GL_ONE);
	
	glUniform1i(glGetUniformLocation(currentShader -> GetProgram(),"depthTex"), 3);

	glUniform1i(glGetUniformLocation(currentShader -> GetProgram(),"normTex"), 4);
	
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);
	
	glUniform3fv(glGetUniformLocation(currentShader -> GetProgram(),"cameraPos"), 1, (float *)& camera -> GetPosition());
	
	glUniform2f(glGetUniformLocation(currentShader -> GetProgram(),"pixelSize"), 1.0f / width, 1.0f / height);

	Vector3 translate = Vector3((RAW_HEIGHT * HEIGHTMAP_X / 2.0f), 500,(RAW_HEIGHT * HEIGHTMAP_Z / 2.0f));

	Matrix4 pushMatrix = Matrix4::Translation(translate);
	Matrix4 popMatrix = Matrix4::Translation(-translate);
	
	for (int x = 0; x < LIGHTNUM; ++x) {
		for (int z = 0; z < LIGHTNUM; ++z) {
			Light & l = pointLights[(x * LIGHTNUM) + z];
			float radius = l.GetRadius();
			
			modelMatrix =pushMatrix *
				Matrix4::Rotation(rotation, Vector3(0, 1, 0)) *popMatrix *
				Matrix4::Translation(l.GetPosition()) *
				Matrix4::Scale(Vector3(radius, radius, radius));
			
			l.SetPosition(modelMatrix.GetPositionVector());
			
				SetShaderLight(l);
			
				UpdateShaderMatrices();
			
				float dist = (l.GetPosition() -camera -> GetPosition()).Length();
			if (dist < radius) {// camera is inside the light volume !
				glCullFace(GL_FRONT);
			}
			else {
				glCullFace(GL_BACK);
			}
			sphere -> Draw();	
		}
	}
	Light & l = *lightsta;
	float radius = l.GetRadius();

	modelMatrix = Matrix4::Translation(l.GetPosition()) *
		          Matrix4::Scale(Vector3(radius, radius, radius));

	l.SetPosition(modelMatrix.GetPositionVector());

	SetShaderLight(l);

	UpdateShaderMatrices();

	float dist = (l.GetPosition() - camera->GetPosition()).Length();
	if (dist < radius) {// camera is inside the light volume !
		glCullFace(GL_FRONT);
	}
	else {
		glCullFace(GL_BACK);
	}
	sphere->Draw();
	glCullFace(GL_BACK);
	glDisable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.1f, 0.1f, 0.1f, 1);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void Renderer::CombineBuffers() {
	glEnable(GL_BLEND);
	SetCurrentShader(combineShader);
	
	projMatrix = Matrix4::Orthographic(-1, 1, 1, -1, -1, 1);
	UpdateShaderMatrices();
	
	glUniform1i(glGetUniformLocation(currentShader -> GetProgram(),"diffuseTex"), 2);

	glUniform1i(glGetUniformLocation(currentShader -> GetProgram(),"emissiveTex"), 3);

	glUniform1i(glGetUniformLocation(currentShader -> GetProgram(),"specularTex"), 4);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 2);
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, lightEmissiveTex);
	
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);
	
	quad -> Draw();
	
	glDisable(GL_BLEND);
	glUseProgram(0);
}

void Renderer::DrawText(const std::string &text, const Vector3 &position, const float size, const bool perspective) {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	//Create a new temporary TextMesh, using our line of text and our font
	TextMesh* mesh = new TextMesh(text, *basicFont);

	//This just does simple matrix setup to render in either perspective or
	//orthographic mode, there's nothing here that's particularly tricky.
	if (perspective) {
		textureMatrix.ToIdentity();
		modelMatrix = Matrix4::Translation(position) * Matrix4::Scale(Vector3(size, size, 1));
		viewMatrix = camera->BuildViewMatrix();
		projMatrix = Matrix4::Perspective(1.0f, 100000.0f, (float)width / (float)height, 45.0f);
	}
	else {
		//In ortho mode, we subtract the y from the height, so that a height of 0
		//is at the top left of the screen, which is more intuitive
		//(for me anyway...)
		textureMatrix.ToIdentity();
		modelMatrix = Matrix4::Translation(Vector3(position.x, height - position.y, position.z)) * Matrix4::Scale(Vector3(size, size, 1));
		viewMatrix.ToIdentity();
		projMatrix = Matrix4::Orthographic(-1.0f, 1.0f, (float)width, 0.0f, (float)height, 0.0f);
	}
	//Either way, we update the matrices, and draw the mesh

	SetCurrentShader(txtShader);//Enable the shader...
								//And turn on texture unit 0
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	UpdateShaderMatrices();
	mesh->Draw();

	delete mesh; //Once it's drawn, we don't need it anymore!;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);
}