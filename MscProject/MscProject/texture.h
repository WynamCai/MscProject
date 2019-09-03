#include<iostream>

#define STB_IMAGE_IMPLEMENTATION
#include<stb-master/stb_image.h>

#include<glad/glad.h>

class Texture
{
public:
	unsigned int texture;

	Texture(const char* file_path)
	{
	
		glGenTextures(1, &texture);
		
		int width, height, nrChannels;
		//stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(file_path, &width, &height, &nrChannels, 0);
		if (data)
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);
	}
	~Texture()
	{
	}

	void activateTex()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
	}
private:

};
