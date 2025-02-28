// Graphics libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Imgui lib
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

// Custom includes
#include "stb_image.h"
#include <SHADER/shader_c.h>
#include "mine_imgui.h"
#include "scene.h"

// standart libraries
#include <iostream>
#include <string.h>
#include <vector>

// Initialisations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, Camera* camera);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void toggle_flashlight(GLFWwindow* window, SpotLight& flashlight);
void renderLightQuad(const glm::vec3& lightPos, const glm::mat4& view, Shader& shader);
void renderQuad();

// Global variable setup
bool debug = 0;
bool renderImGUI = 0;
float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
Camera* camera;
float lastX = 400, lastY = 300;
unsigned int quadVAO, quadVBO;
int fps;
World_param world;
int debugLayer = 0;

Model* createPlaneModel(const std::string& textureFile, const std::string& path) {
    vector<Vertex> vertices = {
        // Positions          // Normals        // Texture Coords
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // Bottom-left
        {{0.5f, 0.0f, -0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // Bottom-right
        {{0.5f, 0.0f, 0.5f},   {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}}, // Top-right
        {{-0.5f, 0.0f, 0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}  // Top-left
    };

    vector<unsigned int> indices = {
        2, 1, 0, // First triangle
        0, 3, 2 // Second triangle
    };

    vector<Texture> textures;
    Texture texture;
    texture.id = TextureFromFile(textureFile.c_str(), path.c_str()); // Load texture
    texture.type = "texture_diffuse";
    texture.path = textureFile;
    textures.push_back(texture);

    return new Model(Mesh(vertices, indices, textures));
}

int main(){
    // GLFW SET UP
    // =----------------------------------------------------------=
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Request OpenGL 4.0+
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5); // Request OpenGL 4.5
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    int screenWidth = mode->width;
    int screenHeight = mode->height;

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "LearnOpenGL", NULL, NULL);
    if (window == NULL){
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); 
    glfwMaximizeWindow(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    stbi_set_flip_vertically_on_load(false);
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();

    // OPENGL PARAMETRS
    // =------------------------------------------------------=
    // Set OPENGL parametrs
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Gamma correction
    glEnable(GL_FRAMEBUFFER_SRGB);

    // SOME PARAMETRS
    double previousTime = glfwGetTime();
    int frameCount = 0;

    // Setting up the SCENE
    //=-------------------------------------------------------=
    // build and compile shader program

    Shader shader("shader.vert", "shader.frag");
    Shader light_shader("lightsource.vert", "lightsource.frag");
    Shader single_color("shader.vert", "single_color.frag");
    Shader skyboxShader("skybox.vert", "skybox.frag");
    Shader DLdepth_shader("depthShader.vert", "DLightDepthShader.frag", "DLightDepthShader.geom");
    Shader PLdepth_shader("PlightDepthShader.vert", "PLightDepthShader.frag", "PLightDepthShader.geom");
    
    Scene scene;

    scene.setModelShader(shader);
    scene.setOutlineShader(single_color);
    scene.setSkyboxShader(skyboxShader);
    scene.setDLDepthShader(DLdepth_shader);
    scene.setPLDepthShader(PLdepth_shader);

    GLuint cubeMapArray;
    glGenTextures(1, &cubeMapArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, cubeMapArray);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY,
        0,
        GL_DEPTH_COMPONENT16,
        scene.properties.PLShadowResolution,
        scene.properties.PLShadowResolution,
        20*6,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);

    GLuint depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapArray, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!";
        throw 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //// Create a cube map array
    //GLuint cubeMapArray;
    //glGenTextures(1, &cubeMapArray);
    //GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubeMapArray));

    //// Allocate storage for the cube map array
    //GL_CHECK(glTexStorage3D(
    //    GL_TEXTURE_CUBE_MAP_ARRAY,
    //    1,                         
    //    GL_DEPTH_COMPONENT16,   
    //    scene.properties.PLShadowResolution, scene.properties.PLShadowResolution,
    //    20 * 6       
    //));

    //// Set texture parameters
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    //GLuint depthMapFBO;
    //glGenFramebuffers(1, &depthMapFBO);
    //glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    //glDrawBuffer(GL_NONE); // No color buffer is drawn to
    //glReadBuffer(GL_NONE); // No color buffer is read from

    //glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind the FBO

    //// Bind the cube map array to a texture unit
    //glActiveTexture(GL_TEXTURE4);
    //GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, cubeMapArray));

    unsigned int shader_ub = glGetUniformBlockIndex(shader.ID, "Matrices");
    unsigned int light_shader_ub = glGetUniformBlockIndex(light_shader.ID, "Matrices");
    unsigned int single_color_ub = glGetUniformBlockIndex(single_color.ID, "Matrices");
    unsigned int skyboxShader_ub = glGetUniformBlockIndex(skyboxShader.ID, "Matrices");

    glUniformBlockBinding(shader.ID, shader_ub, 0);
    glUniformBlockBinding(light_shader.ID, light_shader_ub, 0);
    glUniformBlockBinding(single_color.ID, single_color_ub, 0);
    glUniformBlockBinding(skyboxShader.ID, skyboxShader_ub, 0);

    unsigned int uboMatrices;
    glGenBuffers(1, &uboMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

    shader.use();
    shader.setInt("PLshadowMapArray", 4); // Use the same texture unit
    
    // Adding camera object
    camera = scene.AddCamera(glm::vec3(0.0f, 0.0f, 3.0f));

    // Adding ambient light object
    AmbientLight* ambientLight = scene.AddAmbientLight(glm::vec3(0.3f, 0.3f, 0.3f));

    // Add directional light to scene
    DirectionalLight* directionLight = scene.AddDirectionalLight(camera, glm::vec3(-0.2f, -0.5f, -0.3f), "Sun");

    // Create point lights and add to scene
    PointLight* pointLight = scene.AddPointLight(camera, glm::vec3(3.0f, 2.0f, 2.0f));
    //pointLight->setIntensity(1.0f);

    PointLight* pointLight1 = scene.AddPointLight(camera, glm::vec3(2.0f, 2.0f, 2.0f));

    PointLight* pointLight2 = scene.AddPointLight(camera, glm::vec3(1.0f, 2.0f, 2.0f));

    SpotLight* spotLight = scene.AddSpotLight("Flashlight");
    spotLight->update(&shader, 0);

    // Set path to cube object
    char path_cube[] = "resources/models/default/CUBE/default_cube.obj";
    
    // Add first cube to scene
    Model* default_cube_1 = scene.AddModel(path_cube, "Default_cube1");
    default_cube_1->setSelection(false);
    default_cube_1->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    default_cube_1->scale_texture = true;

    // Add second cube to scene
    Model* default_cube_2 = scene.AddModel(path_cube, "Default_cube2");
    default_cube_2->setSelection(false);
    default_cube_2->setPosition(glm::vec3(3.0f, 2.0f, 0.0f));
    default_cube_2->scale_texture = true;

    // Add plane to scene
    Model* plane = createPlaneModel("concrete_diffuse.png", "resources/textures/default");
    scene.AddModel(plane, "Plane");
    plane->setSelection(false);
    plane->setPosition(glm::vec3(1.5f, -1.0001f, 0.0f));
    plane->setSize(glm::vec3(20.0f));
    plane->scale_texture = true;


    // Add Skybox to scene
    Skybox* skybox = scene.AddSkybox();
    
    skyboxShader.setInt("skybox", 0);

    char path[] = "resources/models/backpack/backpack.obj";
    Model* backpack = scene.AddModel(path);
    backpack->setPosition(glm::vec3(-3.0, 1.0, 1.0));

    unsigned int light_texture = TextureFromFile("lightb.png", "resources/textures/default");

    // Creating framebuffer
    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // generate texture
    unsigned int texColorBuffer;
    glGenTextures(1, &texColorBuffer);
    glBindTexture(GL_TEXTURE_2D, texColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);
    // attach it to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

    // Creating renderbuffer
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" <<
        std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float quadVertices[] = {
        // positions // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f
    };

    Shader quadShader("quad.vert", "quad.frag");

    unsigned int fullquadVAO, fullquadVBO;
    glGenVertexArrays(1, &fullquadVAO);
    glGenBuffers(1, &fullquadVBO);
    glBindVertexArray(fullquadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fullquadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    // Position Attribute (x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Texture Coordinate Attribute (u, v)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // TESTING
    //=---------------------=
    //default_cube_1->setSelection(1);
    //default_cube_2->setSelection(1);

    shader.use();
    shader.setInt("DLshadowMap", 3);
    //shader.setInt("PLshadowMap", 4);
    // DEBUG QUAD SETUP
    // =----------------------=
    Shader debugDepthQuad("quad.vert", "debug_quad.frag");
    debugDepthQuad.use();
    debugDepthQuad.setInt("depthMap", 0);


    // RENDER LOOP
    // =--------------------------------------------------=
    while (!glfwWindowShouldClose(window)){
        // Gamma correction
        glEnable(GL_FRAMEBUFFER_SRGB);

        camera->screenHeight = screenHeight;
        camera->screenWidth = screenWidth;
        camera->uniform_buffer = uboMatrices;

        // input
        processInput(window, camera);
        toggle_flashlight(window, *spotLight);

        // DeltaTime calculation
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        glClearColor(world.color[0], world.color[1], world.color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        // RENDER_SCENE
        //=------------------------=
        glViewport(0, 0, screenWidth, screenHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        
        glEnable(GL_DEPTH_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        glEnable(GL_CULL_FACE);

        spotLight->setPosition(camera->getPosition());
        spotLight->setDirection(camera->Front);

        glStencilFunc(GL_ALWAYS, 1, 0xFF); // all fragments pass the stencil test
        glStencilMask(0xFF); // enable writing to the stencil buffer

        shader.use();

        //Material set
        //shader.setInt("numPointLights", 3);
        //shader.setInt("numSpotLights", 1);

        scene.DrawScene(0, fbo, screenWidth, screenHeight, cubeMapArray, depthMapFBO);

        light_shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, light_texture);
        light_shader.setVec3("lightPos", pointLight->getPosition());
        light_shader.setFloat("radius", 0.5f);
        light_shader.setVec3("lightColor", glm::vec3(1.0, 1.0, 1.0));
        light_shader.setVec3("viewPos", camera->getPosition());
        camera->update_shaders(&light_shader);

        renderLightQuad(pointLight->getPosition(), camera->GetViewMatrix(), light_shader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        quadShader.use();
        glBindVertexArray(fullquadVAO);
        glDisable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, texColorBuffer);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // render ImGui
        if(renderImGUI){
            // Measure framerate
            double currentTime = glfwGetTime();
            frameCount++;
            // If a second has passed.
            if (currentTime - previousTime >= 1.0) {
                // Display the frame count here any way you want.
                fps = frameCount;
                frameCount = 0;
                previousTime = currentTime;
            }

            // Gamma correction
            glDisable(GL_FRAMEBUFFER_SRGB);
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            //ImGui::ShowDemoWindow(); // Show demo window! :)
            ShowMyWindow(&scene, world, fps);
            // Render ImGUI ontop
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        debugDepthQuad.use();
        debugDepthQuad.setInt("layer", debugLayer);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, directionLight->depthMap);
        if (debug)
        {
            renderQuad();
        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteFramebuffers(1, &fbo);

    //clear ImGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window, Camera* camera){
    // Exit program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Toggle ImGUI
    static bool f5PressedLastFrame = false; // Track F5 state
    bool f5CurrentlyPressed = (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS);
    if (f5CurrentlyPressed && !f5PressedLastFrame){
        renderImGUI = !renderImGUI;
    }
    f5PressedLastFrame = f5CurrentlyPressed;

    // Toggle Debug
    static bool EPressedLastFrame = false; // Track F5 state
    bool ECurrentlyPressed = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
    if (ECurrentlyPressed && !EPressedLastFrame) {
        debug = !debug;
    }
    EPressedLastFrame = ECurrentlyPressed;

    // cycle the depthmaps
    static bool arrowPressedLastFrame = false; // Track F5 state
    bool arrowCurrentlyPressed = (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
    if (arrowCurrentlyPressed && !arrowPressedLastFrame) {
        if (debugLayer == 3)
            debugLayer = 0;
        else
            debugLayer++;
    }
    arrowPressedLastFrame = arrowCurrentlyPressed;

    bool shift = 0;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        shift = 1;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->ProcessKeyboard(FORWARD, shift, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
        camera->ProcessKeyboard(BACKWARD, shift, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->ProcessKeyboard(LEFT, shift, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(RIGHT, shift, deltaTime);
}

void toggle_flashlight(GLFWwindow* window, SpotLight& flashlight) {
    static bool fPressedLastFrame = false; // Track F5 state
    bool fCurrentlyPressed = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS);
    if (fCurrentlyPressed && !fPressedLastFrame) {
        if (flashlight.getIntensity() == 1.0f)
            flashlight.setIntensity(0.0f);
        else
            flashlight.setIntensity(1.0f);
    }

    fPressedLastFrame = fCurrentlyPressed;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent((float)xpos, (float)ypos);

    if (io.WantCaptureMouse)
        return; // ImGui is handling the mouse input

    static bool mouseHeld = false; // Track if the mouse button is held

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!mouseHeld) {
            // First press: update lastX and lastY to prevent a jump
            lastX = xpos;
            lastY = ypos;
            mouseHeld = true; // Mark that the button is now held
            return; // Skip movement this frame to avoid an initial jump
        }

        // Normal movement calculation
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed: y ranges bottom to top
        camera->ProcessMouseMovement(xoffset, yoffset, true);
    }
    else {
        mouseHeld = false; // Reset flag when the button is released
    }

    lastX = xpos;
    lastY = ypos;
}


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent((float)xoffset, (float)yoffset);

    if (io.WantCaptureMouse)
        return; // ImGui is handling the scroll input

    camera->ProcessMouseScroll(yoffset);
}

void renderLightQuad(const glm::vec3& lightPos, const glm::mat4& view, Shader& shader) {
    if (quadVAO == 0) {
        float quadVertices[] = {
            // Positions       // Texture Coords
            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, // Top-left
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, // Bottom-left
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom-right

            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, // Top-left
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f, // Bottom-right
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f  // Top-right
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);

        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Billboard transformation (Quad faces the camera)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, lightPos);  // Move quad to light position
    

    // Extract camera right and up vectors from the view matrix
    glm::vec3 cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);

    // Billboard rotation
    model[0] = glm::vec4(cameraRight, 0.0f);
    model[1] = glm::vec4(cameraUp, 0.0f);
    model[2] = glm::vec4(glm::cross(cameraRight, cameraUp), 0.0f);  // Forward vector
    model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));

    // Set uniforms
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);

    // Render the quad
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

 //renderQuad() renders a 1x1 XY quad in NDC
 //-----------------------------------------
unsigned int debug_quadVAO = 0;
unsigned int debug_quadVBO;
void renderQuad(){
    if (debug_quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &debug_quadVAO);
        glGenBuffers(1, &debug_quadVBO);
        glBindVertexArray(debug_quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debug_quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(debug_quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}