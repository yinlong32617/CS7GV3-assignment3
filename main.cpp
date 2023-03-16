// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "stb_image.h"
#include "maths_funcs.h"
#include "camera.h"
#include "shader.h"

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
//#define MESH_NAME "D:/Programming Workplace/ComputerGraphics/assimp-demo/monkeyhead_smooth.dae"

#define MESH_NAME "african_head/african_head.obj" //0.1
float s = 0.1;

Shader shader1;
Shader shader2;

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct ModelData
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
	std::vector<vec3> mTangents;
	std::vector<vec3> mBitangents;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;

ModelData mesh_data;

int scr_width = 1000;
int scr_height = 600;

Camera camera(vec3(0, 0, 0.1));
vec3 lightPos(0.0f, 1.0f, 1.0f);

float lastX = scr_width / 2.0f;
float lastY = scr_height / 2.0f;
bool firstMouse = true;

int deltaTime = 0;
int lastFrame = 0;

//GLuint loc1, loc2, loc3, loc4, loc5;;
GLfloat rotate_x = 0.0f;
GLfloat rotate_y = 0.0f;
GLfloat rotate_z = 0.0f;

unsigned int VAO, vpVBO, vnVBO, vtVBO, vtanVBO, vbitVBO;
unsigned int diffuseTexture;
unsigned int normalMapTexture;

unsigned int loadTexture(const char* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
				const aiVector3D* vtan = &(mesh->mTangents[v_i]);
				const aiVector3D* vbit = &(mesh->mBitangents[v_i]);
				modelData.mTangents.push_back(vec3(vtan->x, vtan->y, vtan->z));
				modelData.mBitangents.push_back(vec3(vbit->x, vbit->y, vbit->z));
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh() {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.
	//loc1 = glGetAttribLocation(shader1.ID, "vertex_position");
	//loc2 = glGetAttribLocation(shader1.ID, "vertex_normal");
	//loc3 = glGetAttribLocation(shader1.ID, "vertex_texCoords");
	//loc4 = glGetAttribLocation(shader1.ID, "vertex_tangent");
	//loc5 = glGetAttribLocation(shader1.ID, "vertex_bitangent");

	mesh_data = load_mesh(MESH_NAME);
	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &vpVBO);
	glGenBuffers(1, &vnVBO);
	glGenBuffers(1, &vtVBO);
	glGenBuffers(1, &vtanVBO);
	glGenBuffers(1, &vbitVBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, vpVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mVertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vnVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mNormals[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vtVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec2), &mesh_data.mTextureCoords[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vtanVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mTangents[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbitVBO);
	glBufferData(GL_ARRAY_BUFFER, mesh_data.mPointCount * sizeof(vec3), &mesh_data.mBitangents[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vpVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vnVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, vtVBO);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, vtanVBO);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(4);
	glBindBuffer(GL_ARRAY_BUFFER, vbitVBO);
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, NULL);

}
#pragma endregion VBO_FUNCTIONS


void display() {
	int currentFrame = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	mat4 view = identity_mat4();
	mat4 persp_proj = perspective(camera.Zoom, (float)scr_width / (float)scr_height, 0.1f, 100.0f);
	view = camera.GetViewMatrix();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMapTexture);

	glBindVertexArray(VAO);

	// left
	shader1.use();

	shader1.setVec3("lightPos", lightPos);
	shader1.setVec3("viewPos", camera.Position);
	shader1.setMat4("proj", persp_proj);
	shader1.setMat4("view", view);
	
	mat4 model = identity_mat4();
	model = scale(model, vec3(s, s, s));
	//model = rotate_x_deg(model, rotate_x);
	model = rotate_y_deg(model, rotate_y);
	//model = rotate_z_deg(model, rotate_z);
	model = translate(model, vec3(-0.08, 0, -0.2));
	shader1.setMat4("model", model);
		
	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	// right
	shader2.use();

	shader2.setVec3("lightPos", lightPos);
	shader2.setVec3("viewPos", camera.Position);
	shader2.setMat4("proj", persp_proj);
	shader2.setMat4("view", view);

	model = identity_mat4();
	model = scale(model, vec3(s, s, s));
	//model = rotate_x_deg(model, rotate_x);
	model = rotate_y_deg(model, rotate_y);
	//model = rotate_z_deg(model, rotate_z);
	model = translate(model, vec3(0.08, 0, -0.2));
	//glUniformMatrix4fv(matrix_location, 1, GL_FALSE, model.m);
	shader2.setMat4("model", model);

	glDrawArrays(GL_TRIANGLES, 0, mesh_data.mPointCount);

	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	// Rotate the model slowly around the y axis at 20 degrees per second
	rotate_y += 20.0f * delta;
	//rotate_y = fmodf(rotate_y, 360.0f);

	rotate_x += 20.0f * delta;
	//rotate_x = fmodf(rotate_x, 360.0f);

	rotate_z += 20.0f * delta;
	//rotate_z = fmodf(rotate_z, 360.0f);

	// Draw the next frame
	glutPostRedisplay();
}

//setup keyboard controlling function here

void init()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Set up the shaders
	shader1.CompileShader("PhoneVertexShader.txt","PhoneFragmentShader.txt");
	shader2.CompileShader("NormalMappingVertexShader.txt", "NormalMappingFragmentShader.txt");

	// load mesh into a vertex buffer array
	generateObjectBufferMesh();

	stbi_set_flip_vertically_on_load(true);
	diffuseTexture = loadTexture("african_head/african_head_diffuse.tga");
	normalMapTexture = loadTexture("african_head/african_head_nm.tga");

	shader1.use();
	shader1.setInt("diffuseMap", 0);
	shader1.setInt("normalMap", 1);

	shader2.use();
	shader2.setInt("diffuseMap", 0);
	shader2.setInt("normalMap", 1);
}

void keypress(unsigned char key, int x, int y) {
	float delta = deltaTime/1000.0;
	if (key == 27) {
		exit(0);
	}
	else if (key == 'w') {
		camera.ProcessKeyboard(FORWARD, delta);
	}
	else if (key == 's') {
		camera.ProcessKeyboard(BACKWARD, delta);
	}
	else if (key == 'a') {
		camera.ProcessKeyboard(LEFT, delta);
	}
	else if (key == 'd') {
		camera.ProcessKeyboard(RIGHT, delta);
	}
}

void PassiveMouse_Callback(int x, int y) {

	float xpos = static_cast<float>(x);
	float ypos = static_cast<float>(y);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
	
}

int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(scr_width, scr_height);
	glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	//glutPassiveMotionFunc(PassiveMouse_Callback);
		
	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
