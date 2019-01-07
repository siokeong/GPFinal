#include "../Externals/Include/Include.h"
#include "mesh_data.h"
#include "texture_data.h"
#include <windows.h>
#include <conio.h>



#define MENU_TIMER_START 1
#define MENU_TIMER_STOP 2
#define MENU_EXIT 3

float tmp = 0;
float wave_cnt = 0.0f;
float movCnt = 0.0f;
bool isMoving = false;
int movMode = 0;

int screen_width = 600;
int screen_height = 600;

int shadow_width = 8192*2;
int shadow_height = 8192*2;

GLuint quadVAO;
GLuint quadVBO;
GLuint programdebug;

GLuint depthMapFBO;
GLuint depthMap;
GLuint depthprogram;

GLuint DiffuseTextureID;
GLuint NormalTextureID;

GLuint program;
GLuint um4p;
GLuint um4mv;
GLuint um4mvit;
GLuint ueye;

GLubyte timer_cnt = 0;
bool timer_enabled = true;
unsigned int timer_speed = 16;

using namespace glm;
using namespace std;

mat4 mv, p;
mat4 mvit;


GLuint program2;
GLuint smv;
GLuint sp;
GLuint stex;


struct Shape { 
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_normal;
	GLuint vbo_texcoord;
	GLuint vbo_tangent;
	GLuint ibo;
	int drawCount;
	int materialID; 
};

struct Material { 
	GLuint diffuse_tex;
	GLuint normals_tex;
};

Material* materials;
Shape* shapes;
float eyex = 0.0, eyey = 0.0, eyez = 0.0;
const aiScene *scene;
int scenemode = 0;


//Camera set
vec3 cposition = vec3(-60.0, 80.0, 25.0);
vec3 cfront = vec3(0.0,0.0,-1.0);
vec3 cup = vec3(0.0,1.0,0.0);
vec3 cworldup = vec3(0.0, 1.0, 0.0);
vec3 cright ;

float cyaw = 0.0f;
float cpitch = 0.0f;
float cmovementspeed = 0.3f;
float cmousesentivity = 0.20f;
float czoom = 45.0f;

void updateCameraVectors()
{
	// Calculate the new Front vector
	vec3 temp;
	temp.x = cos(radians(cyaw)) * cos(radians(cpitch));
	temp.y = sin(radians(cpitch));
	temp.z = sin(radians(cyaw)) * cos(radians(cpitch));
	cfront = normalize(temp);
	// Also re-calculate the Right and Up vector
	cright = normalize(cross(cfront, cworldup));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
	cup = normalize(cross(cright, cfront));
}

float oldx, oldy , xoffset, yoffset;


GLuint skyboxVAO, skyboxVBO;
GLuint cubemapTexture;
// skybox vertex
GLfloat skyboxVertices[] = {
	// Positions
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f,  1.0f
};


char** loadShaderSource(const char* file)
{
	FILE* fp = fopen(file, "rb");
	fseek(fp, 0, SEEK_END);
	long sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *src = new char[sz + 1];
	fread(src, sizeof(char), sz, fp);
	src[sz] = '\0';
	char **srcp = new char*[1];
	srcp[0] = src;
	return srcp;
}

void freeShaderSource(char** srcp)
{
	delete[] srcp[0];
	delete[] srcp;
}

// define a simple data structure for storing texture image raw data
typedef struct _TextureData
{
	_TextureData(void) :
		width(0),
		height(0),
		data(0)
	{
	}

	int width;
	int height;
	unsigned char* data;
} TextureData;

// load a png image and return a TextureData structure with raw data
// not limited to png format. works with any image format that is RGBA-32bit
TextureData loadPNG(const char* const pngFilepath)
{
	TextureData texture;
	int components;

	// load the texture with stb image, force RGBA (4 components required)
	stbi_uc *data = stbi_load(pngFilepath, &texture.width, &texture.height, &components, 4);

	// is the image successfully loaded?
	if (data != NULL)
	{
		// copy the raw data
		size_t dataSize = texture.width * texture.height * 4 * sizeof(unsigned char);
		texture.data = new unsigned char[dataSize];
		memcpy(texture.data, data, dataSize);

		// mirror the image vertically to comply with OpenGL convention
		for (size_t i = 0; i < texture.width; ++i)
		{
			for (size_t j = 0; j < texture.height / 2; ++j)
			{
				for (size_t k = 0; k < 4; ++k)
				{
					size_t coord1 = (j * texture.width + i) * 4 + k;
					size_t coord2 = ((texture.height - j - 1) * texture.width + i) * 4 + k;
					std::swap(texture.data[coord1], texture.data[coord2]);
				}
			}
		}

		// release the loaded image
		stbi_image_free(data);
	}

	return texture;
}

void showShaderCompileStatus(GLuint shader, GLint *shaderCompiled)
{
	glGetShaderiv(shader, GL_COMPILE_STATUS, shaderCompiled);
	if (GL_FALSE == (*shaderCompiled))
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character.
		GLchar *errorLog = (GLchar*)malloc(sizeof(GLchar) * maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);
		fprintf(stderr, "%s", errorLog);

		glDeleteShader(shader);
		free(errorLog);
	}
}

void My_Init()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);


	// debug
	programdebug = glCreateProgram();
	GLuint vertexShaderdebug = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderdebug = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSourcedebug = loadShaderSource("debug.vs.glsl");
	char** fragmentShaderSourcedebug = loadShaderSource("debug.fs.glsl");
	glShaderSource(vertexShaderdebug, 1, vertexShaderSourcedebug, NULL);
	glShaderSource(fragmentShaderdebug, 1, fragmentShaderSourcedebug, NULL);
	freeShaderSource(vertexShaderSourcedebug);
	freeShaderSource(fragmentShaderSourcedebug);
	glCompileShader(vertexShaderdebug);
	GLint vShaderCompileddebug;
	showShaderCompileStatus(vertexShaderdebug, &vShaderCompileddebug);
	if (!vShaderCompileddebug) system("pause"), exit(123);
	glCompileShader(fragmentShaderdebug);
	GLint vFragmentCompileddebug;
	showShaderCompileStatus(fragmentShaderdebug, &vFragmentCompileddebug);
	if (!vFragmentCompileddebug) system("pause"), exit(456);
	glAttachShader(programdebug, vertexShaderdebug);
	glAttachShader(programdebug, fragmentShaderdebug);
	glLinkProgram(programdebug);
	glUseProgram(programdebug);

	float quadVertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


	// depth
	depthprogram = glCreateProgram();
	GLuint vertexShaderdepth = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderdepth = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSourcedepth = loadShaderSource("depth.vs.glsl");
	char** fragmentShaderSourcedepth = loadShaderSource("depth.fs.glsl");
	glShaderSource(vertexShaderdepth, 1, vertexShaderSourcedepth, NULL);
	glShaderSource(fragmentShaderdepth, 1, fragmentShaderSourcedepth, NULL);
	freeShaderSource(vertexShaderSourcedepth);
	freeShaderSource(fragmentShaderSourcedepth);
	glCompileShader(vertexShaderdepth);
	GLint vShaderCompileddepth;
	showShaderCompileStatus(vertexShaderdepth, &vShaderCompileddepth);
	if (!vShaderCompileddepth) system("pause"), exit(123);
	glCompileShader(fragmentShaderdepth);
	GLint vFragmentCompileddepth;
	showShaderCompileStatus(fragmentShaderdepth, &vFragmentCompileddepth);
	if (!vShaderCompileddepth) system("pause"), exit(456);
	glAttachShader(depthprogram, vertexShaderdepth);
	glAttachShader(depthprogram, fragmentShaderdepth);
	glLinkProgram(depthprogram);
	glUseProgram(depthprogram);

	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_width, shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//Program2 skybox

	program2 = glCreateProgram();
	GLuint vertexShader2 = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader2 = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource2 = loadShaderSource("skyfvert.vs.glsl");
	char** fragmentShaderSource2 = loadShaderSource("skyfrag.fs.glsl");
	glShaderSource(vertexShader2, 1, vertexShaderSource2, NULL);
	glShaderSource(fragmentShader2, 1, fragmentShaderSource2, NULL);
	freeShaderSource(vertexShaderSource2);
	freeShaderSource(fragmentShaderSource2);
	glCompileShader(vertexShader2);
	GLint vShaderCompiled2;
	showShaderCompileStatus(vertexShader2, &vShaderCompiled2);
	if (!vShaderCompiled2) system("pause"), exit(123);

	glCompileShader(fragmentShader2);
	GLint vFragmentCompiled2;
	showShaderCompileStatus(fragmentShader2, &vFragmentCompiled2);
	if (!vShaderCompiled2) system("pause"), exit(456);

	glAttachShader(program2, vertexShader2);
	glAttachShader(program2, fragmentShader2);
	glLinkProgram(program2);
	smv = glGetUniformLocation(program2, "smv");
	sp = glGetUniformLocation(program2, "sp");
	stex = glGetUniformLocation(program2, "skybox");
	
	glUseProgram(program2);

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);

	vector<const GLchar*> faces;
	faces.push_back("Skybox/right.png");
	faces.push_back("Skybox/left.png");
	faces.push_back("Skybox/bottom.png");
	faces.push_back("Skybox/top.png");
	faces.push_back("Skybox/back.png");
	faces.push_back("Skybox/front.png");
	glGenTextures(1, &cubemapTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

	for (GLuint i = 0; i < faces.size(); i++)
	{
		TextureData skytex = loadPNG(faces[i]);
		//cout << faces[i] << endl;
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, skytex.width, skytex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, skytex.data);
	}
	
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	



	
	// Program1
	
	program = glCreateProgram();
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	char** vertexShaderSource = loadShaderSource("vertex.vs.glsl");
	char** fragmentShaderSource = loadShaderSource("fragment.fs.glsl");
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);
	glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);
	freeShaderSource(vertexShaderSource);
	freeShaderSource(fragmentShaderSource);
	glCompileShader(vertexShader);
	GLint vShaderCompiled;
	showShaderCompileStatus(vertexShader, &vShaderCompiled);
	if (!vShaderCompiled) system("pause"), exit(123);

	glCompileShader(fragmentShader);
	GLint vFragmentCompiled;
	showShaderCompileStatus(fragmentShader, &vFragmentCompiled);
	if (!vShaderCompiled) system("pause"), exit(456);

	//glPrintShaderLog(vertexShader);
	//glPrintShaderLog(fragmentShader);
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	um4mv = glGetUniformLocation(program, "um4mv");
	um4p = glGetUniformLocation(program, "um4p");
	um4mvit = glGetUniformLocation(program, "um4mvit");
	ueye = glGetUniformLocation(program, "ueye");
	DiffuseTextureID = glGetUniformLocation(program, "tex_color");
	NormalTextureID = glGetUniformLocation(program, "tex_normal");
	glUseProgram(program);


	

	//HW3
	scene = aiImportFile("ElvenRuins.obj", aiProcessPreset_TargetRealtime_MaxQuality);

	aiPostProcessSteps(aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

	 materials = (Material* )malloc(scene->mNumMaterials * sizeof(Material));

	 for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
	 {
		 aiMaterial *material = scene->mMaterials[i];
		 //	cout << i << ' ' << material->GetTextureCount(aiTextureType_LIGHTMAP) << endl;
		 Material material1;
		 aiString texturePath;
		 if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
		 { // load width, height and data from texturePath.C_Str();

			 TextureData tex = loadPNG(texturePath.C_Str());
			 //printf("diffuse %s\n", texturePath.C_Str());
			 glGenTextures(1, &material1.diffuse_tex);
			 glBindTexture(GL_TEXTURE_2D, material1.diffuse_tex);
			 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
			 glGenerateMipmap(GL_TEXTURE_2D);
		 }
		 else { // load some default image as default_diffuse_tex 
			 TextureData tex = loadPNG(texturePath.C_Str());
			 glGenTextures(1, &material1.diffuse_tex);
			 glBindTexture(GL_TEXTURE_2D, material1.diffuse_tex);
			 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
			 glGenerateMipmap(GL_TEXTURE_2D);
		 }

		 if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == aiReturn_SUCCESS)
		 { // load width, height and data from texturePath.C_Str();
			 TextureData tex = loadPNG(texturePath.C_Str());
			 //printf("normals %s\n", texturePath.C_Str());
			 glGenTextures(1, &material1.normals_tex);
			 glBindTexture(GL_TEXTURE_2D, material1.normals_tex);
			 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
			 glGenerateMipmap(GL_TEXTURE_2D);
		 }
		 else { // load some default image as default_diffuse_tex 
			 TextureData tex = loadPNG(texturePath.C_Str());
			 glGenTextures(1, &material1.normals_tex);
			 glBindTexture(GL_TEXTURE_2D, material1.normals_tex);
			 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.data);
			 glGenerateMipmap(GL_TEXTURE_2D);
		 }
		 // save material¡K }
		 materials[i] = material1;
	 }

	 shapes = (Shape*)malloc(scene->mNumMeshes * sizeof(Shape));
	 for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
		 aiMesh *mesh = scene->mMeshes[i];
		 Shape shape;
		 glGenVertexArrays(1, &shape.vao);
		 glBindVertexArray(shape.vao);
		 // create 3 vbos to hold data 


		 float* pos = new float[mesh->mNumVertices * 3]();
		 float* nor = new float[mesh->mNumVertices * 3]();
		 float* texcoord = new float[mesh->mNumVertices * 2]();
		 for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
		 {

			 pos[v * 3] = mesh->mVertices[v][0];
			 pos[v * 3 + 1] = mesh->mVertices[v][1];
			 pos[v * 3 + 2] = mesh->mVertices[v][2];

			 nor[v * 3] = mesh->mNormals[v][0];
			 nor[v * 3 + 1] = mesh->mNormals[v][1];
			 nor[v * 3 + 2] = mesh->mNormals[v][2];

			 if (mesh->mTextureCoords[0]) {
				 texcoord[v * 2] = mesh->mTextureCoords[0][v][0];
				 texcoord[v * 2 + 1] = mesh->mTextureCoords[0][v][1];
			 }
			 else {
				 texcoord[v * 2] = 0.0f;
				 texcoord[v * 2 + 1] = 0.0f;
			 }

			 // mesh->mVertices[v][0~2] => position 
			 // mesh->mNormals[v][0~2] => normal 
			 // mesh->mTextureCoords[0][v][0~1] => texcoord 
		 }

		 glGenBuffers(1, &shape.vbo_texcoord);
		 glGenBuffers(1, &shape.vbo_normal);
		 glGenBuffers(1, &shape.vbo_position);

		 glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_position);

		 glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * 3 * sizeof(float), pos, GL_STATIC_DRAW);
		 glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, 0);

		 glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_normal);
		 glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * 3 * sizeof(float), nor, GL_STATIC_DRAW);
		 glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 3, 0);

		 glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_texcoord);
		 glBufferData(GL_ARRAY_BUFFER, mesh->mNumVertices * 2 * sizeof(float), texcoord, GL_STATIC_DRAW);
		 glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT) * 2, 0);


		 float* tangents = new float[mesh->mNumFaces * 9]();
		 for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
			 if (mesh->mNumFaces == 1) {
				 continue;
			 }
			 int idx0 = mesh->mFaces[f].mIndices[0];
			 int idx1 = mesh->mFaces[f].mIndices[1];
			 int idx2 = mesh->mFaces[f].mIndices[2];

			 vec3 p1 = vec3(mesh->mVertices[idx0][0], mesh->mVertices[idx0][1], mesh->mVertices[idx0][2]);
			 vec3 p2 = vec3(mesh->mVertices[idx1][0], mesh->mVertices[idx1][1], mesh->mVertices[idx1][2]);
			 vec3 p3 = vec3(mesh->mVertices[idx2][0], mesh->mVertices[idx2][1], mesh->mVertices[idx2][2]);
			 
			 vec3 n1 = vec3(mesh->mNormals[idx0][0], mesh->mNormals[idx0][1], mesh->mNormals[idx0][2]);
			 vec3 n2 = vec3(mesh->mNormals[idx1][0], mesh->mNormals[idx1][1], mesh->mNormals[idx1][2]);
			 vec3 n3 = vec3(mesh->mNormals[idx2][0], mesh->mNormals[idx2][1], mesh->mNormals[idx2][2]);
			 
			 vec2 uv1 = vec2(mesh->mTextureCoords[0][idx0][0], mesh->mTextureCoords[0][idx0][1]);
			 vec2 uv2 = vec2(mesh->mTextureCoords[0][idx1][0], mesh->mTextureCoords[0][idx1][1]);
			 vec2 uv3 = vec2(mesh->mTextureCoords[0][idx2][0], mesh->mTextureCoords[0][idx2][1]);

			 vec3 dp1 = p2 - p1;
			 vec3 dp2 = p3 - p1;

			 vec2 duv1 = uv2 - uv1;
			 vec2 duv2 = uv3 - uv1;

			 float r = 1.0f / (duv1[0] * duv2[1] - duv1[1] * duv2[0]);

			 vec3 t;
			 t[0] = (dp1[0] * duv2[1] - dp2[0] * duv1[1]) * r;
			 t[1] = (dp1[1] * duv2[1] - dp2[1] * duv1[1]) * r;
			 t[2] = (dp1[2] * duv2[1] - dp2[2] * duv1[1]) * r;

			 vec3 t1 = glm::cross(n1, t);
			 vec3 t2 = glm::cross(n2, t);
			 vec3 t3 = glm::cross(n3, t);

			 tangents[idx0 * 3 + 0] += t1[0];
			 tangents[idx0 * 3 + 1] += t1[1];
			 tangents[idx0 * 3 + 2] += t1[2];

			 tangents[idx1 * 3 + 0] += t2[0];
			 tangents[idx1 * 3 + 1] += t2[1];
			 tangents[idx1 * 3 + 2] += t2[2];

			 tangents[idx2 * 3 + 0] += t3[0];
			 tangents[idx2 * 3 + 1] += t3[1];
			 tangents[idx2 * 3 + 2] += t3[2];
		 }

		 glGenBuffers(1, &shape.vbo_tangent);
		 glBindBuffer(GL_ARRAY_BUFFER, shape.vbo_tangent);
		 glBufferData(GL_ARRAY_BUFFER, sizeof(tangents), tangents, GL_STATIC_DRAW);
		 glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
		 glEnableVertexAttribArray(3);
		 delete tangents;


		 // create 1 ibo to hold data 
		 glGenBuffers(1, &shape.ibo);
		 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape.ibo);

		 int* index = new int[mesh->mNumFaces * 3]();
		 for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
		 {
			 index[f * 3] = mesh->mFaces[f].mIndices[0];
			 index[f * 3 + 1] = mesh->mFaces[f].mIndices[1];
			 index[f * 3 + 2] = mesh->mFaces[f].mIndices[2];

			 // mesh->mFaces[f].mIndices[0~2] => index 
		 }
		 glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->mNumFaces * 3 * sizeof(float), index, GL_STATIC_DRAW);
		 glEnableVertexAttribArray(0);
		 glEnableVertexAttribArray(1);
		 glEnableVertexAttribArray(2);


		 // glVertexAttribPointer / glEnableVertexArray calls¡K 
		 shape.materialID = mesh->mMaterialIndex;
		 shape.drawCount = mesh->mNumFaces * 3;
		 // save shape¡K 
		 shapes[i] = shape;

	 }
	 
	//Camera
	updateCameraVectors();

}
void DrawCurve(vec3 pos1, vec3 pos2, vec3 pos3, vec3 pos4)
{
	printf("%f\n",movCnt);
	float rt = 1.0f - movCnt;
	vec3 pos = (pos1 * rt * rt * rt)
		+ (3.0f * pos2 * movCnt * rt * rt)
		+ (3.0f * pos3 * movCnt * movCnt * rt)
		+ (pos4 * movCnt * movCnt * movCnt);
	cposition = pos;
	movCnt += 0.001f;
	if (movCnt >= 1.0f) {
		isMoving = false;
		movCnt = 0.0f;
		movMode = 0;
	}
}
void My_Display()
{
	mat4 scale_matrix = scale(mat4(), vec3(0.01f, 0.01f, 0.01f));

	mat4 lightProjection, lightView;
	mat4 lightSpaceMatrix;
	float near_plane = 1.0f, far_plane = 600.0f;
	lightProjection = glm::ortho(-300.0f, 300.0f, -100.0f, 200.0f, near_plane, far_plane);
	lightView = lookAt(vec3(-160.0, 180.0, 125.0), vec3(0.0f), vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView * scale_matrix;

	glUseProgram(depthprogram);
	glUniformMatrix4fv(glGetUniformLocation(depthprogram, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));
	glCullFace(GL_FRONT);
	glViewport(0, 0, shadow_width, shadow_height);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		for (int i = 0; i < scene->mNumMeshes; ++i) {
			glBindVertexArray(shapes[i].vao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			glDrawElements(GL_TRIANGLES, shapes[i].drawCount, GL_UNSIGNED_INT, 0);
		}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCullFace(GL_BACK);

	glViewport(0, 0, screen_width, screen_height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// organize the arrays
	glUseProgram(program);

	mv = lookAt(cposition, cposition + cfront, cup);
	mat4 mvs = mv*scale_matrix;
	mvit = transpose(inverse(mvs));
	
	glUniformMatrix4fv(um4mvit, 1, GL_FALSE, value_ptr(mvit));
	glUniformMatrix4fv(um4mv, 1, GL_FALSE, value_ptr(mvs));
	glUniformMatrix4fv(um4p, 1, GL_FALSE, value_ptr(p));
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(DiffuseTextureID, 0);
	glUniform1i(NormalTextureID, 1);
	glUniform1i( glGetUniformLocation(program, "depthmap"), 2);
	glUniformMatrix4fv(glGetUniformLocation(program, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));
	glUniform1f(glGetUniformLocation(program, "water_time"), wave_cnt * 10);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		for (int i = 0; i < scene->mNumMeshes; ++i)
		{
			glBindVertexArray(shapes[i].vao);
			int materialID = shapes[i].materialID;
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, materials[materialID].diffuse_tex);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, materials[materialID].normals_tex);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, depthMap);
			if (i == 1474) glUniform1i(glGetUniformLocation(program, "ocean"), 1);
			else glUniform1i(glGetUniformLocation(program, "ocean"), 0);
			if (i < 600 && i>500)glDrawElements(GL_TRIANGLES, shapes[i].drawCount, GL_UNSIGNED_INT, 0);
			else glDrawElements(GL_TRIANGLES, shapes[i].drawCount - 3, GL_UNSIGNED_INT, 0);
		}

	//skybox
	glUseProgram(program2);
	mat4 r = rotate(mat4(), radians(180.0f), vec3(0.0, 0.0, 1.0));
	mat4 s = scale(mat4(), vec3(10000000.0, 10000000.0, 10000000.0));
	glUniformMatrix4fv(smv, 1, GL_FALSE, value_ptr(mv*r*s));
	glUniformMatrix4fv(sp, 1, GL_FALSE, value_ptr(p));
	glUniform1i(stex, 0);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	
	// depth visual debugging
	glUseProgram(programdebug);
	glUniform1i(glGetUniformLocation(programdebug, "depthmap"), 0);
	glUniform1f(glGetUniformLocation(programdebug, "near_plane"), near_plane);
	glUniform1f(glGetUniformLocation(programdebug, "far_plane"), far_plane);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	if (tmp > 2.5f) {
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glutSwapBuffers();
}

void My_Reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	
	float viewportAspect = (float)width / (float)height;
	vec3 eye = vec3(500.0 + eyex, 180.0 + eyey, 150.0);
	//mv = lookAt(eye, vec3(500.0f, 180.0f, 200.0f), vec3(0.0f, 1.0f, 0.0f));
	mv = lookAt(cposition, cposition + cfront, cup);
	
	//p = ortho(-6 * viewportAspect, 6 * viewportAspect, -6.0f, 6.0f, 0.0f, 10.0f);
	p = perspective(czoom, (float)width / (float)height, 0.1f, 10000000.0f);

	screen_width = width;
	screen_height = height;
}

void My_Timer(int val)
{
	timer_cnt += 1.0f;
	glutPostRedisplay();
	if (isMoving&& movMode!=0) {
		if(movMode == 1)DrawCurve(vec3(-57.0f, 75.0f, 50.0f), vec3(-14.0f, 87.0f, 66.0f), vec3(8.0f, 100.0f, 48.0f), vec3(40.0f, 137.0f, 2.0f));
		else if(movMode == 2)DrawCurve(vec3(40.0f, 137.0f, 2.0f), vec3(74.0f, 110.0f, 95.0f), vec3(110.0f, 90.0f, 173.0f), vec3(-27.0f, 86.0f, 255.0f));
		else if (movMode == 3)DrawCurve(vec3(-27.0f, 86.0f, 255.0f), vec3(-83.0f, 95.0f, 150.0f), vec3(-126.0f, 100.0f, 69.0f), vec3(-172.0f, 102.0f, -105.0f));
	}
	glutTimerFunc(timer_speed, My_Timer, val);
	wave_cnt += 1.0f;
}

void My_Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		oldx = x;
		oldy = y;
		//printf("Mouse %d is pressed at (%d, %d)\n", button, x, y);
	}
	else if (state == GLUT_UP)
	{
		//printf("Mouse %d is released at (%d, %d)\n", button, x, y);
	}
}

void My_MotionMouse(int x, int y)
{
	xoffset = x - oldx;
	yoffset = y - oldy;

	xoffset *= cmousesentivity;
	yoffset *= cmousesentivity;
	cyaw += xoffset;
	cpitch += yoffset;
	

	updateCameraVectors();
	mv = lookAt(cposition, cposition + cfront, cup);

	oldx = x;
	oldy = y;


	glutPostRedisplay();
}



void My_Keyboard(unsigned char key, int x, int y)
{
	//printf("Key %c is pressed at (%d, %d)\n", key, x, y);
	GLfloat cvelocity = cmovementspeed;
	if (key == 'd')
	{
		eyex += 1;
		cposition += cright * cvelocity;
	}
	else if (key == 'a')
	{
		eyex -= 1;
		cposition -= cright * cvelocity;
	}
	else if (key == 'w')
	{
		eyey += 1;
		cposition += cfront * cvelocity;
	}
	else if (key == 's') {
		eyey -= 1;
		cposition -= cfront * cvelocity;
	}else if (key == 'z') {
		eyez += 1;
		cposition.y +=  cvelocity;
	}else if (key == 'x') {
		eyez -= 1;
		cposition.y -= cvelocity;
	}
	else if (key == 'c') {
		if (tmp > 2) tmp = 0;
		else tmp = 5.0f;
	}
	//vec3 eye = vec3(0.0 + eyex, -5.0 + eyey, eyez + 3.0);
	//mv = lookAt(eye, eye + vec3(0.0, 0.0, -1.0), vec3(0.0f, -1.0f, 0.0f));
	//printf("%f %f %f\n",cposition.x,cposition.y,cposition.z);
}

void My_SpecialKeys(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_F1:
		printf("F1 is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_PAGE_UP:
		printf("Page up is pressed at (%d, %d)\n", x, y);
		break;
	case GLUT_KEY_LEFT:
		printf("Left arrow is pressed at (%d, %d)\n", x, y);
		break;
	default:
		printf("Other special key is pressed at (%d, %d)\n", x, y);
		break;
	}
}

void My_Menu(int id)
{
	switch (id)
	{
	case MENU_TIMER_START:
		if (!timer_enabled)
		{
			timer_enabled = true;
			glutTimerFunc(timer_speed, My_Timer, 0);
		}
		break;
	case MENU_TIMER_STOP:
		timer_enabled = false;
		break;
	case MENU_EXIT:
		exit(0);
		break;
	case 4:
		isMoving = true;
		movMode = 1;
		movCnt = 0.0f;
		break;
	case 5:
		isMoving = true;
		movMode = 2;
		movCnt = 0.0f;
		break;
	case 6:
		isMoving = true;
		movMode = 3;
		movCnt = 0.0f;
		break;
	case 7:
		isMoving = false;
		break;
	case 8:
		isMoving = true;
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
#ifdef __APPLE__
	// Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(screen_width, screen_height);
	glutCreateWindow("final"); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif
	glPrintContextInfo();
	My_Init();
	
	
	// Create a menu and bind it to mouse right button.
	int menu_main = glutCreateMenu(My_Menu);
	int menu_timer = glutCreateMenu(My_Menu);

	glutSetMenu(menu_main);
	glutAddSubMenu("Timer", menu_timer);
	glutAddMenuEntry("Route_1", 4);
	glutAddMenuEntry("Route_2", 5);
	glutAddMenuEntry("Route_3", 6);
	glutAddMenuEntry("Pause", 7);
	glutAddMenuEntry("Resume", 8);
	glutAddMenuEntry("Exit", MENU_EXIT);

	glutSetMenu(menu_timer);
	glutAddMenuEntry("Start", MENU_TIMER_START);
	glutAddMenuEntry("Stop", MENU_TIMER_STOP);

	glutSetMenu(menu_main);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// Register GLUT callback functions.
	glutDisplayFunc(My_Display);
	glutReshapeFunc(My_Reshape);
	glutMouseFunc(My_Mouse);
	glutMotionFunc(My_MotionMouse);
	glutKeyboardFunc(My_Keyboard);
	glutSpecialFunc(My_SpecialKeys);
	glutTimerFunc(timer_speed, My_Timer, 0);

	// Enter main event loop.
	glutMainLoop();

	return 0;
}

