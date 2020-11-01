#include "FontRenderer.h"
#include <cstdlib>
namespace gekRender { // Makes things easier

BMPFontRenderer::BMPFontRenderer(std::string filepath, unsigned int x_screen_width, unsigned int y_screen_height, float scaling_factor,
								 std::string Shader_location) {
	// std::cout << "\nEntered constructor of BMPFontRenderer!!!" << std::endl;
	// If it's already been initialized, delete everything
	if (!isNull) {
		if (Screen) {
			delete Screen;
			Screen = nullptr;
		}
		if (BMPFont) {
			delete BMPFont;
			BMPFont = nullptr;
		}
		if (Screenquad_Mesh) {
			delete Screenquad_Mesh;
			Screenquad_Mesh = nullptr;
		}
		if (Screenquad_Shader) {
			delete Screenquad_Shader;
			Screenquad_Shader = nullptr;
		}
		if (Buffer)
			delete Buffer;
		BMPFontWidth = 0;
		BMPFontHeight = 0;
		screen_width = 0;
		screen_height = 0;
		my_scaling_factor = 1.0;
		isNull = true;
	}
	// First: Setup the bmp font with the passthrough
	int temp_width;
	int temp_height;
	int temp_components;
	int temp_something = 0; // no idea
	BMPFont = Texture::stbi_load_passthrough((char*)filepath.c_str(), &temp_width, &temp_height, &temp_components, temp_something);
	if (BMPFont != nullptr) {
		BMPFontWidth = temp_width;
		BMPFontHeight = temp_height;
		char_width = BMPFontWidth / 8;
		char_height = char_width;
		// std::cout << "\nBMPFONT INFO\nWidth: " << BMPFontWidth << " \nHeight:
		// "
		// << BMPFontHeight;
		num_components_BMPFont = temp_components;
	} else {
		std::cout << "\nERROR!!! PROBLEM LOADING FILE!!!" << std::endl;
		std::abort();
		std::cout << "\nThe program didn't crash. HOW?!?!" << std::endl;
	}
	// Second: Setup the screen texture
	isNull = false; // we have to do this before resize
	resize(x_screen_width, y_screen_height, scaling_factor);

	// Third: Prepare to render Screen quads
	if (Screenquad_Mesh == nullptr) {
		// Set up the positions
		screenquad_IndexedModel.positions.push_back(glm::vec3(-1, -1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(-1, 1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(1, 1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(1, -1, 0));
		// Set up the index
		// First Triangle
		screenquad_IndexedModel.indices.push_back(0);
		screenquad_IndexedModel.indices.push_back(2);
		screenquad_IndexedModel.indices.push_back(3);
		// Second triangle
		screenquad_IndexedModel.indices.push_back(0);
		screenquad_IndexedModel.indices.push_back(1);
		screenquad_IndexedModel.indices.push_back(2);
		// Put in some bullshit for the texcoords and normals, we don't need
		// them and if this works we'll remove them later
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		// We also need bullshit for the normals
		screenquad_IndexedModel.normals.push_back(glm::vec3(1, -1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(2, 1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(3, 1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(4, -1, 0));

		// std::cout<<"\n We got the indexed model done";
		// Make the mesh
		Screenquad_Mesh = new Mesh(screenquad_IndexedModel, false, true, true);
		// std::cout<<"\n we got the mesh made";
		// Make the camera needed to render the screenquad
		Camera* ScreenquadCamera = nullptr;
		ScreenquadCamera = new Camera();
		ScreenquadCamera->buildOrthogonal(-1, 1, -1, 1, 0, 1);
		ScreenquadCamera->pos = glm::vec3(0, 0, 0);
		ScreenquadCamera->forward = glm::vec3(0, 0, 1);
		ScreenquadCamera->up = glm::vec3(0, 1, 0);
		Screenquad_CameraMatrix = ScreenquadCamera->getViewProjection();
		delete ScreenquadCamera;
	}
	// fourth: setup the shader
	Screenquad_Shader = new Shader(Shader_location);
	Screenquad_Shader->bind();
	diffuse_loc = Screenquad_Shader->getUniformLocation("diffuse");
	cam_loc = Screenquad_Shader->getUniformLocation("World2Camera");
	isNull = false;
	// std::cout << "\nFinished constructor of BMPFontRenderer!!!" << std::endl;
} // eof constructor that uses a string path

BMPFontRenderer::BMPFontRenderer(unsigned char* _BMPFont, unsigned int _BMPFont_Width, unsigned int _BMPFont_Height, unsigned int _num_components_BMPFont,
								 unsigned int x_screen_width, unsigned int y_screen_height, float scaling_factor, std::string Shader_location) {
	// std::cout << "\nEntered constructor of BMPFontRenderer!!!" << std::endl;
	// If it's already been initialized, delete everything
	if (!isNull) {
		if (Screen) {
			delete Screen;
			Screen = nullptr;
		}
		if (BMPFont) {
			delete BMPFont;
			BMPFont = nullptr;
		}
		if (Screenquad_Mesh) {
			delete Screenquad_Mesh;
			Screenquad_Mesh = nullptr;
		}
		if (Screenquad_Shader) {
			delete Screenquad_Shader;
			Screenquad_Shader = nullptr;
		}
		if (Buffer)
			delete Buffer;
		BMPFontWidth = 0;
		BMPFontHeight = 0;
		screen_width = 0;
		screen_height = 0;
		my_scaling_factor = 1.0;
		isNull = true;
	}
	// First: Setup the bmp font with the passthrough
	//~ int temp_width;
	//~ int temp_height;
	//~ int temp_components;
	//~ int temp_something = 4; //no idea
	//~ BMPFont = Texture::stbi_load_passthrough((char*)filepath.c_str(),
	//&temp_width, &temp_height, &temp_components, temp_something);
	BMPFont = _BMPFont;
	if (BMPFont != nullptr) {
		BMPFontWidth = _BMPFont_Width;
		char_width = BMPFontWidth / 8;
		char_height = char_width;
		BMPFontHeight = _BMPFont_Height;
		num_components_BMPFont = _num_components_BMPFont;
	} else {
		std::cout << "\nERROR!!! PROBLEM LOADING FILE!!!" << std::endl;
		std::abort();
		std::cout << "\nThe program didn't crash. HOW?!?!" << std::endl;
	}
	// Second: Setup the screen texture
	isNull = false; // we have to do this before resize
	resize(x_screen_width, y_screen_height, scaling_factor);

	// Third: Prepare to render Screen quads
	if (Screenquad_Mesh == nullptr) {
		// Set up the positions
		screenquad_IndexedModel.positions.push_back(glm::vec3(-1, -1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(-1, 1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(1, 1, 0));
		screenquad_IndexedModel.positions.push_back(glm::vec3(1, -1, 0));
		// Set up the index
		// First Triangle
		screenquad_IndexedModel.indices.push_back(0);
		screenquad_IndexedModel.indices.push_back(2);
		screenquad_IndexedModel.indices.push_back(3);
		// Second triangle
		screenquad_IndexedModel.indices.push_back(0);
		screenquad_IndexedModel.indices.push_back(1);
		screenquad_IndexedModel.indices.push_back(2);
		// Put in some bullshit for the texcoords and normals, we don't need
		// them and if this works we'll remove them later
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		screenquad_IndexedModel.texCoords.push_back(glm::vec2(0, 0));
		// We also need bullshit for the normals
		screenquad_IndexedModel.normals.push_back(glm::vec3(1, -1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(2, 1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(3, 1, 0));
		screenquad_IndexedModel.normals.push_back(glm::vec3(4, -1, 0));

		// std::cout<<"\n We got the indexed model done";
		// Make the mesh
		Screenquad_Mesh = new Mesh(screenquad_IndexedModel, false, true, true);
		// std::cout<<"\n we got the mesh made";
		// Make the camera needed to render the screenquad
		Camera* ScreenquadCamera = nullptr;
		ScreenquadCamera = new Camera();
		ScreenquadCamera->buildOrthogonal(-1, 1, -1, 1, 0, 1);
		ScreenquadCamera->pos = glm::vec3(0, 0, 0);
		ScreenquadCamera->forward = glm::vec3(0, 0, 1);
		ScreenquadCamera->up = glm::vec3(0, 1, 0);
		Screenquad_CameraMatrix = ScreenquadCamera->getViewProjection();
		delete ScreenquadCamera;
	}
	// fourth: setup the shader
	Screenquad_Shader = new Shader(Shader_location);
	Screenquad_Shader->bind();
	diffuse_loc = Screenquad_Shader->getUniformLocation("diffuse");
	cam_loc = Screenquad_Shader->getUniformLocation("World2Camera");
	isNull = false;
	// std::cout << "\nFinished constructor of BMPFontRenderer!!!" << std::endl;
} // eof constructor that uses a pointer

// Related to constructing
void BMPFontRenderer::resize(unsigned int x_screen_width, unsigned int y_screen_height, float scaling_factor) {
	if (isNull) {
		std::cout << "\n CRITICAL FAIL!";
		return;
	}
	my_scaling_factor = scaling_factor;
	screen_width = x_screen_width * my_scaling_factor;
	screen_height = y_screen_height * my_scaling_factor;
	//                                                              1 RGBA width
	//                                                              height
	unsigned char* Temporary_Memory = (unsigned char*)malloc(sizeof(unsigned char) * 4 * screen_width * screen_height);
	// Make it black
	for (int i = 0; i < 4 * screen_width * screen_height; i++) {
		// Temporary_Memory[i] = rand()%256; //RANDOM!!!
		Temporary_Memory[i] = 0; // Black
	}
	if (Screen) {
		delete Screen;
		Screen = nullptr;
	}
	if (Buffer)
		free(Buffer);
	Buffer = nullptr;
	if (Stripe)
		free(Stripe);
	Stripe = nullptr;
	Stripe = (unsigned char*)malloc(sizeof(unsigned char) * 4 * screen_width);
	Screen = new Texture(screen_width, screen_height, 4, Temporary_Memory, GL_NEAREST, GL_NEAREST, GL_REPEAT, 1.0);
	Buffer = Temporary_Memory; // HA I saved a malloc!
}

void BMPFontRenderer::swapBuffers() { memcpy(Screen->getDataPointerNotConst(), Buffer, (sizeof(unsigned char) * 4 * screen_width * screen_height)); }

// the destructor
BMPFontRenderer::~BMPFontRenderer() {
	// The destructing part of the destructor
	if (!isNull) {
		if (Screen) {
			delete Screen;
			Screen = nullptr;
		}
		if (BMPFont) {
			free(BMPFont);
			BMPFont = nullptr;
		}
		if (Screenquad_Mesh) {
			delete Screenquad_Mesh;
			Screenquad_Mesh = nullptr;
		}
		if (Screenquad_Shader) {
			delete Screenquad_Shader;
			Screenquad_Shader = nullptr;
		}
		if (Buffer)
			free(Buffer);
		Buffer = nullptr;
		if (Stripe)
			free(Stripe);
		Stripe = nullptr;
		BMPFontWidth = 0;
		BMPFontHeight = 0;
		screen_width = 0;
		screen_height = 0;
		my_scaling_factor = 1.0;
		isNull = true;
	}
}

// Other functions

void BMPFontRenderer::draw(bool useBlending) {
	GLenum communism = glGetError();
	Screen->bind(0);
	Screenquad_Shader->bind();
	glUniformMatrix4fv(cam_loc, 1, GL_FALSE, &Screenquad_CameraMatrix[0][0]);
	glUniform1i(diffuse_loc, 0);
	GLuint m_handle = Screenquad_Mesh->getVAOHandle();
	glEnableVertexAttribArray(0);
	glBindVertexArray(m_handle);
	glDisable(GL_DEPTH_TEST);
	if (useBlending) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
				   0); // CHANGE BACK TO 6 AFTER WE HAVE THIS TEXTURE ISSUE SORTED OUT
	if (useBlending) {
		glDisable(GL_BLEND);
	}
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);

	communism = glGetError();
	if (communism != GL_NO_ERROR) {
		std::cout << "\n OpenGL reports an ERROR!";
		if (communism == GL_INVALID_ENUM)
			std::cout << "\n Invalid enum.";
		if (communism == GL_INVALID_OPERATION)
			std::cout << "\n Invalid operation.";
		if (communism == GL_INVALID_FRAMEBUFFER_OPERATION)
			std::cout << "\n Invalid Framebuffer Operation.";
		if (communism == GL_OUT_OF_MEMORY) {
			std::cout << "\n Out of memory. You've really messed up. How could you "
						 "do this?!?!";
			std::abort();
		}
	}
	// std::cout << "\nSuccessfully Rendering the Persistence layer!";
}
inline void BMPFontRenderer::writePixel(unsigned int x, unsigned int y, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
	if (x >= Screen->getMyWidth() || y >= Screen->getMyHeight())
		return; // Do not attempt to write a pixel if it's invalid NOTE: X and Y
				// will never be less than 0 because they're unsigned
	//[(x + width * y)*num_components + component index] gets you the byte
	//~ unsigned char* Target = &(Screen->getDataPointerNotConst()[(x + Screen->getMyWidth() * y) * 4]);
	unsigned char* Target = &(Buffer[(x + Screen->getMyWidth() * y) * 4]);
	// Set target
	Target[0] = red;
	Target[1] = green;
	Target[2] = blue;
	Target[3] = alpha;
}
void BMPFontRenderer::writeRectangle(int x1, int y1, int x2, int y2, unsigned char red, unsigned char green,
									 unsigned char blue, unsigned char alpha) {
	if (x1 == x2 && y1 == y2) // A single pixel
	{
		writePixel(x1, y1, red, green, blue, alpha);
		return;
	}
	int minX = (x1 < x2) ? x1 : x2;
	int maxX = (x1 > x2) ? x1 : x2;
	int minY = (y1 < y2) ? y1 : y2;
	int maxY = (y1 > y2) ? y1 : y2; // Note to self: if both y1 and y2 are the same...
											 // then they're the same and it doesn't matter
											 // which one we pick... don't use >=
	if (maxX > Screen->getMyWidth())
		maxX = Screen->getMyWidth();
	if (maxY > Screen->getMyHeight())
		maxY = Screen->getMyHeight();
	if (minX > Screen->getMyWidth())
		minX = Screen->getMyWidth();
	if (minY > Screen->getMyHeight())
		minY = Screen->getMyHeight();
		
	if (maxX < 0)
		maxX = 0;
	if (maxY < 0)
		maxY = 0;
	if (minX < 0)
		minX = 0;
	if (minY < 0)
		minY = 0;
	// new method using memcpy, writes horizontal stripes.
	unsigned char Mem[4];
	Mem[0] = red;
	Mem[1] = green;
	Mem[2] = blue;
	Mem[3] = alpha;

	//~ unsigned char* Stripe = nullptr;
	//~ Stripe = (unsigned char*)malloc(4 * (maxX - minX));
	for (unsigned int i = 0; i < 4 * (maxX - minX); i++)
		Stripe[i] = Mem[i % 4];
	//~ unsigned char* Target = Screen->getDataPointerNotConst();
	unsigned char* Target = Buffer;
	for (minY = minY; minY < maxY; minY++)
		memcpy(Target + minX * 4 + minY * 4 * Screen->getMyWidth(), Stripe, sizeof(Mem) * (maxX - minX));
	//~ free(Stripe);
}
void BMPFontRenderer::writeEllipse(int x, int y, float width, float height, float rotation, unsigned char red, unsigned char green, unsigned char blue,
								   unsigned char alpha) {
	// if(x < width || y < height) return; //If x is less than width, then x -
	// width will be less than 0 and loop around due to the effects of unsigned
	// int math
	if (width == 0 || height == 0)
		return;
	int minX = x - width;
	int maxX = x + width;
	int minY = y - height;
	int maxY = y + height;
	float rx = width;
	float ry = height;
	for (int w = minX; w < maxX; w++)
		for (int h = minY; h < maxY; h++)
			if (((float)w - (float)x) * ((float)w - (float)x) / (rx * rx) + ((float)h - (float)y) * ((float)h - (float)y) / (ry * ry) < 1)
				writePixel(w, h, red, green, blue, alpha);
}
void BMPFontRenderer::writeCircle(int x, int y, float radius, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
	writeEllipse(x, y, radius, radius, 0, red, green, blue, alpha);
}

float clamp(float val, float min, float max) {
	if (!(val < max))
		return max;
	if (!(val > min))
		return min;
	return val;
}

void BMPFontRenderer::writeImage(int x, int y, // Where in the buffer shall the bottom left corner be
								 unsigned char* Source, unsigned int width, unsigned int height,
								 unsigned int num_components, // Information about source image. if a component (such
															  // as alpha) is missing, then it is assumed to be 1
								 unsigned int subx1, unsigned int subx2, unsigned int suby1,
								 unsigned int suby2, // Where in the source image?
								 unsigned int targwidth,
								 unsigned int targheight, // How wide and how tall in the target?
								 bool flip_x, bool flip_y // Flip?
) {														  // NOTE: if you just want to do 1:1 scaling, you can write a version which
	// will get the compiler to do vectorization and therefore generate much
	// faster code, I may include this in a future version
	unsigned int minSourceX = (subx1 < subx2) ? subx1 : subx2;
	unsigned int maxSourceX = (subx1 > subx2) ? subx1 : subx2;
	unsigned int minSourceY = (suby1 < suby2) ? suby1 : suby2;
	unsigned int maxSourceY = (suby1 > suby2) ? suby1 : suby2;
	int minTargetX = x;
	int minTargetY = y;
	int maxTargetX = x + targwidth;
	int maxTargetY = y + targheight;
	// Avoid dividing by zero
	if (targwidth == 0)
		return;
	if (targheight == 0)
		return;

	for (int w = minTargetX; w <= maxTargetX; w++)
		for (int h = minTargetY; h <= maxTargetY; h++) {
			if (w < 0 || h < 0)
				continue;
			float PercentThroughWidth = (float)(w - minTargetX) / (float)(targwidth); // despite the name, it is not multiplied by
																					  // 100
			float PercentThroughHeight = (float)(h - minTargetY) / (float)(targheight);
			PercentThroughWidth = clamp(PercentThroughWidth, 0, 1);
			PercentThroughHeight = clamp(PercentThroughHeight, 0, 1);
			if (flip_x)
				PercentThroughWidth = 1.0 - PercentThroughWidth;
			if (!flip_y)
				PercentThroughHeight = 1.0 - PercentThroughHeight;
			// Do flipping here
			unsigned int Source_Width_Offset = maxSourceX * PercentThroughWidth + (1.0 - PercentThroughWidth) * minSourceX;
			unsigned int Source_Height_Offset = maxSourceY * PercentThroughHeight + (1.0 - PercentThroughHeight) * minSourceY;
			//~ if(Source_Width_Offset >= width || Source_Height_Offset >=
			// height) ~ {continue;}
			// we are now ready to find the exact pixel
			unsigned int sourceOffset = (Source_Width_Offset + width * Source_Height_Offset) * num_components;
			unsigned char* sourcePixelStartByte = &(Source[(Source_Width_Offset + width * Source_Height_Offset) * num_components]);
			// Extract R, G, B, and possibly A
			unsigned char red_source = sourcePixelStartByte[0];
			unsigned char green_source = red_source;
			unsigned char blue_source = red_source;
			unsigned char alpha_source = 0;
			if (num_components > 1)
				green_source = sourcePixelStartByte[1];
			if (num_components > 2)
				blue_source = sourcePixelStartByte[2];
			if (num_components > 3)
				alpha_source = sourcePixelStartByte[3];
			writePixel(w, h, red_source, green_source, blue_source, alpha_source);
		}
}

void BMPFontRenderer::writeCharacter(char Letter, int x,
									 int y, // Where in the buffer shall the bottom left corner be
									 unsigned int targwidth,
									 unsigned int targheight, // Width and Height in the target
									 glm::vec3 color_0_255, glm::vec3 backcolor_0_255, bool RenderBackground) {
	float Percent_Through_Width = 0.0; // TODO: Fix _ convention to non_
	float Percent_Through_Height = 0.0;
	int minTargetX = x;
	int minTargetY = y;
	int maxTargetX = x + targwidth;
	int maxTargetY = y + targheight;
	int charX = Letter % 8;
	int charY = Letter / 8;
	unsigned int charXOff = charX * char_width;
	unsigned int charYOff = charY * char_height;
	for (int w = minTargetX; w <= maxTargetX; w++)
		for (int h = minTargetY; h <= maxTargetY; h++) {
			if (w < 0 || h < 0)
				continue;
			Percent_Through_Width = ((float)(w - minTargetX)) / ((float)targwidth);
			Percent_Through_Height = ((float)(h - minTargetY)) / ((float)targheight);
			Percent_Through_Width = clamp(Percent_Through_Width, 0, 1);
			Percent_Through_Height = clamp(Percent_Through_Height, 0, 1);
			Percent_Through_Height = 1.0 - Percent_Through_Height;
			unsigned int Src_Value_Char = (unsigned int)getRedChar(clamp(Percent_Through_Width * (char_width - 0.5), 0, char_width - 1) + charXOff,
																   clamp(Percent_Through_Height * (char_height - 0.5), 0, char_height - 1) + charYOff);
			float Src_Value = ((float)Src_Value_Char) / 255.0f; // get it between 0 and 1
			unsigned char alpha = Src_Value_Char;
			unsigned char red = Src_Value * color_0_255.x + (1 - Src_Value) * backcolor_0_255.x;
			unsigned char green = Src_Value * color_0_255.y + (1 - Src_Value) * backcolor_0_255.y;
			unsigned char blue = Src_Value * color_0_255.z + (1 - Src_Value) * backcolor_0_255.z;
			if (!RenderBackground) {
				red = Src_Value * color_0_255.x;
				green = Src_Value * color_0_255.y;
				blue = Src_Value * color_0_255.z;
			}
			if (RenderBackground)
				writePixel(w, h, red, green, blue, 255);
			else if (Src_Value_Char > 0)
				writePixel(w, h, red, green, blue, Src_Value_Char);
		}
}

void BMPFontRenderer::writeString(std::string str, int x, int y, // Where in the buffer shall the bottom left
																 // corner of the first character be
								  unsigned int targwidth, unsigned int targheight, glm::vec3 color_0_255, glm::vec3 backcolor_0_255, bool RenderBackground) {
	unsigned int str_length = str.length();
	int x_start = x;
	int y_start = y;
	for (size_t i = 0; i < str_length; i++) {

		if (str[i] != '\n' && str[i] != '\t' && str[i] != ' ' && str[i] != '\b' && str[i] != '\a' && str[i] != '\r') // all other characters are drawn
		{
			writeCharacter(str[i], x_start, y_start, targwidth, targheight, color_0_255, backcolor_0_255, RenderBackground);
			// Iterate the x_start
			x_start += (int)targwidth;
		} else if (str[i] == '\n' || str[i] == '\r') {
			// Iterate the y_start and
			x_start = x;
			y_start -= (int)targheight;
		} else {
			x_start += (int)targwidth;
		}
	}
}

void BMPFontRenderer::clearScreen(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
	//~ for (unsigned int w = 0; w < Screen->getMyWidth(); w++)
	//~ for (unsigned int h = 0; h < Screen->getMyHeight(); h++) {
	//~ writePixel(w, h, red, green, blue, alpha);
	//~ }
	//~ unsigned char* Stripe = (unsigned char*)malloc(4 * Screen->getMyWidth());
	for (size_t i = 0; i < Screen->getMyWidth() * 4; i++)
		switch (i % 4) {
		case 0:
			Stripe[i] = red;
			break;
		case 1:
			Stripe[i] = green;
			break;
		case 2:
			Stripe[i] = blue;
			break;
		case 3:
			Stripe[i] = alpha;
			break;
		}
	for (size_t i = 0; i < Screen->getMyHeight(); i++)
		memcpy(Buffer + i * Screen->getMyWidth() * 4, Stripe, Screen->getMyWidth() * 4);
	//~ if(Stripe) free(Stripe);
}
void BMPFontRenderer::pushChangestoTexture() { Screen->reinitFromDataPointer(false, true); }
}; // namespace gekRender
