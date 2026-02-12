#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Header/shader.hpp"
#include "../Header/model.hpp"
#include "../Header/TextUtil.h"

// --- CONSTANTS & SETTINGS ---
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
const float MAP_SIZE = 10.0f;

// --- GLOBAL STATE ---

// Camera & Modes
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 2.0f);
glm::vec3 cameraFront =glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f));
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Saved positions for mode switching
glm::vec3 savedWalkPos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::vec3 savedMeasurePos = glm::vec3(0.0f, 10.0f, 10.0f);

bool isWalkingMode = true;

// Player
glm::vec3 playerPos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 lastPlayerPos = glm::vec3(0.0f, 0.0f, 0.0f);
float playerSpeed = 5.0f;
float playerRotation = 180.0f;
float totalWalkDistance = 0.0f;

// Measurement (Pins)
std::vector<glm::vec3> measurementPoints;
float totalMeasuredLength = 0.0f;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// UI
float iconSize = 200.0f;
float iconPadding = 20.0f;

// Rendering Resources (VAO/VBO/Textures)
unsigned int mapVAO, mapVBO, mapTexture;
unsigned int uiVAO, uiVBO, iconWalkTex, iconMeasureTex;
unsigned int lineVAO, lineVBO;

bool depthTestEnabled = true;
bool faceCullingEnabled = false;

const double targetFPS = 75.0;
const double frameTimeLimit = 1.0 / targetFPS;

// --- FUNCTION PROTOTYPES ---
GLFWwindow* InitGLFW();
void InitScene();
void ProcessInput(GLFWwindow* window);
void RenderScene(Shader& shader, Model& humanoid, Model& pin);
void RenderUI(Shader& shader, Shader& textShader);
void ToggleMode();
bool GetGroundIntersection(GLFWwindow* window, double mouseX, double mouseY, glm::vec3& outIntersection);

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// ----------------------------------------------------------------------------
// MAIN FUNCTION
// ----------------------------------------------------------------------------
int main()
{
    // 1. Initialize Window & OpenGL
    GLFWwindow* window = InitGLFW();
    if (!window) return -1;

    // 2. Load Shaders & Models
    Shader phongShader("Shaders/phong.vert", "Shaders/phong.frag");
    Shader uiShader("Shaders/text.vert", "Shaders/text.frag");

    Model humanoidModel("Resources/bob-model/bob_the_builder.obj");
    Model pinModel("Resources/pin-model/map_pin.obj");

    // 3. Initialize Geometry (Map, UI Quad, Lines)
    InitScene();

    // 4. Initialize Text System
    initText(uiShader.ID, "Resources/Antonio-Regular.ttf");

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // --- MAIN LOOP ---
    while (!glfwWindowShouldClose(window))
    {
        // Timing
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;

        if (deltaTime < frameTimeLimit) {
            continue;
        }

        lastFrame = currentFrame;

        // Input
        ProcessInput(window);

        // Clear Screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- RENDER 3D SCENE ---
        RenderScene(phongShader, humanoidModel, pinModel);

        // --- RENDER 2D UI ---
        RenderUI(phongShader, uiShader);

        // Swap Buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &mapVAO);
    glDeleteBuffers(1, &mapVBO);
    glDeleteTextures(1, &mapTexture);

    glfwTerminate();
    return 0;
}

// ----------------------------------------------------------------------------
// INITIALIZATION FUNCTIONS
// ----------------------------------------------------------------------------
GLFWwindow* InitGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "3D Map Project", primaryMonitor, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }

    glfwMakeContextCurrent(window);
    glViewport(0, 0, mode->width, mode->height);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);

    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return NULL;
    }
    return window;
}

void InitScene() {
    // A) Map Setup (Plane on XZ axis)
    float mapVertices[] = {
        // Pos                          // Normals          // TexCoords
        -10.0f, 0.0f, -10.0f,          0.0f, 1.0f, 0.0f,       0.0f, 0.0f, 
        -10.0f, 0.0f,  10.0f,          0.0f, 1.0f, 0.0f,       0.0f, 1.0f, 
         10.0f, 0.0f,  10.0f,          0.0f, 1.0f, 0.0f,       1.0f, 1.0f, 

         -10.0f, 0.0f, -10.0f,          0.0f, 1.0f, 0.0f,       0.0f, 0.0f, 
          10.0f, 0.0f,  10.0f,          0.0f, 1.0f, 0.0f,       1.0f, 1.0f, 
          10.0f, 0.0f, -10.0f,          0.0f, 1.0f, 0.0f,       1.0f, 0.0f
    };

    glGenVertexArrays(1, &mapVAO);
    glGenBuffers(1, &mapVBO);
    glBindVertexArray(mapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mapVertices), mapVertices, GL_STATIC_DRAW);

    // Layouts: 0=Pos, 1=Norm, 2=Tex
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    mapTexture = TextureFromFile("map.jpg", "Resources");

    // B) UI Quad Setup
    float uiVertices[] = {
        // Pos(x,y,z)               // Normal               // Tex
        0.0f, 0.0f, 0.0f,          0.0f, 0.0f, 1.0f,       0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,          0.0f, 0.0f, 1.0f,       1.0f, 1.0f, 
        0.0f, 1.0f, 0.0f,          0.0f, 0.0f, 1.0f,       0.0f, 0.0f, 

        1.0f, 0.0f, 0.0f,          0.0f, 0.0f, 1.0f,       1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,          0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,          0.0f, 0.0f, 1.0f,       0.0f, 0.0f
    };

    glGenVertexArrays(1, &uiVAO);
    glGenBuffers(1, &uiVBO);
    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uiVertices), uiVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    iconWalkTex = TextureFromFile("walking.png", "Resources");
    iconMeasureTex = TextureFromFile("ruler.png", "Resources");

    // C) Line Setup
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// ----------------------------------------------------------------------------
// RENDER LOGIC
// ----------------------------------------------------------------------------
void RenderScene(Shader& shader, Model& humanoid, Model& pin) {
    shader.use();

    // 1. Setup Lights
    shader.setVec3("uSun.direction", -0.2f, -1.0f, -0.3f);
    shader.setVec3("uSun.kA", 0.4f, 0.4f, 0.4f);
    shader.setVec3("uSun.kD", 0.4f, 0.4f, 0.4f);
    shader.setVec3("uSun.kS", 0.25f, 0.25f, 0.25f);
    shader.setVec3("uViewPos", cameraPos);

    // Point lights (Pins)
    int nrLights = (!isWalkingMode) ? std::min((int)measurementPoints.size(), 32) : 0;
    shader.setInt("uNrActiveLights", nrLights);

    for (int i = 0; i < nrLights; i++) {
        std::string num = std::to_string(i);
        shader.setVec3("pointLights[" + num + "].position", measurementPoints[i] + glm::vec3(0.0f, 1.5f, 0.0f));
        shader.setVec3("pointLights[" + num + "].kA", 0.0f, 0.0f, 0.0f);
        shader.setVec3("pointLights[" + num + "].kD", 0.5f, 0.0f, 0.0f); // Red light
        shader.setVec3("pointLights[" + num + "].kS", 0.5f, 0.0f, 0.0f);
        shader.setFloat("pointLights[" + num + "].constant", 1.0f);
        shader.setFloat("pointLights[" + num + "].linear", 0.35f);
        shader.setFloat("pointLights[" + num + "].quadratic", 0.44f);
    }

    // 2. Setup Matrices
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
    shader.setMat4("uP", projection);
    shader.setMat4("uV", view);

    // 3. Draw Map
    glm::mat4 model = glm::mat4(1.0f);
    shader.setMat4("uM", model);
    shader.setVec3("uMaterial.kA", 1.0f, 1.0f, 1.0f);
    shader.setVec3("uMaterial.kD", 1.0f, 1.0f, 1.0f);
    shader.setVec3("uMaterial.kS", 0.2f, 0.2f, 0.2f);
    shader.setFloat("uMaterial.shine", 32.0f);

    shader.setInt("uUseTexture", 1);
    shader.setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTexture);
    glBindVertexArray(mapVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 4. Draw Player (Walking Mode)
    if (isWalkingMode) {
        float dist = glm::distance(playerPos, lastPlayerPos);
        totalWalkDistance += dist;
        lastPlayerPos = playerPos;

        model = glm::mat4(1.0f);
        model = glm::translate(model, playerPos);
        model = glm::rotate(model, glm::radians(playerRotation), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f));
        shader.setMat4("uM", model);
        humanoid.Draw(shader);
    }
    // 5. Draw Measurement Tools (Measuring Mode)
    else {
        shader.setInt("uUseTexture", 0); 

        // Draw Pins
        shader.setVec3("uMaterial.kA", 1.0f, 0.0f, 0.0f);
        shader.setVec3("uMaterial.kD", 1.0f, 0.0f, 0.0f);

        for (auto& point : measurementPoints) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, point);
            model = glm::translate(model, glm::vec3(0.0f, 0.75f, 0.0f));
            model = glm::scale(model, glm::vec3(0.2f));
            shader.setMat4("uM", model);
            pin.Draw(shader);
        }

        // Draw Lines
        if (measurementPoints.size() > 1) {
            shader.setVec3("uMaterial.kA", 1.0f, 0.0f, 0.0f);
            shader.setVec3("uMaterial.kD", 0.0f, 0.0f, 0.0f);

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.05f, 0.0f)); // Slightly above ground
            shader.setMat4("uM", model);

            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, measurementPoints.size() * sizeof(glm::vec3), &measurementPoints[0], GL_DYNAMIC_DRAW);
            glBindVertexArray(lineVAO);
            glLineWidth(5.0f);
            glDrawArrays(GL_LINE_STRIP, 0, measurementPoints.size());
            glLineWidth(1.0f);
        }
    }
}

void RenderUI(Shader& shader, Shader& textShader) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // A) Icon
    shader.use();
    glm::mat4 uiProj = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    shader.setMat4("uP", uiProj);
    shader.setMat4("uV", glm::mat4(1.0f));

    glm::mat4 model = glm::mat4(1.0f);
    float iconX = SCR_WIDTH - (iconSize + iconPadding);
    float iconY = SCR_HEIGHT - (iconSize + iconPadding);
    model = glm::translate(model, glm::vec3(iconX, iconY, 0.0f));
    model = glm::scale(model, glm::vec3(iconSize, iconSize, 1.0f));
    shader.setMat4("uM", model);

    // Unlit material for UI
    shader.setVec3("uSun.kA", 1.0f, 1.0f, 1.0f);
    shader.setVec3("uSun.kD", 0.0f, 0.0f, 0.0f);
    shader.setInt("uNrActiveLights", 0);
    shader.setVec3("uMaterial.kA", 1.0f, 1.0f, 1.0f);
    shader.setVec3("uMaterial.kD", 0.0f, 0.0f, 0.0f);

    shader.setInt("uUseTexture", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, isWalkingMode ? iconWalkTex : iconMeasureTex);

    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // B) Text
    glUseProgram(textShader.ID);
    glUniformMatrix4fv(glGetUniformLocation(textShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(uiProj));

    std::stringstream ss;
    if (isWalkingMode) {
        ss << "Ukupna predjena distanca: " << std::fixed << totalWalkDistance;
    }
    else {
        ss << "Ukupna izmerena distanca: " << std::fixed << totalMeasuredLength;
    }
    RenderText(textShader.ID, ss.str(), 25.0f, SCR_HEIGHT - 50.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    RenderText(textShader.ID, "Mijat Krivokapic SV41/2022", 25.0f, 25.0f, 1.0f, 1.0f, 1.0f, 0.0f);
}

// ----------------------------------------------------------------------------
// LOGIC & INPUT
// ----------------------------------------------------------------------------
void ToggleMode() {
    if (isWalkingMode) {
        savedWalkPos = cameraPos;
        isWalkingMode = false;
        cameraPos = savedMeasurePos;
    }
    else {
        // Switch to Walking
        savedMeasurePos = cameraPos;
        isWalkingMode = true;
        cameraPos = savedWalkPos;
    }
}

void ProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float velocity = playerSpeed * deltaTime;
    float camSpeed = 5.0f * deltaTime;

    if (isWalkingMode) {
        // Player Movement
        glm::vec3 oldPos = playerPos;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { playerPos.z -= velocity; playerRotation = 180.0f; }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { playerPos.z += velocity; playerRotation = 0.0f; }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { playerPos.x -= velocity; playerRotation = -90.0f; }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { playerPos.x += velocity; playerRotation = 90.0f; }

        // Bounds check
        if (playerPos.x < -MAP_SIZE || playerPos.x > MAP_SIZE || playerPos.z < -MAP_SIZE || playerPos.z > MAP_SIZE)
            playerPos = oldPos;
    }

    // Camera Movement
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    cameraPos.z -= camSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  cameraPos.z += camSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  cameraPos.x -= camSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraPos.x += camSpeed;

    // Camera Limits
    if (cameraPos.x > MAP_SIZE) cameraPos.x = MAP_SIZE;
    if (cameraPos.x < -MAP_SIZE) cameraPos.x = -MAP_SIZE;

    if (isWalkingMode) {
        if (cameraPos.z > 12.0f) cameraPos.z = 12.0f;
        if (cameraPos.z < -8.0f) cameraPos.z = -8.0f;
    }
    else {
        if (cameraPos.z > 20.0f) cameraPos.z = 20.0f;
        if (cameraPos.z < 0.0f) cameraPos.z = 0.0f;
    }
    
}

// Helper: Performs Raycasting to find where the mouse clicks on the map (y=0)
bool GetGroundIntersection(GLFWwindow* window, double mouseX, double mouseY, glm::vec3& outIntersection) {
    // 1. Reconstruct Matrices
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec4 viewport = glm::vec4(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // 2. Unproject 2D mouse coordinates to 3D Ray
    glm::vec3 winPosNear = glm::vec3(mouseX, SCR_HEIGHT - mouseY, 0.0f); // Near plane
    glm::vec3 winPosFar = glm::vec3(mouseX, SCR_HEIGHT - mouseY, 1.0f); // Far plane

    glm::vec3 rayStart = glm::unProject(winPosNear, view, projection, viewport);
    glm::vec3 rayEnd = glm::unProject(winPosFar, view, projection, viewport);
    glm::vec3 rayDir = glm::normalize(rayEnd - rayStart);

    // 3. Find intersection with Plane Y=0
    // formula: t = -start.y / dir.y
    if (rayDir.y == 0.0f) return false;

    float t = -rayStart.y / rayDir.y;
    if (t < 0.0f) return false; // Intersection is behind camera

    outIntersection = rayStart + rayDir * t;
    return true;
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // A. CHECK ICON CLICK
    float iconLeft = SCR_WIDTH - (iconSize + iconPadding);
    float iconRight = SCR_WIDTH - iconPadding;
    float iconTop = iconPadding;
    float iconBottom = iconPadding + iconSize;

    if (xpos >= iconLeft && xpos <= iconRight && ypos >= iconTop && ypos <= iconBottom) {
        ToggleMode();
        return;
    }

    // B. CHECK MAP CLICK (Only in Measuring Mode)
    if (!isWalkingMode) {
        glm::vec3 hitPoint;
        if (GetGroundIntersection(window, xpos, ypos, hitPoint)) {
            // Check Map Bounds
            if (hitPoint.x >= -MAP_SIZE && hitPoint.x <= MAP_SIZE && hitPoint.z >= -MAP_SIZE && hitPoint.z <= MAP_SIZE) {

                // Logic: Delete if close to existing, else Add
                bool deleted = false;
                for (size_t i = 0; i < measurementPoints.size(); i++) {
                    if (glm::distance(measurementPoints[i], hitPoint) < 0.5f) {
                        measurementPoints.erase(measurementPoints.begin() + i);
                        deleted = true;
                        break;
                    }
                }

                if (!deleted) {
                    measurementPoints.push_back(hitPoint);
                }

                // Recalculate total length
                totalMeasuredLength = 0.0f;
                if (measurementPoints.size() > 1) {
                    for (size_t i = 0; i < measurementPoints.size() - 1; i++) {
                        totalMeasuredLength += glm::distance(measurementPoints[i], measurementPoints[i + 1]);
                    }
                }
            }
        }
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        ToggleMode();
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        depthTestEnabled = !depthTestEnabled;
        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        std::cout << "Depth Test: " << (depthTestEnabled ? "ON" : "OFF") << std::endl;
    }

    // Taster M za Face Culling
    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        faceCullingEnabled = !faceCullingEnabled;
        if (faceCullingEnabled) glEnable(GL_CULL_FACE);
        else glDisable(GL_CULL_FACE);
        std::cout << "Face Culling: " << (faceCullingEnabled ? "ON" : "OFF") << std::endl;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}