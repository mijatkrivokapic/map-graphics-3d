#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>

// GLM Matematika
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Tvoje klase iz vežbi
#include "../Header/shader.hpp" // Ocekujemo da imas ovo iz vezbi posto ga Model koristi
#include "../Header/model.hpp"
#include "../Header/TextUtil.h" // Tvoj stari header za tekst

// --- PODEŠAVANJA ---
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

// Kamera
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 5.0f); // Početna pozicija
glm::vec3 cameraFront = glm::vec3(0.0f, -0.5f, -1.0f); // Gleda blago dole
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Tajming
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Stanje aplikacije
bool isWalkingMode = true; // True = Setanje, False = Merenje
float walkHeight = 2.0f;   // Visina kamere u setanju
float measureHeight = 10.0f; // Visina kamere u merenju (ptičija perspektiva)

// Osvetljenje
glm::vec3 lightPos(0.0f, 5.0f, 0.0f);

glm::vec3 playerPos = glm::vec3(0.0f, 0.0f, 0.0f); // Igrac krece od centra
float playerSpeed = 5.0f; // Brzina kretanja
float playerRotation = 180.0f;

float totalDistance = 0.0f; // Ukupna predjena razdaljina
glm::vec3 lastPlayerPos = glm::vec3(0.0f, 0.0f, 0.0f);

// Deklaracije funkcija
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void renderMap(unsigned int vao, Shader& shader, unsigned int texture);

int main()
{
    // 1. INICIJALIZACIJA GLFW i GLEW (Isto kao pre)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Mapa Projekat", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // 2. GLOBALNA OPENGL PODEŠAVANJA
    glEnable(GL_DEPTH_TEST); // OBAVEZNO za 3D! Da se objekti ne bi provideli pogrešno

    // Učitavanje Shadera (koristeći klasu iz vežbi)
    // Pretpostavljam da su fajlovi u folderu "Shaders"
    Shader phongShader("Shaders/phong.vert", "Shaders/phong.frag");
    Shader uiShader("Shaders/text.vert", "Shaders/text.frag"); // Tvoj stari shader za tekst

    // 3. UČITAVANJE MODELA
    // Putanje prilagodi svojim fajlovima
    Model humanoidModel("Resources/bob-model/bob_the_builder.obj"); // Tvoj lik
    Model pinModel("Resources/low-poly-fox.obj");        // Tvoja čioda

    // 4. GENERISANJE MAPE (Pravougaonik u XZ ravni)
    // Mapa leži na Y=0, proteže se od -10 do 10
    float mapVertices[] = {
        -10.0f, 0.0f, -10.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, // Gore Levo
         10.0f, 0.0f, -10.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // Gore Desno
         10.0f, 0.0f,  10.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Dole Desno

        -10.0f, 0.0f, -10.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, // Gore Levo
         10.0f, 0.0f,  10.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Dole Desno
        -10.0f, 0.0f,  10.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f  // Dole Levo
    };

    unsigned int mapVAO, mapVBO;
    glGenVertexArrays(1, &mapVAO);
    glGenBuffers(1, &mapVBO);

    glBindVertexArray(mapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mapVertices), mapVertices, GL_STATIC_DRAW);

    // Layout 0: Pozicija (3 floata)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Layout 1: Normala (3 floata) - Preskacemo prva 3
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Layout 2: Tekstura (2 floata) - Preskacemo prvih 6
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    // Layout 2: TexCoord (Ako tvoj phong shader podržava teksture, dodajemo i ovo)
    // Ako phong.vert nema layout 2, ovo preskoči ili prilagodi shader.

    // Učitaj teksturu mape (koristeći tvoju ili bibliotečku funkciju)
    unsigned int mapTexture = TextureFromFile("map.jpg", "Resources");

    // Inicijalizacija tvog tekst sistema
    initText(uiShader.ID, "Resources/Antonio-Regular.ttf");


    // --- GLAVNA PETLJA ---
    while (!glfwWindowShouldClose(window))
    {
        // 1. Računanje vremena
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 2. Input
        processInput(window);

        // 3. Čišćenje ekrana
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ===========================================
        // 1. FAZA: 3D SCENA (Mapa, Lik, Čiode)
        // ===========================================
        glEnable(GL_DEPTH_TEST);
        phongShader.use();

        // --- A) Globalna podešavanja za 3D (Svetlo i Kamera) ---
        phongShader.setVec3("uLight.pos", lightPos);
        phongShader.setVec3("uLight.kA", 0.2f, 0.2f, 0.2f);
        phongShader.setVec3("uLight.kD", 0.8f, 0.8f, 0.8f);
        phongShader.setVec3("uLight.kS", 1.0f, 1.0f, 1.0f);
        phongShader.setVec3("uViewPos", cameraPos);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        if (isWalkingMode) {
            // Kamera prati igraca:
            // X i Z su isti kao igrac (ili malo iza), Y je visoko iznad
            // Namestamo da kamera bude 4 jedinice IZA (po Z) i 3 jedinice IZNAD (po Y)
            view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

            float dist = glm::distance(playerPos, lastPlayerPos);

            // Dodaj na ukupnu sumu
            totalDistance += dist;

            // Azuriraj proslu poziciju za sledeci krug
            lastPlayerPos = playerPos;
        }
        else {
            // Rezim merenja: Kamera je visoko (pticija perspektiva)
            // Ovde ne menjamo cameraPos automatski, nego ga kontrolisu strelice u processInput
            cameraPos.y = 10.0f; // Fiksna visina za mapu
            view = glm::lookAt(cameraPos, cameraPos + glm::vec3(0.0f, -1.0f, 0.0f), cameraUp);
        }

        phongShader.setMat4("uP", projection);
        phongShader.setMat4("uV", view);


        // --- B) Iscrtavanje MAPE ---
        glm::mat4 model = glm::mat4(1.0f);
        phongShader.setMat4("uM", model);

        // Materijal mape
        phongShader.setVec3("uMaterial.kA", 0.5f, 0.5f, 0.5f);
        phongShader.setVec3("uMaterial.kD", 0.6f, 0.6f, 0.6f);
        phongShader.setVec3("uMaterial.kS", 0.1f, 0.1f, 0.1f);
        phongShader.setFloat("uMaterial.shine", 32.0f);

        // Tekstura mape (UKLJUČENA)
        phongShader.setInt("uUseTexture", 1);
        phongShader.setInt("texture_diffuse1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mapTexture);

        glBindVertexArray(mapVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6); // Crtamo mapu SAMO JEDNOM ovde

        // --- C) Iscrtavanje LIKA (Kocke/Modela) ---
        if (isWalkingMode) {
            glm::mat4 model = glm::mat4(1.0f);

            // 1. Postavi lika na njegovu poziciju
            model = glm::translate(model, playerPos);

            // 2. Rotiraj ga da gleda gde se krece
            model = glm::rotate(model, glm::radians(playerRotation), glm::vec3(0.0f, 1.0f, 0.0f));

            // 3. Skaliraj ga (zadrzi ono sto smo nasli da radi, npr 0.05f)
            model = glm::scale(model, glm::vec3(0.01f));

            phongShader.setMat4("uM", model);

            // Resetuj materijal na belo da se vidi originalna boja teksture
            phongShader.setVec3("uMaterial.kA", 1.0f, 1.0f, 1.0f);
            phongShader.setVec3("uMaterial.kD", 1.0f, 1.0f, 1.0f);
            phongShader.setVec3("uMaterial.kS", 0.5f, 0.5f, 0.5f);

            phongShader.setInt("uUseTexture", 1);
            humanoidModel.Draw(phongShader);
        }
        else {
            // Rezim merenja: Kamera je visoko (pticija perspektiva)
            // Ovde ne menjamo cameraPos automatski, nego ga kontrolisu strelice u processInput
            cameraPos.y = 10.0f; // Fiksna visina za mapu
            view = glm::lookAt(cameraPos, cameraPos + glm::vec3(0.0f, -1.0f, 0.0f), cameraUp);
        }


        // ===========================================
        // 2. FAZA: 2D UI (Tekst i ikone)
        // ===========================================
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Pazi: RenderText funkcija obicno sama radi UseProgram, ali nije na odmet
        glUseProgram(uiShader.ID);

        glm::mat4 uiProjection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
        glUniformMatrix4fv(glGetUniformLocation(uiShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(uiProjection));

        std::stringstream ss;
        // Formatiramo broj na 2 decimale (npr. 12.45 m)
        ss << "Ukupna predjena distanca: " << std::fixed << totalDistance;

        // Ispisujemo gore levo (X=25, Y=Visina-50)
        // Boja: Zuta (1.0, 1.0, 0.0) da se razlikuje
        RenderText(uiShader.ID, ss.str(), 25.0f, SCR_HEIGHT - 50.0f, 1.0f, 1.0f, 1.0f, 0.0f);

        // Reset za sledeci krug
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	// 5. Čišćenje resursa
	glDeleteVertexArrays(1, &mapVAO);
	glDeleteBuffers(1, &mapVBO);
	glDeleteTextures(1, &mapTexture);

	glfwTerminate();
	return 0;
}

// Funkcija za obradu unosa
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Promena režima (Space)
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        isWalkingMode = !isWalkingMode;
        spacePressed = true;

        // Podesavanje visine i ugla kamere pri promeni rezima
        if (isWalkingMode) {
            cameraPos.y = 2.0f; // Nize za setnju
            // Gleda blago dole da bi video lika i mapu
            cameraFront = glm::vec3(0.0f, -0.5f, -1.0f);
        }
        else {
            cameraPos.y = 10.0f; // Visoko za merenje
            // Gleda skoro skroz dole (kao 2D)
            cameraFront = glm::vec3(0.0f, -0.99f, -0.1f);
        }
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spacePressed = false;
    }

    // Brzine
    float velocity = playerSpeed * deltaTime;
    float camSpeed = 5.0f * deltaTime; // Brzina kamere

    if (isWalkingMode)
    {
        // 1. KRETANJE LIKA (WASD) - Samo menja playerPos
        glm::vec3 oldPos = playerPos;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            playerPos.z -= velocity;
            playerRotation = 180.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            playerPos.z += velocity;
            playerRotation = 0.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            playerPos.x -= velocity;
            playerRotation = -90.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            playerPos.x += velocity;
            playerRotation = 90.0f;
        }

        // Ogranicenje lika na mapi
        if (playerPos.x < -10.0f || playerPos.x > 10.0f || playerPos.z < -10.0f || playerPos.z > 10.0f)
            playerPos = oldPos;

        // 2. KRETANJE KAMERE (Strelice) - Nezavisno od lika!
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPos.z -= camSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPos.z += camSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraPos.x -= camSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraPos.x += camSpeed;
    }
    else
    {
        // REZIM MERENJA: Strelice pomeraju KAMERU (Isto kao gore, ali mozda brze ili drugacije)
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPos.z -= camSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPos.z += camSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraPos.x -= camSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraPos.x += camSpeed;
    }

    if (cameraPos.x > 10.0f) cameraPos.x = 10.0f;
    if (cameraPos.x < -10.0f) cameraPos.x = -10.0f;

    if (cameraPos.z > 15.0f) cameraPos.z = 15.0f;
    if (cameraPos.z < -10.0f) cameraPos.z = -10.0f;


}
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}