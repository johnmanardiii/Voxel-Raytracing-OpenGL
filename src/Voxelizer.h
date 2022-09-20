#pragma once
#include <glad/glad.h>
#include "Program.h"
#include "Shape.h"
#include <memory>
#include <iostream>
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "GLSL.h"


#define VOXELSIZE 256
#define VOXEL_RENDER_SIZE 512

class Voxelizer {
private:
	std::shared_ptr<Program> voxel_prog, minecraft_prog;
	GLuint intToFloatProgram;
	void initIntToFloatTextureShader();
	void compute_float_tex();
public:
	GLuint voxelTex_red, voxelTex_green, voxelTex_blue, voxelTex_alpha, voxelTex_float, v_rendertex,
		v_fbo;
	Voxelizer();
	void drawToVoxelTexture(GLuint FBOtex_shadowMapDepth,
		const glm::mat4 &P_light, const glm::mat4 &V_light, glm::vec3 campos,
		std::shared_ptr<Shape> sponza, glm::vec3 lp);
	void renderSponzaToScreenMinecraft(std::shared_ptr<Shape> sponza,
		const glm::mat4& P, const glm::mat4& V);
};