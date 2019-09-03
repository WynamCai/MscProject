#include "pch.h"
#include <iostream>

#include<glad/glad.h>
#include<GLFW/glfw3.h>


#include"shader.h"
#include"model.h"
#include"camera.h"
#include"texture.h"
#include"scatteringshader.h"
#include"shadowMapShader.h"

//Constants
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float EarthRadius = 6360000.0;
const float AtomosphereRadius = 6420000.0;
glm::mat4 biasMatrix(glm::vec4(0.5f, 0.f, 0.f, 0.f),
	glm::vec4(0.f, 0.5f, 0.f, 0.f),
	glm::vec4(0.f, 0.f, 0.5f, 0.f),
	glm::vec4(0.5f, 0.5f, 0.5f, 1.f));


//Shader Path
const std::string EARTH_PATH = "models/sky.obj";
const std::string LAND_PATH = "models/M1.obj";
const std::string SKY_PATH = "models/sky.obj";

const char* earthVert = "shaders/earth.vert";
const char* earthFrag = "shaders/earth.frag";
const char* earthTexPath = "texture/grass.jpg";

const char* mountVert = "shaders/mountain.vert";
const char* mountFrag = "shaders/mountain.frag";
const char* mountTexPath= "texture/mountain.jpg";

const char* skyVert = "shaders/sky.vert";
const char* skyFrag = "shaders/sky.frag";

const char* smVert = "shaders/shadowmap.vert";
const char* smFrag = "shaders/shadowmap.frag";

//Camera Setting
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT,"Scattering",glfwGetPrimaryMonitor(),NULL);
	//GLFWwindow* window = glfwCreateWindow(800, 600, "test",0,0);
	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	//Earth
	Shader earthShader(earthVert, earthFrag);
	Model earth = Model(EARTH_PATH);
	Texture earthTex = Texture(earthTexPath);
	earthShader.use();
	earthShader.setInt("texture1", 0);

	//Mount Terrain
	Shader mountShader(mountVert, mountFrag);
	Model mountain = Model(LAND_PATH);
	Texture mountTex = Texture(mountTexPath);
	mountShader.use();
	mountShader.setInt("texture0", 0);
	//mountShader.setInt("depthmap", 7);

	//Sky
	Shader skyShader(skyVert, skyFrag);
	Model sky = Model(SKY_PATH);
	scatteringShader scShader(&skyShader);
	skyShader.use();
	skyShader.setInt("rayleighDensity", 6);
	skyShader.setInt("mieDensity", 5);
	skyShader.setInt("texture_diffuse", 0);
	skyShader.setInt("texture_normalmap", 2);
	skyShader.setInt("uniform sampler2D shadowMap", 7);

	//ShadowMap
	Shader SMshader(smVert, smFrag);
	ShadowMapShader myShadowMapping(SMshader);

	myShadowMapping.init();

	glm::mat4 mod = glm::identity<glm::mat4>();
	//Terrain Model
	glm::mat4 modelMountain = glm::translate(mod, glm::vec3(0, 0, 0))*glm::scale(mod, glm::vec3(20, 20, 20));
	//Earth model
	glm::mat4 modelSphere = glm::translate(mod, glm::vec3(0, -EarthRadius, 0))* glm::scale(mod, glm::vec3(EarthRadius, EarthRadius, EarthRadius));
	//Sky Model
	glm::mat4 modelSky = glm::translate(mod, glm::vec3(0, -EarthRadius, 0))* glm::scale(mod, glm::vec3(AtomosphereRadius, AtomosphereRadius, AtomosphereRadius));

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glm::vec3 sunDir = glm::vec3(sin(currentFrame*glm::radians(1.0f)), - cos(currentFrame*glm::radians(1.0f)), 0);
		glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 1.0f, 1400000.0f);
		glm::mat4 view = camera.GetViewMatrix();
		//glm::mat4 lightProjection = glm::ortho(-10000.0f, 10000.0f, -10000.0f, 10000.0f, 0.0f, 10000.0f);
		//glm::mat4 lightLookAt = glm::lookAt(glm::vec3(AtomosphereRadius,0,0), glm::vec3(0, -EarthRadius, 0), glm::vec3(0, 1, 0));
		glm::mat4 spaceMatrix = myShadowMapping.sunViewMatrix(sunDir, glm::vec3(0.0f, 1.0f, 0.0001f), glm::vec3(0, -EarthRadius, 0), 10000.0f);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
		myShadowMapping.use();
		SMshader.setMat4("lightSpaceMatrix", spaceMatrix);
		//rednerScene(SMshader);
		//earthTex.activateTex();
		//SMshader.setMat4("model", modelSphere);
		//earth.Draw(earthShader);

		mountTex.activateTex();

		SMshader.setMat4("model", modelMountain);
		mountain.Draw(mountShader);

		scShader.activateTex();
		SMshader.setMat4("model", modelSky);
		sky.Draw(skyShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		/*earthTex.activateTex();
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, myShadowMapping.depthTexture);
		earthShader.use();
		earthShader.setMat4("proj", proj);
		earthShader.setMat4("view", view);
		earthShader.setMat4("model", modelSphere);
		earth.Draw(earthShader);*/

		mountTex.activateTex();
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, myShadowMapping.depthTexture);
		mountShader.use();
		mountShader.setMat4("view", view);
		mountShader.setMat4("model", modelMountain);
		mountShader.setMat4("proj", proj);
		mountain.Draw(mountShader);

		scShader.activateTex();
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, myShadowMapping.depthTexture);
		skyShader.use();
		skyShader.setMat4("view", view);
		skyShader.setMat4("model", modelSky);
		skyShader.setMat4("proj", proj);
		skyShader.setVec3("cam", camera.Position);
		skyShader.setVec3("sunDir", sunDir);
		sky.Draw(skyShader);
		

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	return 0;
}

//void rednerScene(Shader &inShader)
//{
//	earthTex.activateTex();
//	inShader.setMat4("model", modelSphere);
//	earth.Draw(earthShader);
//
//	mountTex.activateTex();
//	inShader.setMat4("model", modelMountain);
//	mountain.Draw(mountShader);
//
//	scShader.activateTex();
//	inShader.setMat4("model", modelSky);
//	sky.Draw(skyShader);
//}

