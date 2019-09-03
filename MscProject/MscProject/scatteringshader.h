#include"shader.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include<stb-master/stb_image_write.h>

class scatteringShader
{
public:
	//Shader shader;
	unsigned int rayleighDensity;
	unsigned int mieDensity;
	

	scatteringShader(Shader* inShader)
	{
		
		//rayleighDensity = glGetUniformLocation(inShader->ID, "rayleighDensity");
		//mieDensity = glGetUniformLocation(inShader->ID, "mieDensity");
		//glUniform1i(rayleighDensity, 6);
		//(mieDensity, 5);

		
		createHeightMap();
		glGenTextures(2, scatterMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, scatterMap[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthR, heightR, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, scatterMap[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, widthM, heightM, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataM);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	void activateTex()
	{
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, scatterMap[0]);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, scatterMap[1]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	~scatteringShader()
	{}

private:
	unsigned int scatterMap[2];
	const float Earth_Radius = 6360000.0;
	const glm::vec3 Earth_Center = { 0,-Earth_Radius,0 };
	const float Atmo_Radius = 6420000.0;
	const float Atmo_Radius_sq = 41216400000000.0;

	const char* tex1_path = "texture/white.png";
	const char* tex2_path = "texture/white2.png";

	int widthR, heightR, nChannel;
	int widthM, heightM;
	unsigned char* dataR;
	unsigned char* dataM;
	void createHeightMap()
	{
		
		dataR = stbi_load(tex1_path, &widthR, &heightR, &nChannel, 0);
		dataM = stbi_load(tex2_path, &widthM, &heightM, &nChannel, 0);
		float atmHeight = Atmo_Radius - Earth_Radius;
		float HR = 7994.0;
		float HM = 1200.0;
		float P0 = 1.0f;
		float N_STEPS = 100;

		uint32_t *pixR = (uint32_t *)dataR;
		uint32_t *pixM = (uint32_t *)dataM;
		for (int i = 0; i < widthR; i++)
		{
			float initialHeight = ((Atmo_Radius - Earth_Radius)*i) / widthR;
			float point_earth = initialHeight + Earth_Radius;
			glm::vec3 point =glm::vec3 (0.0f, point_earth, 0.0f);
			for (int j = 0; j < heightR; j++)
			{
				float density_AP[2] = { 0.0, 0.0 };
				float cosTheta = (2.0f * j / (heightR - 1)) - 1.0f;
				float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
				glm::vec3 lightDir = { sinTheta, -cosTheta,0 };
				glm::vec3 t1, t2;

				bool isInter = intersection(point, point + (-lightDir * 15000000.0f), t1, t2);
				
				float diferential_A = glm::distance(t1, t2) / N_STEPS;
				glm::vec3 delta_A = -lightDir * diferential_A;

				for (float step = 0.5; step < N_STEPS; step += 1.0f)
				{
					float hPoint = glm::length(point + delta_A * step) - Earth_Radius;
					float relation[2] = { -hPoint / HR, -hPoint / HM };
					density_AP[0] += P0 * exp(relation[0]) * diferential_A;
					density_AP[1] += P0 * exp(relation[1]) * diferential_A;
				}
				pixR[j*widthR + i] = (uint32_t)density_AP[0];
				pixM[j*widthR + i] = (uint32_t)density_AP[1];
			}
		}
		stbi_write_bmp("output1.bmp", widthR, heightR, nChannel, dataR);
		stbi_write_bmp("output2.bmp", widthM, heightM, nChannel, dataM);
	}
	
	bool intersection(glm::vec3 p1, glm::vec3 p2, glm::vec3 &t1, glm::vec3 &t2)
	{
		glm::vec3 rayD = glm::normalize(p2 - p1);
		glm::vec3 oc = p1 - Earth_Center;

		float b = 2.0f * glm::dot(rayD, oc);
		float c = glm::dot(oc, oc) - Atmo_Radius_sq;
		float delta = b * b - 4.0f*c;

		t1 = p1;
		t2 = p2;

		if (delta < 0.0f) return false;

		float d0 = (-b - sqrtf(delta)) / 2.0f;
		float d1 = (-b + sqrtf(delta)) / 2.0f;

		if (d0 > d1) {
			float aux = d0;
			d0 = d1;
			d1 = aux;
		}

		if (d1 < 0.0f) return false;

		t1 = fmax(d0, 0.0f) * rayD + p1;
		t2 = (d1 > glm::distance(p1, p2)) ? p2 : d1 * rayD + p1;
		return true;
	}
};