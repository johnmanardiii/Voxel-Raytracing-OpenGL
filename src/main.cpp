/* Lab 6 base code - transforms using local matrix functions
    to be written by students -
    based on lab 5 by CPE 471 Cal Poly Z. Wood + S. Sueda
    & Ian Dunn, Christian Eckhardt
*/
#include <iostream>
#include <string>
#include <glad/glad.h>
#include "stb_image.h"

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "WindowManager.h"
#include "Camera.h"
#include "Voxelizer.h"
#include "skmesh.h"
#include "SmartTexture.h"
// used for helper in perspective
#include "glm/glm.hpp"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/vector3.h"
#include "assimp/scene.h"
#include <assimp/mesh.h>

using namespace std;
using namespace glm;

#define SHADOW_DIM 8192

bool deferred = true;
bool aoMode = true;

// sorry about the globals my brothers im going for speed ;)
// fpscam fcam;
// float lastX, lastY;

class ssbo_data
{
public:
    int sum_luminance;
    int max_luminance;
};

class Application : public EventCallbacks
{

public:

    WindowManager* windowManager = nullptr;

    // Our shader program
    std::shared_ptr<Program> prog_world_3_sampler, prog_gen_lighting, shadowProg, prog_bloom_pass, prog_frag_final, prog_ssao, prog_hbao, prog_ao_blur, prog_ray_denoise,
        skinProg, skinShadowProg, star_prog;
    GLuint computeProgram;


    // Shape to be used (from obj file)
    shared_ptr<Shape> shape;
    shared_ptr<Shape> sponza;
    shared_ptr<Voxelizer> voxelizer;

    // skinnedMesh
    SkinnedMesh skmesh;

    // shared_ptr<Shape> fountain;

    //camera
    // camera mycam;

    // FBOs
    GLuint fb_world_to_postproc;
    GLuint depth_rb;
    GLuint fb_shadowMap;
    GLuint fb_merge_bloom;
    GLuint fb_bloom_passes[2];
    GLuint fb_ray_denoise_passes[2];
    GLuint fb_ssao;
    GLuint fb_blur_ssao;

    // SSBO
    GLuint ssbo_GPU_id;


    // Render Textures
    GLuint FBOColorTex;
    GLuint PosTex;
    GLuint NormTex, mask_tex;
    GLuint FBOtex_shadowMapDepth;
    GLuint FBOtex_bloom_pass[2];
    GLuint FBOtex_ray_denoise_pass[2];
    GLuint FBOtex_world_nobloom;
    GLuint FBOtex_world_nobloom_hdr;
    GLuint FBOtex_ao;
    GLuint FBOtex_ao_blurred;

    // SSAO
    vec3 dome[64];
    vec3 rt_dome[30];
    GLuint TextureNoise;
    GLuint noiseTexture;

    GLuint VertexArrayIDBox, VertexBufferIDBox, VertexBufferTex;

    // Contains vertex information for OpenGL
    GLuint VertexArrayID;

    // Data necessary to give our triangle to OpenGL
    GLuint VertexBufferID;

    unsigned int framecounter = 0;
    float timecounter = 0.0f;
    double frametime = 0.0f;

    float exposure = 1.0f;
    float max_luminance = 0.01f;

    float radius = 0.1f;
    float bias = 0.015f;
    float total_time = 0.0;

    bool ssaoOn = false;
    bool bloom = true;
    bool tonemap = true;
    bool raycast_on = false;
    bool blur_on = true;
    vec3 lightPos;
    int raymarch_count = 60;
    int num_random_samples = 6;


    ssbo_data luminance_info;

    glm::mat4 P_light, V_light, V_eye, P_eye;

    vector<ObjectPath> paths;


    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        if (key == GLFW_KEY_W && action == GLFW_PRESS)
        {
            mycam.w = 1;
        }
        if (key == GLFW_KEY_W && action == GLFW_RELEASE)
        {
            mycam.w = 0;
        }
        if (key == GLFW_KEY_S && action == GLFW_PRESS)
        {
            mycam.s = 1;
        }
        if (key == GLFW_KEY_S && action == GLFW_RELEASE)
        {
            mycam.s = 0;
        }
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
        {
            mycam.a = 1;
        }
        if (key == GLFW_KEY_A && action == GLFW_RELEASE)
        {
            mycam.a = 0;
        }
        if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        {
            mycam.z = 1;
        }
        if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        {
            mycam.z = 0;
        }
        if (key == GLFW_KEY_D && action == GLFW_PRESS)
        {
            mycam.d = 1;
        }
        if (key == GLFW_KEY_D && action == GLFW_RELEASE)
        {
            mycam.d = 0;
        }
        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            mycam.q = 1;
        }
        if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
        {
            mycam.q = 0;
        }
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            mycam.e = 1;
        }
        if (key == GLFW_KEY_E && action == GLFW_RELEASE)
        {
            mycam.e = 0;
        }
        if (key == GLFW_KEY_V && action == GLFW_PRESS)
        {
            radius += 0.01;
            cout << "radius: " << radius << endl;
        }
        if (key == GLFW_KEY_B && action == GLFW_PRESS)
        {
            radius -= 0.01;
            cout << "radius: " << radius << endl;
        }
        if (key == GLFW_KEY_N && action == GLFW_PRESS)
        {
            bias += 0.01;
            cout << "bias: " << bias << endl;
        }
        if (key == GLFW_KEY_M && action == GLFW_PRESS)
        {
            bias -= 0.01;
            cout << "bias: " << bias << endl;
        }
        if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        {
            raymarch_count += 5;
            raymarch_count = clamp(raymarch_count, 5, 125);
            cout << "Number of raymarches: " << raymarch_count << endl;
        }
        if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        {
            raymarch_count -= 5;
            raymarch_count = clamp(raymarch_count, 5, 125);
            raymarch_count = clamp(raymarch_count, 5, 125);
            cout << "Number of raymarches: " << raymarch_count << endl;
        }
        if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        {
            num_random_samples -= 1;
            num_random_samples = clamp(num_random_samples, 3, 30);
            cout << "Number of random samples: " << num_random_samples << endl;
        }
        if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        {
            num_random_samples += 1;
            num_random_samples = clamp(num_random_samples, 3, 30);
            cout << "Number of random samples: " << num_random_samples << endl;
        }
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        {
            aoMode = !aoMode;
            cout << "\nAO Mode: " << (aoMode ? "SSAO" : "HBAO") << endl;
        }
        if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
        {
            // pops off the last point on the list if there are any
            paths[0].popPoint();
        }
        if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
        {
            vec3 dir, pos, up;
            mycam.get_dirpos(up, dir, pos);
            paths[0].addPoint(PathPoint(pos, dir, up));
        }
        if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS)
        {
            mycam.pathcam = !mycam.pathcam;
        }
        if (key == GLFW_KEY_B && action == GLFW_PRESS)
        {
            bloom = !bloom;
        }
        if (key == GLFW_KEY_N && action == GLFW_PRESS)
        {
            tonemap = !tonemap;
        }
        if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            raycast_on = !raycast_on;
        }
        if (key == GLFW_KEY_T && action == GLFW_PRESS)
        {
            blur_on = !blur_on;
        }
    }

    void mouseCallback(GLFWwindow* window, int button, int action, int mods)
    {
        double posX, posY;

        if (action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &posX, &posY);
            cout << "Pos X " << posX << " Pos Y " << posY << endl;
        }
    }

    void resizeCallback(GLFWwindow* window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void init(const std::string& resourceDirectory)
    {
        int width, height;
        glfwSetCursorPosCallback(windowManager->getHandle(), mouse_curs_callback);
        glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        lastX = width / 2.0f;
        lastY = height / 2.0f;
        GLSL::checkVersion();

        paths.push_back(ObjectPath(vec3(1, .5, 0),
            vec3(.4, 1, .2), "path2.txt"));

        // Set background color.
        glClearColor(0.12f, 0.34f, 0.56f, 1.0f);

        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);

        //culling:
        glDisable(GL_CULL_FACE);

        //transparency
        glEnable(GL_BLEND);
        //next function defines how to mix the background color with the transparent pixel in the foreground. 
        //This is the standard:
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Initialize the GLSL program.
        prog_world_3_sampler = make_shared<Program>();
        prog_world_3_sampler->setVerbose(true);
        prog_world_3_sampler->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
        if (!prog_world_3_sampler->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_world_3_sampler->init();
        prog_world_3_sampler->addUniform("P");
        prog_world_3_sampler->addUniform("V");
        prog_world_3_sampler->addUniform("M");
        prog_world_3_sampler->addAttribute("vertPos");
        prog_world_3_sampler->addAttribute("vertNor");
        prog_world_3_sampler->addAttribute("vertTex");


        // Initialize the Shadow Map shader program.
        shadowProg = make_shared<Program>();
        shadowProg->setVerbose(true);
        shadowProg->setShaderNames(resourceDirectory + "/shadow_vert.glsl", resourceDirectory + "/shadow_frag.glsl");
        if (!shadowProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        shadowProg->init();
        shadowProg->addUniform("P");
        shadowProg->addUniform("V");
        shadowProg->addUniform("M");
        shadowProg->addAttribute("vertPos");
        shadowProg->addAttribute("vertNor");
        shadowProg->addAttribute("vertTex");


        prog_gen_lighting = make_shared<Program>();
        prog_gen_lighting->setVerbose(true);
        prog_gen_lighting->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_gen_lighting.glsl");
        if (!prog_gen_lighting->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_gen_lighting->init();
        prog_gen_lighting->addUniform("P");
        prog_gen_lighting->addUniform("P_light");
        prog_gen_lighting->addUniform("V_light");
        prog_gen_lighting->addUniform("V");
        prog_gen_lighting->addUniform("M");
        prog_gen_lighting->addUniform("rt_dome");
        prog_gen_lighting->addUniform("campos");
        prog_gen_lighting->addUniform("exposure");
        prog_gen_lighting->addUniform("noiseScale");
        prog_gen_lighting->addUniform("max_luminance");
        prog_gen_lighting->addUniform("bloom");
        prog_gen_lighting->addUniform("raycast_on");
        prog_gen_lighting->addUniform("tonemap");
        prog_gen_lighting->addUniform("num_raymarches");
        prog_gen_lighting->addUniform("num_random_samples");
        prog_gen_lighting->addUniform("ssaoOn");
        prog_gen_lighting->addUniform("lp");
        prog_gen_lighting->addAttribute("vertPos");
        prog_gen_lighting->addAttribute("vertTex");


        prog_ssao = make_shared<Program>();
        prog_ssao->setVerbose(true);
        prog_ssao->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/ssao_frag.glsl");
        if (!prog_ssao->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_ssao->init();
        prog_ssao->addUniform("P");
        prog_ssao->addUniform("P_eye");
        prog_ssao->addUniform("V_eye");
        prog_ssao->addUniform("V");
        prog_ssao->addUniform("M");
        prog_ssao->addUniform("dome");
        prog_ssao->addUniform("radius");
        prog_ssao->addUniform("bias");
        prog_ssao->addAttribute("vertPos");
        prog_ssao->addAttribute("vertTex");

        prog_hbao = make_shared<Program>();
        prog_hbao->setVerbose(true);
        prog_hbao->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/hbao_frag.glsl");
        if (!prog_hbao->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_hbao->init();
        prog_hbao->addUniform("P");
        prog_hbao->addUniform("P_eye");
        prog_hbao->addUniform("V_eye");
        prog_hbao->addUniform("V");
        prog_hbao->addUniform("M");
        prog_ssao->addUniform("bias");
        prog_hbao->addAttribute("vertPos");
        prog_hbao->addAttribute("vertTex");

        prog_ao_blur = make_shared<Program>();
        prog_ao_blur->setVerbose(true);
        prog_ao_blur->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/ssao_blur_frag.glsl");
        if (!prog_ao_blur->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_ao_blur->init();
        prog_ao_blur->addUniform("P");
        prog_ao_blur->addUniform("V");
        prog_ao_blur->addUniform("M");
        prog_ao_blur->addAttribute("vertPos");
        prog_ao_blur->addAttribute("vertTex");


        prog_bloom_pass = make_shared<Program>();
        prog_bloom_pass->setVerbose(true);
        prog_bloom_pass->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_bloom_pass.glsl");
        if (!prog_bloom_pass->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_bloom_pass->init();
        prog_bloom_pass->addUniform("P");
        prog_bloom_pass->addUniform("V");
        prog_bloom_pass->addUniform("M");
        prog_bloom_pass->addUniform("direction");
        prog_bloom_pass->addAttribute("vertPos");
        prog_bloom_pass->addAttribute("vertTex");

        prog_ray_denoise = make_shared<Program>();
        prog_ray_denoise->setVerbose(true);
        prog_ray_denoise->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_ray_denoise.glsl");
        if (!prog_ray_denoise->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_ray_denoise->init();
        prog_ray_denoise->addUniform("P");
        prog_ray_denoise->addUniform("V");
        prog_ray_denoise->addUniform("M");
        prog_ray_denoise->addUniform("direction");
        prog_ray_denoise->addUniform("V_eye");
        prog_ray_denoise->addAttribute("vertPos");
        prog_ray_denoise->addAttribute("vertTex");


        prog_frag_final = make_shared<Program>();
        prog_frag_final->setVerbose(true);
        prog_frag_final->setShaderNames(resourceDirectory + "/vert.glsl", resourceDirectory + "/frag_final.glsl");
        if (!prog_frag_final->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        prog_frag_final->init();
        prog_frag_final->addUniform("P");
        prog_frag_final->addUniform("V");
        prog_frag_final->addUniform("M");
        prog_frag_final->addAttribute("vertPos");
        prog_frag_final->addAttribute("vertTex");


        std::string ShaderString = readFileAsString(resourceDirectory + "/compute.glsl");
        const char* shader = ShaderString.c_str();
        GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(computeShader, 1, &shader, nullptr);

        GLint rc;
        CHECKED_GL_CALL(glCompileShader(computeShader));
        CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
        if (!rc)	//error compiling the shader file
        {
            GLSL::printShaderInfoLog(computeShader);
            std::cout << "Error compiling fragment shader " << std::endl;
            exit(1);
        }

        computeProgram = glCreateProgram();
        glAttachShader(computeProgram, computeShader);
        glLinkProgram(computeProgram);
        
    }

    void gen_SSAO_dome()
    {
        float temp_radius;
        float x_angle;
        float y_angle;
        constexpr float pi = glm::pi<float>();

        for (int i = 0; i < 64; i++)
        {
            temp_radius = ((float)rand()) / RAND_MAX;
            temp_radius = pow(temp_radius, 2);
            temp_radius *= radius;

            x_angle = ((float)rand()) / RAND_MAX;
            x_angle = (x_angle - 0.5f) * (pi);

            y_angle = ((float)rand()) / RAND_MAX;
            y_angle = (y_angle - 0.5f) * (pi);

            vec4 point = vec4(0.0f, 0.0f, temp_radius, 1);

            mat4 R_x = glm::rotate(mat4(1.0f), x_angle, vec3(1, 0, 0));
            mat4 R_y = glm::rotate(mat4(1.0f), y_angle, vec3(0, 1, 0));

            point = R_y * R_x * point;

            dome[i] = vec3(point.x, point.y, point.z);
        }
    }

    void gen_rt_dome()
    {
        float temp_radius;
        float x_angle;
        float y_angle;
        constexpr float pi = glm::pi<float>();
        std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
        std::default_random_engine generator;
        int i = 0;
        while(i < 30)
        {
            glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
            if (length(sample) > 1.0) {
                continue;
            }
            sample += vec3(0, 0, static_cast<float>(i) / (30.0 * 2.0));
            sample = glm::normalize(sample);
            rt_dome[i] = sample;
            i++;
        }
        i = 0;
    }

    void initMario(const std::string& resourceDirectory)
    {
        if (!skmesh.LoadMesh(resourceDirectory + "/mario_tangents.fbx")) {
            printf("Mesh load failed\n");
            return;
        }

        skmesh.currentAnimation = 0;

        skinProg = std::make_shared<Program>();
        skinProg->setVerbose(true);
        skinProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/simple_frag.glsl");
        if (!skinProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            windowManager->shutdown();
        }

        skinProg->addUniform("P");
        skinProg->addUniform("V");
        skinProg->addUniform("M");
        skinProg->addUniform("tex");
        skinProg->addUniform("camPos");
        skinProg->addAttribute("vertPos");
        skinProg->addAttribute("vertNor");
        skinProg->addAttribute("vertTex");
        skinProg->addAttribute("BoneIDs");
        skinProg->addAttribute("Weights");
        skinProg->addAttribute("vertTan");
        skinProg->addAttribute("vertBinorm");

        skinShadowProg = std::make_shared<Program>();
        skinShadowProg->setVerbose(true);
        skinShadowProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_shadow_frag.glsl");
        if (!skinShadowProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            windowManager->shutdown();
        }

        skinShadowProg->addUniform("P");
        skinShadowProg->addUniform("V");
        skinShadowProg->addUniform("M");
        skinShadowProg->addUniform("tex");
        skinShadowProg->addUniform("camPos");
        skinShadowProg->addAttribute("vertPos");
        skinShadowProg->addAttribute("vertNor");
        skinShadowProg->addAttribute("vertTex");
        skinShadowProg->addAttribute("BoneIDs");
        skinShadowProg->addAttribute("Weights");
        skinShadowProg->addAttribute("vertTan");
        skinShadowProg->addAttribute("vertBinorm");
    }

    float randomFloat(float a, float b) {
        float random = ((float)rand()) / (float)RAND_MAX;
        float diff = b - a;
        float r = random * diff;
        return a + r;
    }
#define M_PI       3.14159265358979323846   // pi
    vec4 getStarPosition()
    {
        // get a random position towards the top of the sphere
        float phi = randomFloat(0.0, M_PI); // somewhere between 0 and 2pi/3
        float theta = randomFloat(0, 2.0 * M_PI); // somewhere between 0 and 2pi
        float radius = pow(randomFloat(8, 17), 1.8); // I subtract a little here so it doesn't get caught at the top only.
        // convert to cartesian coordinates
        float x = radius * sin(phi) * cos(theta);
        float y = radius * sin(phi) * sin(theta);
        float z = radius * cos(phi) - 3;
        return vec4(x, y, z, 1);
        //return vec4(x, y * .12f, z, 1);
    }

#define STARS 10000
    GLuint QuadVAOID, QuadVertexBufferID, star_tex, VertexNormDBox, VertexTexBox,
        QuadIndexBuffer, InstanceBuffer;
    void init_stars(const std::string& resourceDirectory)
    {
        star_prog = std::make_shared<Program>();
        star_prog->setVerbose(true);
        star_prog->setShaderNames(resourceDirectory + "/star_vertex.glsl", resourceDirectory + "/star_fragment.glsl");
        if (!star_prog->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        star_prog->addUniform("P");
        star_prog->addUniform("V");
        star_prog->addUniform("M");
        star_prog->addUniform("campos");
        star_prog->addUniform("glTime");
        star_prog->addUniform("gamma");
        star_prog->addAttribute("vertPos");
        star_prog->addAttribute("vertNor");
        star_prog->addAttribute("vertTex");
        star_prog->addAttribute("InstancePos");
        //generate the VAO
        glGenVertexArrays(1, &QuadVAOID);
        glBindVertexArray(QuadVAOID);

        //generate vertex buffer to hand off to OGL
        glGenBuffers(1, &QuadVertexBufferID);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, QuadVertexBufferID);

        GLfloat cube_vertices[] = {
            // front
            -1.0, -1.0,  1.0,//LD
            1.0, -1.0,  1.0,//RD
            1.0,  1.0,  1.0,//RU
            -1.0,  1.0,  1.0,//LU
        };
        //make it a bit smaller
        for (int i = 0; i < 12; i++)
            cube_vertices[i] *= 0.5;
        //actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_DYNAMIC_DRAW);

        //we need to set up the vertex array
        glEnableVertexAttribArray(0);
        //key function to get up how many elements to pull out at a time (3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        //color
        GLfloat cube_norm[] = {
            // front colors
            0.0, 0.0, 1.0,
            0.0, 0.0, 1.0,
            0.0, 0.0, 1.0,
            0.0, 0.0, 1.0,

        };
        glGenBuffers(1, &VertexNormDBox);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexNormDBox);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_norm), cube_norm, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        //color
        glm::vec2 cube_tex[] = {
            // front colors
            glm::vec2(0.0, 1.0),
            glm::vec2(1.0, 1.0),
            glm::vec2(1.0, 0.0),
            glm::vec2(0.0, 0.0),

        };
        glGenBuffers(1, &VertexTexBox);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexTexBox);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex), cube_tex, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &QuadIndexBuffer);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadIndexBuffer);
        GLushort cube_elements[] = {

            // front
            0, 1, 2,
            2, 3, 0,
        };
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_elements), cube_elements, GL_STATIC_DRAW);


        //generate vertex buffer to hand off to OGL ###########################
        glGenBuffers(1, &InstanceBuffer);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, InstanceBuffer);
        glm::vec4* positions = new glm::vec4[STARS];
        for (int i = 0; i < STARS; i++)
        {
            // set star position here
            positions[i] = getStarPosition();
        }
        //actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, STARS * sizeof(glm::vec4), positions, GL_STATIC_DRAW);
        int position_loc = glGetAttribLocation(star_prog->pid, "InstancePos");
        for (int i = 0; i < STARS; i++)
        {
            // Set up the vertex attribute
            glVertexAttribPointer(position_loc + i,              // Location
                4, GL_FLOAT, GL_FALSE,       // vec4
                sizeof(vec4),                // Stride
                (void*)(sizeof(vec4) * i)); // Start offset
                                             // Enable it
            glEnableVertexAttribArray(position_loc + i);
            // Make it instanced
            glVertexAttribDivisor(position_loc + i, 1);
        }


        glBindVertexArray(0);



        int width, height, channels;
        char filepath[1000];

        //texture 1
        string str = resourceDirectory + "/Blue_Giant.jpg";
        strcpy(filepath, str.c_str());
        unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &star_tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, star_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        //texture 2
        GLuint Tex1Location = glGetUniformLocation(star_prog->pid, "tex");
        // Then bind the uniform samplers to texture units:
        glUseProgram(star_prog->pid);
        glUniform1i(Tex1Location, 0);
    }

    void initGeom(const std::string& resourceDirectory)
    {
        initMario(resourceDirectory);
        init_stars(resourceDirectory);
        voxelizer = make_shared<Voxelizer>();
        //init rectangle mesh (2 triangles) for the post processing
        glGenVertexArrays(1, &VertexArrayIDBox);
        glBindVertexArray(VertexArrayIDBox);

        //generate vertex buffer to hand off to OGL
        glGenBuffers(1, &VertexBufferIDBox);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDBox);

        GLfloat* rectangle_vertices = new GLfloat[18];
        // front
        int verccount = 0;

        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;
        rectangle_vertices[verccount++] = -1.0, rectangle_vertices[verccount++] = 1.0, rectangle_vertices[verccount++] = 0.0;


        //actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), rectangle_vertices, GL_STATIC_DRAW);
        //we need to set up the vertex array
        glEnableVertexAttribArray(0);
        //key function to get up how many elements to pull out at a time (3)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


        //generate vertex buffer to hand off to OGL
        glGenBuffers(1, &VertexBufferTex);
        //set the current state to focus on our vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTex);

        float t = 1. / 100.;
        GLfloat* rectangle_texture_coords = new GLfloat[12];
        int texccount = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 0;
        rectangle_texture_coords[texccount++] = 1, rectangle_texture_coords[texccount++] = 1;
        rectangle_texture_coords[texccount++] = 0, rectangle_texture_coords[texccount++] = 1;

        //actually memcopy the data - only do this once
        glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), rectangle_texture_coords, GL_STATIC_DRAW);
        //we need to set up the vertex array
        glEnableVertexAttribArray(2);
        //key function to get up how many elements to pull out at a time (3)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        // Initialize mesh.
        shape = make_shared<Shape>();
        shape->loadMesh(resourceDirectory + "/sphere.obj");
        shape->resize();
        shape->init();


        int width, height, channels;
        char filepath[1000];

        sponza = make_shared<Shape>();
        string sponzamtl = resourceDirectory + "/odyssey/";
        sponza->loadMesh(resourceDirectory + "/odyssey/HomeInside.obj", &sponzamtl, stbi_load);
        sponza->resize();
        sponza->init();

        std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
        std::default_random_engine generator;
        std::vector<glm::vec3> ssaoNoise;
        for (unsigned int i = 0; i < 250000; i++)
        {
            glm::vec3 noise(
                randomFloats(generator) * 2.0 - 1.0,
                randomFloats(generator) * 2.0 - 1.0,
                0.0f);
            ssaoNoise.push_back(noise);
        }
        glGenTextures(1, &noiseTexture);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 500, 500, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        //[TWOTEXTURES]
        //set the 2 textures to the correct samplers in the fragment shader:
        GLuint Tex1Location = glGetUniformLocation(prog_world_3_sampler->pid, "tex");//tex, tex2... sampler in the fragment shader
        GLuint Tex2Location = glGetUniformLocation(prog_world_3_sampler->pid, "texNormal");
        // Then bind the uniform samplers to texture units:
        glUseProgram(prog_world_3_sampler->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);


        Tex1Location = glGetUniformLocation(prog_gen_lighting->pid, "texcol");//tex, tex2... sampler in the fragment shader
        Tex2Location = glGetUniformLocation(prog_gen_lighting->pid, "texpos");
        GLuint Tex3Location = glGetUniformLocation(prog_gen_lighting->pid, "texnorm");
        GLuint Tex4Location = glGetUniformLocation(prog_gen_lighting->pid, "shadowmap");
        // Then bind the uniform samplers to texture units:
        glUseProgram(prog_gen_lighting->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);
        glUniform1i(Tex3Location, 2);
        glUniform1i(Tex4Location, 3);
        glUniform1i(glGetUniformLocation(prog_gen_lighting->pid, "voxoutput_alpha"), 4);
        glUniform1i(glGetUniformLocation(prog_gen_lighting->pid, "voxoutput_float"), 5);
        glUniform1i(glGetUniformLocation(prog_gen_lighting->pid, "noiseTex"), 6);
        glUniform1i(glGetUniformLocation(prog_gen_lighting->pid, "maskTex"), 7);

        Tex1Location = glGetUniformLocation(prog_ssao->pid, "texpos");
        Tex2Location = glGetUniformLocation(prog_ssao->pid, "texnorm");
        Tex3Location = glGetUniformLocation(prog_ssao->pid, "noise");
        // Then bind the uniform samplers to texture units:
        glUseProgram(prog_ssao->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);
        glUniform1i(Tex3Location, 2);

        Tex1Location = glGetUniformLocation(prog_hbao->pid, "texpos");
        Tex2Location = glGetUniformLocation(prog_hbao->pid, "texnorm");
        Tex3Location = glGetUniformLocation(prog_hbao->pid, "noise");
        // Then bind the uniform samplers to texture units:
        glUseProgram(prog_hbao->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);
        glUniform1i(Tex3Location, 2);

        Tex1Location = glGetUniformLocation(prog_ao_blur->pid, "ssaoInput");
        glUseProgram(prog_ao_blur->pid);
        glUniform1i(Tex1Location, 0);

        Tex1Location = glGetUniformLocation(prog_frag_final->pid, "texcol");//tex, tex2... sampler in the fragment shader
        Tex2Location = glGetUniformLocation(prog_frag_final->pid, "texbloom");
        // Then bind the uniform samplers to texture units:
        glUseProgram(prog_frag_final->pid);
        glUniform1i(Tex1Location, 0);
        glUniform1i(Tex2Location, 1);
        glUniform1i(glGetUniformLocation(prog_frag_final->pid, "texraytrace"), 2);
        glUniform1i(glGetUniformLocation(prog_frag_final->pid, "objectcolors"), 3);

        Tex1Location = glGetUniformLocation(prog_bloom_pass->pid, "image");//tex, tex2... sampler in the fragment shader
        glUseProgram(prog_bloom_pass->pid);
        glUniform1i(Tex1Location, 0);

        glUseProgram(prog_ray_denoise->pid);
        glUniform1i(glGetUniformLocation(prog_ray_denoise->pid, "raytrace_color"), 0);
        glUniform1i(glGetUniformLocation(prog_ray_denoise->pid, "postex"), 1);
        glUniform1i(glGetUniformLocation(prog_ray_denoise->pid, "normtex"), 2);

        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        //RGBA8 2D texture, 24 bit depth texture, 256x256
        glGenTextures(1, &FBOColorTex);
        glBindTexture(GL_TEXTURE_2D, FBOColorTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);


        glGenTextures(1, &PosTex);
        glBindTexture(GL_TEXTURE_2D, PosTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &NormTex);
        glBindTexture(GL_TEXTURE_2D, NormTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_BGRA, GL_FLOAT, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &mask_tex);
        glBindTexture(GL_TEXTURE_2D, mask_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, width, height, 0, GL_RED_INTEGER,
            GL_UNSIGNED_INT, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        //-------------------------

        glGenFramebuffers(1, &fb_world_to_postproc);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_world_to_postproc);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOColorTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, PosTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, NormTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, mask_tex, 0);
        glGenRenderbuffers(1, &depth_rb);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rb);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rb);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, buffers);

        //-------------------------
        glGenTextures(1, &FBOtex_ao);
        glBindTexture(GL_TEXTURE_2D, FBOtex_ao);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLuint depth_rbloc;
        //-------------------------
        glGenFramebuffers(1, &fb_ssao);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_ssao);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtex_ao, 0);
        glGenRenderbuffers(1, &depth_rbloc);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbloc);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbloc);

        //-------------------------

        glGenFramebuffers(1, &fb_shadowMap);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_shadowMap);
        glGenTextures(1, &FBOtex_shadowMapDepth);
        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SHADOW_DIM, SHADOW_DIM, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(vec3(1.0)));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, FBOtex_shadowMapDepth, 0);
        // We don't want the draw result for a shadow map!
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        //-------------------------

        glGenFramebuffers(1, &fb_blur_ssao);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_blur_ssao);
        glGenTextures(1, &FBOtex_ao_blurred);
        glBindTexture(GL_TEXTURE_2D, FBOtex_ao_blurred);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtex_ao_blurred, 0);

        gen_bloom_FBOs();
        
        gen_SSAO_dome();
        gen_rt_dome();

        //Does the GPU support current FBO configuration?
        GLenum status;
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        switch (status)
        {
        case GL_FRAMEBUFFER_COMPLETE:
            cout << "status framebuffer: good";
            break;
        default:
            cout << "status framebuffer: bad!!!!!!!!!!!!!!!!!!!!!!!!!";
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        glGenBuffers(1, &ssbo_GPU_id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &luminance_info, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind

        cout << "\nAO Mode: " << (aoMode ? "SSAO" : "HBAO") << endl;
    }

    //*************************************
    void get_light_proj_matrix(glm::mat4& lightP)
    {
        // If your scene goes outside these "bounds" (e.g. shadows stop working near boundary),
        // feel free to increase these numbers (or decrease if scene shrinks/light gets closer to
        // scene objects).
        const float left = -30.0f;
        const float right = 30.0f;
        const float bottom = -30.0f;
        const float top = 30.0f;
        const float zNear = 1.f;
        const float zFar = 75.0f;

        lightP = glm::ortho(left, right, bottom, top, zNear, zFar);
    }

    void get_light_view_matrix(glm::mat4& lightV)
    {
        // Change earth_pos (or primaryLight.direction) to change where the light is pointing at.
        lightV = glm::lookAt(lightPos, vec3(0, 0, 0), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    void gen_bloom_FBOs()
    {
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        //RGBA8 2D texture, 24 bit depth texture, 256x256
        glGenTextures(1, &FBOtex_bloom_pass[0]);
        glBindTexture(GL_TEXTURE_2D, FBOtex_bloom_pass[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &FBOtex_bloom_pass[1]);
        glBindTexture(GL_TEXTURE_2D, FBOtex_bloom_pass[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);


        //--------------------


        glGenTextures(1, &FBOtex_ray_denoise_pass[0]);
        glBindTexture(GL_TEXTURE_2D, FBOtex_ray_denoise_pass[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &FBOtex_ray_denoise_pass[1]);
        glBindTexture(GL_TEXTURE_2D, FBOtex_ray_denoise_pass[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        //---------------------

        glGenTextures(1, &FBOtex_world_nobloom);
        glBindTexture(GL_TEXTURE_2D, FBOtex_world_nobloom);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        glGenTextures(1, &FBOtex_world_nobloom_hdr);
        glBindTexture(GL_TEXTURE_2D, FBOtex_world_nobloom_hdr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLuint depth_rbloc;
        //-------------------------
        glGenFramebuffers(1, &fb_merge_bloom);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_merge_bloom);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtex_world_nobloom, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, FBOtex_bloom_pass[0], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, FBOtex_world_nobloom_hdr, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, FBOtex_ray_denoise_pass[0], 0);
        glGenRenderbuffers(1, &depth_rbloc);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbloc);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbloc);

        //-------------------------
        for (int i = 0; i < 2; i++)
        {
            glGenFramebuffers(1, &fb_bloom_passes[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, fb_bloom_passes[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtex_bloom_pass[i], 0);
            glGenRenderbuffers(1, &depth_rbloc);
            glBindRenderbuffer(GL_RENDERBUFFER, depth_rbloc);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbloc);

            glGenFramebuffers(1, &fb_ray_denoise_passes[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, fb_ray_denoise_passes[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FBOtex_ray_denoise_pass[i], 0);
            glGenRenderbuffers(1, &depth_rbloc);
            glBindRenderbuffer(GL_RENDERBUFFER, depth_rbloc);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbloc);
        }

    }

    //*************************************
    double get_last_elapsed_time()
    {
        static double lasttime = glfwGetTime();
        double actualtime = glfwGetTime();
        double difference = actualtime - lasttime;
        lasttime = actualtime;
        return difference;
    }
    //*************************************
    void render_to_screen()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        frametime = get_last_elapsed_time();
        glClearColor(0.0, 0.0, 0.0, 1.0);
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prog_frag_final->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, FBOtex_world_nobloom);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, FBOtex_bloom_pass[0]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, FBOtex_ray_denoise_pass[0]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, FBOColorTex);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        prog_frag_final->unbind();

    }

    void render_voxels()
    {
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        P_eye = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.2f, 120.0f);
        if (mycam.pathcam)
        {
            V_eye = mycam.follow_path(frametime * .5, paths[0], vec3(0, -4, 4.5));
        }
        else
        {
            V_eye = mycam.process(frametime);
        }
        voxelizer->drawToVoxelTexture(FBOtex_shadowMapDepth, P_light, V_light, mycam.pos, sponza, lightPos);
    }

    void render_stars()
    {
        float velocity = 1.0;
        static float w = 0.0;
        const float c = 10.0f;
        w += velocity * frametime;//rotation angle
        float trans = 0;// sin(t) * 2;
        glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0.0f, 1.0f, 0.0f));
        float angle = -3.1415926 / 2.0;
        glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 TransZ = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3 + trans));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));

        mat4 M = TransZ * RotateY * RotateX * S;

        // Draw the box using GLSL.
        star_prog->bind();


        //send the matrices to the shaders
        glUniformMatrix4fv(star_prog->getUniform("P"), 1, GL_FALSE, &P_eye[0][0]);
        glUniformMatrix4fv(star_prog->getUniform("V"), 1, GL_FALSE, &V_eye[0][0]);
        glUniformMatrix4fv(star_prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        vec3 negativecampos = -mycam.pos;
        glUniform3fv(star_prog->getUniform("campos"), 1, &negativecampos[0]);
        glBindVertexArray(QuadVAOID);
        //actually draw from vertex 0, 3 vertices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadIndexBuffer);
        //glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, (void*)0);
        mat4 Vi = glm::transpose(V_eye);
        Vi[0][3] = 0;
        Vi[1][3] = 0;
        Vi[2][3] = 0;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, star_tex);

        M = TransZ * S * Vi;
        glUniformMatrix4fv(star_prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        glUniform1f(star_prog->getUniform("glTime"), w);

        float gamma;
        float diffsquared = 1.0f - ((velocity * velocity) / (c * c));
        if (diffsquared <= 0.00001f)
        {
            gamma = 0;
        }
        else
        {
            gamma = std::sqrt(diffsquared);
        }

        glUniform1f(star_prog->getUniform("gamma"), gamma);

        // glDisable(GL_DEPTH_TEST);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0, STARS);
        // glEnable(GL_DEPTH_TEST);
    }

    void render_3_sample() // aka render to framebuffer
    {

        glBindFramebuffer(GL_FRAMEBUFFER, fb_world_to_postproc);

        // glClearColor(0.12f, 0.34f, 0.56f, 1.0f);
        glClearColor(0.1, 0.1, 0.2, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);

        glm::mat4 M, V, S, T;

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //bind shader and copy matrices
        prog_world_3_sampler->bind();
        glUniformMatrix4fv(prog_world_3_sampler->getUniform("P"), 1, GL_FALSE, &P_eye[0][0]);
        glUniformMatrix4fv(prog_world_3_sampler->getUniform("V"), 1, GL_FALSE, &V_eye[0][0]);


        // *** sponza **
        mat4 M_Sponza = glm::rotate(glm::mat4(1.f), radians(90.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1), glm::vec3(9));
        glUniformMatrix4fv(prog_world_3_sampler->getUniform("M"), 1, GL_FALSE, &M_Sponza[0][0]);
        sponza->draw(prog_world_3_sampler, false);

        //done, unbind stuff
        prog_world_3_sampler->unbind();

        skinProg->bind();
        glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
        glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P_eye[0][0]);
        glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V_eye[0][0]);
        M = translate(mat4(1), vec3(0, .1, 0)) * translate(mat4(1), vec3(0, -5.25, 2.5)) * scale(mat4(1), vec3(.013));
        glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        const float marioAnimationSpeed = 30.0f;	//TODO: I want to move the mario model to its own files and this should be a class var
        skmesh.setBoneTransformationsNoUpdate(skinProg->pid, total_time * marioAnimationSpeed);
        skmesh.Render(glGetUniformLocation(skinProg->pid, "tex"), glGetUniformLocation(skinProg->pid, "texNormal"));
        skinProg->unbind();

        render_stars();
        paths[0].draw(P_eye, V_eye, prog_world_3_sampler, shape, !mycam.pathcam);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, FBOColorTex);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, PosTex);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, NormTex);
        glGenerateMipmap(GL_TEXTURE_2D);
    }


    void render_to_shadowmap()
    {
        total_time += frametime;
        //lightPos = vec3(sin(total_time * .4) * 6, 15, 5.1);
        //lightPos = vec3(sin(total_time * .5) * 1, 4, -10);
        lightPos = vec3(sin(total_time * .3) * 3, 10, -sin(total_time * .3) * 4 - 8);
        glFrontFace(GL_CW);
        glBindFramebuffer(GL_FRAMEBUFFER, fb_shadowMap);
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0, 1.0, 0.0, 0.0);
        glClear(GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, SHADOW_DIM, SHADOW_DIM);

        glDisable(GL_BLEND);

        glm::mat4 M, V, S, T, P;

        // Bind shadow map shader program and matrix uniforms.
        shadowProg->bind();



        // Orthographic frustum in light space; encloses the scene, adjust if larger or smaller scene used.
        get_light_proj_matrix(P);

        // "Camera" for rendering shadow map is at light source, looking at the scene.
        get_light_view_matrix(V);

        // Get current frame buffer size.
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        //glViewport(0, 0, width, height);
        // P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 1.0f, 35.0f); //so much type casting... GLM metods are quite funny ones


        //V = mycam.process();

        // Clear framebuffer.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        P_light = P;
        V_light = V;
        //bind shader and copy matrices
        glUniformMatrix4fv(shadowProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(shadowProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);

        static float sponzaangle = -3.1415926 / 2.0;
        mat4 M_Sponza = glm::rotate(glm::mat4(1.f), radians(90.0f), glm::vec3(1, 0, 0)) * glm::scale(glm::mat4(1), glm::vec3(9));
        glUniformMatrix4fv(shadowProg->getUniform("M"), 1, GL_FALSE, &M_Sponza[0][0]);
        sponza->draw(shadowProg, false);	//draw sponza

        //done, unbind stuff
        shadowProg->unbind();

        skinShadowProg->bind();
        glUniform3fv(skinShadowProg->getUniform("camPos"), 1, &mycam.pos[0]);
        glUniformMatrix4fv(skinShadowProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(skinShadowProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        M = translate(mat4(1), vec3(0, .1, 0)) * translate(mat4(1), vec3(0, -5.25, 2.5)) * scale(mat4(1), vec3(.013));
        glUniformMatrix4fv(skinShadowProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        const float marioAnimationSpeed = 30.0f;	//TODO: I want to move the mario model to its own files and this should be a class var
        skmesh.setBoneTransformations(skinShadowProg->pid, total_time * marioAnimationSpeed);
        skmesh.Render(glGetUniformLocation(skinShadowProg->pid, "tex"), glGetUniformLocation(skinShadowProg->pid, "texNormal"));
        skinShadowProg->unbind();

        glEnable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
        glGenerateMipmap(GL_TEXTURE_2D);
        glFrontFace(GL_CCW);
    }

    void render_to_bloom()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fb_merge_bloom);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3};
        glDrawBuffers(4, buffers);
        glClearColor(0.0, 0.0, 0.0, 1.0);

        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        get_exposure_data();

        prog_gen_lighting->bind();
        glUniformMatrix4fv(prog_gen_lighting->getUniform("P_light"), 1, GL_FALSE, &P_light[0][0]);
        glUniformMatrix4fv(prog_gen_lighting->getUniform("V_light"), 1, GL_FALSE, &V_light[0][0]);
        glUniform3fv(prog_gen_lighting->getUniform("campos"), 1, &mycam.pos.x);
        glUniform3fv(prog_gen_lighting->getUniform("lp"), 1, &lightPos.x);
        glUniform1f(prog_gen_lighting->getUniform("exposure"), exposure);
        glUniform1f(prog_gen_lighting->getUniform("max_luminance"), max_luminance);
        glUniform1i(prog_gen_lighting->getUniform("ssaoOn"), false);
        glUniform1i(prog_gen_lighting->getUniform("bloom"), bloom);
        glUniform1i(prog_gen_lighting->getUniform("tonemap"), tonemap);
        glUniform1i(prog_gen_lighting->getUniform("raycast_on"), raycast_on);
        glUniform3fv(prog_gen_lighting->getUniform("rt_dome"), 30, glm::value_ptr(rt_dome[0]));
        glUniform2f(prog_gen_lighting->getUniform("noiseScale"), width / 4.0, height / 4.0);
        glUniform1i(prog_gen_lighting->getUniform("num_raymarches"), raymarch_count);
        glUniform1i(prog_gen_lighting->getUniform("num_random_samples"), num_random_samples);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, FBOColorTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, PosTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, NormTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, FBOtex_shadowMapDepth);
        glBindTextureUnit(4, voxelizer->voxelTex_alpha);
        glBindTextureUnit(5, voxelizer->voxelTex_float);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, mask_tex);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        prog_gen_lighting->unbind();

        glBindTexture(GL_TEXTURE_2D, FBOtex_world_nobloom);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, FBOtex_bloom_pass[0]);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, FBOtex_world_nobloom_hdr);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, FBOtex_ray_denoise_pass[0]);
        glGenerateMipmap(GL_TEXTURE_2D);

        glDrawBuffers(1, buffers);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void render_bloom_passes(int passID)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fb_bloom_passes[passID]);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(1, buffers);
        // Get current frame buffer size.

        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        prog_bloom_pass->bind();
        glUniform1i(prog_bloom_pass->getUniform("direction"), passID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, FBOtex_bloom_pass[!passID]);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        prog_bloom_pass->unbind();

        glBindTexture(GL_TEXTURE_2D, FBOtex_bloom_pass[passID]);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void render_ray_denoise_passes(int passID)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fb_ray_denoise_passes[passID]);
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(1, buffers);
        // Get current frame buffer size.

        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        prog_ray_denoise->bind();
        glUniform1i(prog_ray_denoise->getUniform("direction"), passID);
        glUniformMatrix4fv(prog_ray_denoise->getUniform("V_eye"), 1, GL_FALSE, &V_eye[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, FBOtex_ray_denoise_pass[!passID]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, PosTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, NormTex);
        glBindVertexArray(VertexArrayIDBox);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        prog_ray_denoise->unbind();

        glBindTexture(GL_TEXTURE_2D, FBOtex_ray_denoise_pass[passID]);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void compute_exposure()
    {
        glUseProgram(computeProgram);
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

        luminance_info.sum_luminance = 0;
        luminance_info.max_luminance = 0;

        GLuint block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
        GLuint ssbo_binding_point_index = 0;
        glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &luminance_info, GL_DYNAMIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
        glBindImageTexture(0, FBOtex_world_nobloom_hdr, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
        glDispatchCompute(width, height, 1);				//start compute shader
        
        //unbind computeProgram
        glUseProgram(0);
    }

    void get_exposure_data()
    {
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ssbo_data), &luminance_info);

        double set_val = (luminance_info.sum_luminance / 100.0f) / ((float)width * height);

        set_val = mapOntoRange(set_val, 2.5, 0.1, 0.25, 0.01);


        exposure += ((double)set_val - exposure) * frametime * 0.25;

        set_val = mapOntoRange(luminance_info.max_luminance / 100.0f, 0.06, 0.01, 0.24, 0.07);


        max_luminance += ((double)set_val - max_luminance) * frametime * 0.25;
    }

    float mapOntoRange(double input, double output_end, double output_start, double input_end, double input_start)
    {
        double slope = 1.0 * (output_end - output_start) / (input_end - input_start);
        return output_start + slope * (input - input_start);
    }

};
//*********************************************************************************************************
int main(int argc, char** argv)
{
    // Where the resources are loaded from
    std::string resourceDir = "../resources";

    if (argc >= 2)
    {
        resourceDir = argv[1];
    }

    Application* application = new Application();

    // Your main will always include a similar set up to establish your window
    // and GL context, etc.

    WindowManager* windowManager = new WindowManager("572 Project");
    windowManager->init(1920, 1080);
    windowManager->setEventCallbacks(application);
    application->windowManager = windowManager;

    // This is the code that will likely change program to program as you
    // may need to initialize or set up different data and state

    application->init(resourceDir);
    application->initGeom(resourceDir);

    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(windowManager->getHandle()))
    {
        
        application->render_to_shadowmap();
        application->render_voxels();
        application->render_3_sample();
        /*if (aoMode)
            application->render_ssao();
        else
            application->render_hbao();
        application->render_ssao_blur();*/
        application->render_to_bloom();
        for (int i = 0; i < 4; i++)
        {
            application->render_bloom_passes(1);
            application->render_bloom_passes(0);
        }
        application->compute_exposure();
        if (application->blur_on) {
            for (int i = 0; i < 10; i++) {
                application->render_ray_denoise_passes(1);
                application->render_ray_denoise_passes(0);
            }
        }
        application->render_to_screen();

        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }
    application->paths[0].SavePathToFile("path2.txt");

    // Quit program.
    windowManager->shutdown();
    return 0;
}
