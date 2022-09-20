#include "Voxelizer.h"
using namespace glm;

using namespace std;

GLuint make3DTexture(GLsizei width, GLsizei height, GLsizei depth, GLsizei levels, GLenum internalFormat, GLint minFilter, GLint magFilter)
{
    GLuint handle;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_3D, handle);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexStorage3D(GL_TEXTURE_3D, levels, internalFormat, width, height, depth);
    if (internalFormat == GL_R32UI) {
        glClearTexImage(handle, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
    }
    else {
        glClearTexImage(handle, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    }
    if (levels > 1) {
        glGenerateMipmap(GL_TEXTURE_3D);
    }
    glBindTexture(GL_TEXTURE_3D, 0);
    return handle;
}

void Voxelizer::initIntToFloatTextureShader()
{
    string shaderString = readFileAsString("../resources/compute_int_to_float_tex.glsl");
    const char* shader = shaderString.c_str();
    GLuint intToFloatShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(intToFloatShader, 1, &shader, nullptr);

    GLint rc;
    CHECKED_GL_CALL(glCompileShader(intToFloatShader));
    CHECKED_GL_CALL(glGetShaderiv(intToFloatShader, GL_COMPILE_STATUS, &rc));
    if (!rc)	//error compiling the shader file
    {
        GLSL::printShaderInfoLog(intToFloatShader);
        std::cout << "Error compiling compute shader " << std::endl;
        exit(1);
    }

    intToFloatProgram = glCreateProgram();
    glAttachShader(intToFloatProgram, intToFloatShader);
    glLinkProgram(intToFloatProgram);
}

Voxelizer::Voxelizer()
{
    voxelTex_red = make3DTexture(VOXELSIZE, VOXELSIZE, VOXELSIZE, 1, GL_R32UI, GL_NEAREST, GL_NEAREST);
    voxelTex_green = make3DTexture(VOXELSIZE, VOXELSIZE, VOXELSIZE, 1, GL_R32UI, GL_NEAREST, GL_NEAREST);
    voxelTex_blue = make3DTexture(VOXELSIZE, VOXELSIZE, VOXELSIZE, 1, GL_R32UI, GL_NEAREST, GL_NEAREST);
    voxelTex_alpha = make3DTexture(VOXELSIZE, VOXELSIZE, VOXELSIZE, 1, GL_R32UI, GL_NEAREST, GL_NEAREST);
    voxelTex_float = make3DTexture(VOXELSIZE, VOXELSIZE, VOXELSIZE, 1, GL_RGBA32F, GL_NEAREST, GL_NEAREST);

	// create the shader program to render the scene
    std::string resourceDir = "../resources";

    initIntToFloatTextureShader();

    voxel_prog = make_shared<Program>();
    voxel_prog->setVerbose(true);
    voxel_prog->setShaderNames(resourceDir + "/voxel_vert.glsl", resourceDir + "/voxel_frag.glsl");
    if (!voxel_prog->init())
    {
        std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
        exit(1);
    }
    voxel_prog->init();
    voxel_prog->addUniform("P");
    voxel_prog->addUniform("V");
    voxel_prog->addUniform("M");
    voxel_prog->addUniform("P_light");
    voxel_prog->addUniform("V_light");
    voxel_prog->addUniform("campos");
    voxel_prog->addUniform("lp");
    voxel_prog->addAttribute("vertPos");
    voxel_prog->addAttribute("vertNor");
    voxel_prog->addAttribute("vertTex");

    glUseProgram(voxel_prog->pid);
    glUniform1i(glGetUniformLocation(voxel_prog->pid, "tex"), 0);
    glUniform1i(glGetUniformLocation(voxel_prog->pid, "texNormal"), 1);
    glUniform1i(glGetUniformLocation(voxel_prog->pid, "voxoutput_red"), 2);
    glUniform1i(glGetUniformLocation(voxel_prog->pid, "voxoutput_green"), 3);
    glUniform1i(glGetUniformLocation(voxel_prog->pid, "voxoutput_blue"), 4);
    glUniform1i(glGetUniformLocation(voxel_prog->pid, "voxoutput_alpha"), 5);
    glUniform1i(glGetUniformLocation(voxel_prog->pid, "shadowmap"), 6);

    // initialize low res frame buffer and texture
    glGenTextures(1, &v_rendertex);
    glBindTexture(GL_TEXTURE_2D, v_rendertex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, VOXEL_RENDER_SIZE, VOXEL_RENDER_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);

    //-------------------------

    glGenFramebuffers(1, &v_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, v_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, v_rendertex, 0);

    // ------------------------
}

void Voxelizer::drawToVoxelTexture(GLuint FBOtex_shadowMapDepth, const mat4 &P_light, const mat4 &V_light, vec3 campos,
    shared_ptr<Shape> sponza, vec3 lp)
{
    glBindFramebuffer(GL_FRAMEBUFFER, v_fbo);
    glViewport(0, 0, VOXEL_RENDER_SIZE, VOXEL_RENDER_SIZE);
    glClearColor(0.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    // clear the texture:
    glClearTexImage(voxelTex_red, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
    glClearTexImage(voxelTex_green, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
    glClearTexImage(voxelTex_blue, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
    glClearTexImage(voxelTex_alpha, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr); //GL_RED_INTEGER or whatever
    // orthographic projection matrix for voxelization: (correct)
    mat4 P = glm::ortho(-10.0, 10.0, -10.0, 10.0, -10.0, 10.0);    // by this, the front of the scene must be placed in front of the cam
    // disable depth test:
    glDisable(GL_DEPTH_TEST);
    // create 3 view matrices (this is correct)
    mat4 origV = lookAt(vec3(0), vec3(1), vec3(0, 1, 0));
    mat4 V1 = lookAt(vec3(0, 0, 0), vec3(origV[0]), vec3(0, 1, 0));
    mat4 V2 = lookAt(vec3(0, 0, 0), vec3(origV[1]), vec3(0, 1, 0));
    mat4 V3 = lookAt(vec3(0, 0, 0), vec3(origV[2]), vec3(0, 1, 0));

    // setup data for shader
    voxel_prog->bind();
    glUniformMatrix4fv(voxel_prog->getUniform("P_light"), 1, GL_FALSE, &P_light[0][0]);
    glUniformMatrix4fv(voxel_prog->getUniform("V_light"), 1, GL_FALSE, &V_light[0][0]);
    glUniform3fv(voxel_prog->getUniform("campos"), 1, &campos.x);
    glUniform3fv(voxel_prog->getUniform("lp"), 1, &campos.x);
    static float sponzaangle = -3.1415926 / 2.0;
    glUniformMatrix4fv(voxel_prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
    mat4 M_Sponza = glm::rotate(glm::mat4(1.f), radians(90.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1), glm::vec3(9));
    glUniformMatrix4fv(voxel_prog->getUniform("M"), 1, GL_FALSE, &M_Sponza[0][0]);
    glBindImageTexture(2, voxelTex_red, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(3, voxelTex_green, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(4, voxelTex_blue, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(5, voxelTex_alpha, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
    
    // render the scene 3 times into each of the associated views
    glUniformMatrix4fv(voxel_prog->getUniform("V"), 1, GL_FALSE, &V1[0][0]);
    sponza->draw(voxel_prog, false);
    glUniformMatrix4fv(voxel_prog->getUniform("V"), 1, GL_FALSE, &V2[0][0]);
    sponza->draw(voxel_prog, false);
    glUniformMatrix4fv(voxel_prog->getUniform("V"), 1, GL_FALSE, &V3[0][0]);
    sponza->draw(voxel_prog, false);

    voxel_prog->unbind();
    glEnable(GL_DEPTH_TEST);

    compute_float_tex();
}

void Voxelizer::compute_float_tex()
{
    glUseProgram(intToFloatProgram);

    glBindImageTexture(0, voxelTex_red, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(1, voxelTex_green, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(2, voxelTex_blue, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(3, voxelTex_alpha, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(4, voxelTex_float, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);

    glDispatchCompute(VOXELSIZE, VOXELSIZE, VOXELSIZE);

    glUseProgram(0);
}