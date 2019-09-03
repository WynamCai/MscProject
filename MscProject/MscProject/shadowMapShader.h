#pragma once
#include"shader.h"

class ShadowMapShader
{
public:
	Shader SMshader;
	GLuint framebufferName;
	GLuint depthTexture;

	int viewportSize = 4096;
	ShadowMapShader(Shader &inShader)
	{
		SMshader = inShader;
	}

	~ShadowMapShader() {}

	void init()
	{
		glGenFramebuffers(1, &framebufferName);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferName);
		glGenTextures(1, &depthTexture);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, viewportSize, viewportSize,
			0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

		glDrawBuffer(GL_NONE);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	void use()
	{
		glActiveTexture(GL_TEXTURE7);
		SMshader.use();
		glViewport(0, 0, viewportSize, viewportSize);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferName);
		
	}


	glm::mat4 sunViewMatrix(glm::vec3 lightDir, glm::vec3 up, glm::vec3 cEarth, float distance)
	{
		glm::mat4 ortho = glm::identity<glm::mat4>();
		GLfloat halfWidth = 10000.0f;
		GLfloat halfHeight = 10000.0f;
		GLfloat zNear = 0.0f;
		GLfloat zFar = 100000.0f;

		ortho[0][0] = 1 / halfWidth;// 100.0f / right;
		ortho[1][1] = 1 / halfHeight;// 100.0f / top;
		ortho[2][2] = -2.0f / (zFar - zNear);
		ortho[3][2] = -(zFar + zNear) / (zFar - zNear);
		ortho[3][3] = 1.0f;

		glm::vec3 a(0.0f, 0.0f, -1.0f);
		glm::vec3 v = glm::cross(a, lightDir);
		float s = glm::length(v);
		float c = glm::dot(a, lightDir);

		glm::mat4 vx({ glm::vec4(0.0f, -v[2], v[1], 0.0f),
						 glm::vec4(v[2], 0.0f, -v[0], 0.0f),
						 glm::vec4(-v[1], v[0], 0.0f, 0.0f),
						 glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) });

		float aux = ((1 - c) / (s*s));
		glm::mat4 vx2 = (vx*vx);
		for (int y = 0; y < 4; y++)
			for (int x = 0; x < 4; x++)
				vx2[x][y] *= aux;

		glm::mat4 r = glm::identity<glm::mat4>() + vx + vx2;

		glm::mat4 suntrans = glm::translate(glm::identity<glm::mat4>(),cEarth) * glm::translate(glm::identity<glm::mat4>(),-lightDir * distance);
		suntrans = glm::transpose(suntrans);
		glm::vec4 sunPos(0.0f, 0.0f, 0.0f, 1.0f);

		for (int x = 0; x < 4; x++)	sunPos[x] = glm::dot(suntrans[x], sunPos);

		glm::vec3 sunPosSimple(sunPos[0], sunPos[1], sunPos[2]);
		glm::mat4 lookAt;
		glm::vec3 X, Y, Z;

		Z = sunPosSimple - cEarth;
		Z = glm::normalize(Z);
		Y = up;
		X = glm::cross(Y, Z);

		Y = glm::cross(Z, X);

		X = glm::normalize(X);
		Y = glm::normalize(Y);

		lookAt[0][0] = X[0];
		lookAt[1][0] = X[1];
		lookAt[2][0] = X[2];
		lookAt[3][0] = glm::dot(-X, -lightDir);
		lookAt[0][1] = Y[0];
		lookAt[1][1] = Y[1];
		lookAt[2][1] = Y[2];
		lookAt[3][1] = glm::dot(-Y, -lightDir);
		lookAt[0][2] = Z[0];
		lookAt[1][2] = Z[1];
		lookAt[2][2] = Z[2];
		lookAt[3][2] = glm::dot(-Z, -lightDir);
		lookAt[0][3] = 0.0f;
		lookAt[1][3] = 0.0f;
		lookAt[2][3] = 0.0f;
		lookAt[3][3] = 1.0f;


		//GLfloat aspect = float(DEFAULT_WIN_HEIGHT) / float(DEFAULT_WIN_WIDTH);
		ortho = ortho *
			lookAt * glm::translate(glm::identity<glm::mat4>(),-sunPosSimple);// * r;
		// -5000.0f, -8000.0f, -5000.0f);

		return ortho;
	}
private:

};
