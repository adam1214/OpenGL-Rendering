/*
CG HW1
Please fill the functions whose parameters are replaced by ... in the following function
static int add_obj(unsigned int program, const char *filename)
static void render() (line 202)

For example : //glGenVertexArrays(...);

*/

#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS

float aspect;
GLint modelLoc;
GLint projLoc;

struct object_struct {
	unsigned int program;
	unsigned int vao;
	unsigned int vbo[3];
	unsigned int ebo;
	unsigned int texture;
	glm::mat4 model;
};

std::vector<object_struct> objects;//vertex array object,vertex buffer object and texture(color) for objs
unsigned int program;
std::vector<int> indicesCount;//Number of indice of objs

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static unsigned int setup_shader(const char *vertex_shader, const char *fragment_shader)
{
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const GLchar**)&vertex_shader, nullptr);

	glCompileShader(vs);	//compile vertex shader

	int status, maxLength;
	char *infoLog = nullptr;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);		//get compile status
	if (status == GL_FALSE)								//if compile error
	{
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);	//get error message length

															/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(vs, maxLength, &maxLength, infoLog);		//get error message

		printf("Vertex Shader Error: %s\n", infoLog);

		delete[] infoLog;
		return 0;
	}
	//	for fragment shader --> same as vertex shader
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const GLchar**)&fragment_shader, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(fs, maxLength, &maxLength, infoLog);

		printf("Fragment Shader Error: %s\n", infoLog);

		delete[] infoLog;
		return 0;
	}

	unsigned int program = glCreateProgram();
	// Attach our shaders to our program
	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);


		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];
		glGetProgramInfoLog(program, maxLength, NULL, infoLog);

		glGetProgramInfoLog(program, maxLength, &maxLength, infoLog);

		printf("Link Error: %s\n", infoLog);

		delete[] infoLog;
		return 0;
	}
	return program;
}

static std::string readfile(const char *filename)
{
	std::ifstream ifs(filename);
	if (!ifs)
		exit(EXIT_FAILURE);
	return std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());
}
// mini bmp loader written by HSU YOU-LUN
static unsigned char *load_bmp(const char *bmp, unsigned int *width, unsigned int *height, unsigned short int *bits)
{
	unsigned char *result = nullptr;
	FILE *fp = fopen(bmp, "rb");
	if (!fp)
		return nullptr;
	char type[2];
	unsigned int size, offset;
	// check for magic signature
	fread(type, sizeof(type), 1, fp);
	if (type[0] == 0x42 || type[1] == 0x4d) {
		fread(&size, sizeof(size), 1, fp);
		// ignore 2 two-byte reversed fields
		fseek(fp, 4, SEEK_CUR);
		fread(&offset, sizeof(offset), 1, fp);
		// ignore size of bmpinfoheader field
		fseek(fp, 4, SEEK_CUR);
		fread(width, sizeof(*width), 1, fp);
		fread(height, sizeof(*height), 1, fp);
		// ignore planes field
		fseek(fp, 2, SEEK_CUR);
		fread(bits, sizeof(*bits), 1, fp);
		unsigned char *pos = result = new unsigned char[size - offset];
		fseek(fp, offset, SEEK_SET);
		while (size - ftell(fp)>0)
			pos += fread(pos, 1, size - ftell(fp), fp);
	}
	fclose(fp);
	return result;
}
static int add_obj(unsigned int program, const char *filename, const char *texbmp)
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, filename);

	if (!err.empty() || shapes.size() == 0)
	{
		std::cerr << err << std::endl;
		exit(1);
	}
	object_struct new_node;

	//Generate memory for buffers.
	glGenVertexArrays(1, &new_node.vao);
	glGenBuffers(3, new_node.vbo);
	glGenBuffers(1, &new_node.ebo);
	glGenTextures(1, &new_node.texture);

	for (int i = 0; i < shapes.size(); i++)
	{
		//Tell the program which VAO you are going to modify
		glBindVertexArray(new_node.vao);

		// Upload postion array
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * shapes[i].mesh.positions.size(), shapes[i].mesh.positions.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		if (shapes[i].mesh.normals.size() > 0)
		{
			// Upload normal array
			glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[1]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * shapes[i].mesh.normals.size(), shapes[i].mesh.normals.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}

		if (shapes[i].mesh.texcoords.size() > 0)
		{
			// Upload texCoord array
			glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[2]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * shapes[i].mesh.texcoords.size(), shapes[i].mesh.texcoords.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
			//glActiveTexture(GL_TEXTURE0);	//Activate texture unit before binding texture, used when having multiple texture

			glBindTexture(GL_TEXTURE_2D, new_node.texture);
			unsigned int width, height;
			unsigned short int bits;
			unsigned char *bgr = load_bmp(texbmp, &width, &height, &bits);
			GLenum format = (bits == 24 ? GL_BGR : GL_BGRA);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, bgr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glGenerateMipmap(GL_TEXTURE_2D);
			delete[] bgr;
		}

		// Setup index buffer for glDrawElements(ebo)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_node.ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * shapes[i].mesh.indices.size(), shapes[i].mesh.indices.data(), GL_STATIC_DRAW);
		indicesCount.push_back(shapes[i].mesh.indices.size());

		glBindVertexArray(0);

		new_node.program = program;

		objects.push_back(new_node);
	}
	return objects.size() - 1;
}

static void releaseObjects()
{
	for (int i = 0; i<objects.size(); i++) {
		glDeleteVertexArrays(1, &objects[i].vao);
		glDeleteTextures(1, &objects[i].texture);
		glDeleteBuffers(3, objects[i].vbo);
		glDeleteBuffers(1, &objects[i].ebo);
	}
	glDeleteProgram(program);
}

static void render()
{
	float scale = 20.0;
	float time = glfwGetTime() / 100.0f;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);
	glm::mat4 proj_matrix, model_matrix, view_matrix, eye(1.0f);

	//set camera matrix
	proj_matrix = glm::perspective(glm::radians(45.0f), (float)800 / (float)600, 0.1f, 100.0f);
	view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 30.0f), glm::vec3(), glm::vec3(0, 1, 0)) * glm::mat4(1.0f);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj_matrix*view_matrix));

	for (int i = 0; i<objects.size(); i++) {

		//Bind VAO
		glBindVertexArray(objects[i].vao);

		//If you don't want to rotate or move your object, you can comment the functions below.

		model_matrix = glm::scale(model_matrix, glm::vec3(0.1, 0.1, 0.1))
			*glm::translate(eye, glm::vec3(0.0f))
			* glm::rotate(eye, 98.70f * time, glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::rotate(eye, 123.40f * time, glm::vec3(1.0f, 0.0f, 0.0f));
		

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model_matrix));

		//Draw object
		glDrawElements(GL_TRIANGLES, indicesCount[i], GL_UNSIGNED_INT, 0);
		printf("222\n");
	}

	//Unbind VAO
	glBindVertexArray(0);
}

static void reshape(GLFWwindow* window, int width, int height)
{
	aspect = (float)width / height;
	glViewport(0, 0, width, height);
}

void init_shader()
{
	modelLoc = glGetUniformLocation(program, "model");
	projLoc = glGetUniformLocation(program, "proj");
}

int main()
{
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}
	// OpenGL 3.3, Mac OS X is reported to have some problem. However I don't have Mac to test
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);		//set hint to glfwCreateWindow, (target, hintValue)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//hint--> window not resizable,  explicit use core-profile,  opengl version 3.3
	// For Mac OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(800, 600, "Simple Example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwSetFramebufferSizeCallback(window, reshape);
	reshape(window, 800, 600);
	glfwMakeContextCurrent(window);	//set current window as main window to focus

									// This line MUST put below glfwMakeContextCurrent
	glewExperimental = GL_TRUE;		//tell glew to use more modern technique for managing OpenGL functionality
	glewInit();

	// Enable vsync
	glfwSwapInterval(1);

	// Setup input callback
	glfwSetKeyCallback(window, key_callback);	//set key event handler
												// load shader program
	program = setup_shader(readfile("light.vert").c_str(), readfile("light.frag").c_str());
	//program = setup_shader(readfile("vs.txt").c_str(), readfile("fs.txt").c_str());
	init_shader();

	add_obj(program, "table.obj", "sun.bmp");

	glEnable(GL_DEPTH_TEST);
	// prevent faces rendering to the front while they're behind other faces.
	glCullFace(GL_BACK);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	while (!glfwWindowShouldClose(window))
	{//program will keep draw here until you close the window
		printf("777\n");
		render();
		glfwSwapBuffers(window);	//swap the color buffer and show it as output to the screen.
		glfwPollEvents();			//check if there is any event being triggered
	}
	releaseObjects();
	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}