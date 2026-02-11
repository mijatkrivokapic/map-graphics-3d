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

std::vector<glm::vec3> measurementPoints; // Niz tačaka (čioda)
float measurementTotalLen = 0.0f;         // Ukupna dužina

// Za iscrtavanje linija
unsigned int lineVAO, lineVBO;

// Deklaracije funkcija
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void renderMap(unsigned int vao, Shader& shader, unsigned int texture);
void mouse_callback(GLFWwindow* window, int button, int action, int mods);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // --- FULL SCREEN KOD ---

    // 1. Pronadji primarni monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    // 2. Pronadji trenutnu rezoluciju tog monitora (npr. 1920x1080 @ 60Hz)
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    // 3. Kreiraj prozor koristeci te dimenzije
    // Cetvrti parametar 'primaryMonitor' znaci: "Otvori u Full Screen-u na ovom monitoru"
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "3D Mapa Projekat", primaryMonitor, NULL);

    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Azuriramo viewport da se slaze sa novom rezolucijom
    glViewport(0, 0, mode->width, mode->height);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    glfwSetMouseButtonCallback(window, mouse_callback);

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
    Model pinModel("Resources/pin-model/map_pin.obj");        // Tvoja čioda

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

    // --- SETUP ZA LINIJE ---
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    // Samo nam trebaju pozicije (X, Y, Z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


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

        // View matrica (ona tvoja zakošena logika)
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));

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
        glDrawArrays(GL_TRIANGLES, 0, 6);


        // --- C) SPECIFIČNO PO REŽIMIMA ---

        if (isWalkingMode)
        {
            // --- Logika setanja ---
            // Racunamo predjeni put
            float dist = glm::distance(playerPos, lastPlayerPos);
            totalDistance += dist;
            lastPlayerPos = playerPos;

            // --- Iscrtavanje LIKA ---
            model = glm::mat4(1.0f);
            model = glm::translate(model, playerPos);
            model = glm::rotate(model, glm::radians(playerRotation), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.01f)); // Pazi na skalu

            phongShader.setMat4("uM", model);

            // Materijal i tekstura lika
            phongShader.setVec3("uMaterial.kA", 1.0f, 1.0f, 1.0f);
            phongShader.setVec3("uMaterial.kD", 1.0f, 1.0f, 1.0f);
            phongShader.setVec3("uMaterial.kS", 0.5f, 0.5f, 0.5f);

            phongShader.setInt("uUseTexture", 1); // Lik koristi teksturu
            humanoidModel.Draw(phongShader);

        }
        else
        {
            // --- Logika merenja ---
            // U ovom rezimu ne crtamo lika, vec ciode i linije

            // 1. Iscrtavanje ČIODA
            phongShader.setInt("uUseTexture", 0); // Iskljuci teksturu (bojimo rucno)

            // Crvena boja
            phongShader.setVec3("uMaterial.kD", 1.0f, 0.0f, 0.0f);
            phongShader.setVec3("uMaterial.kS", 1.0f, 1.0f, 1.0f);
            phongShader.setFloat("uMaterial.shine", 64.0f);

            for (unsigned int i = 0; i < measurementPoints.size(); i++)
            {
                glm::mat4 model = glm::mat4(1.0f);

                // 1. Postavi na tacnu lokaciju klika
                model = glm::translate(model, measurementPoints[i]);

                // --- NOVO: PODIZANJE MODELA ---
                // Dodajemo ofset po Y osi da bi "igla" bola mapu, a lopta bila gore.
                // Vrednost 1.0f ili 2.0f zavisi od toga koliko je model originalno velik.
                // Probaj redom: 0.5f, 1.0f, 2.0f dok ne izgleda dobro.
                model = glm::translate(model, glm::vec3(0.0f, 0.75f, 0.0f));

                // 2. Skaliranje
                model = glm::scale(model, glm::vec3(0.2f));

                phongShader.setMat4("uM", model);
                pinModel.Draw(phongShader);
            }

            // 2. Iscrtavanje LINIJA
            if (measurementPoints.size() > 1)
            {
                // Boja: Koristimo Ambijentalnu komponentu da linija bude skroz CRVENA
                // Posto linije nemaju normale, difuzno i spekularno mora biti 0
                phongShader.setVec3("uMaterial.kA", 1.0f, 0.0f, 0.0f); // Crvena boja
                phongShader.setVec3("uMaterial.kD", 0.0f, 0.0f, 0.0f); // Iskljuci difuzno
                phongShader.setVec3("uMaterial.kS", 0.0f, 0.0f, 0.0f); // Iskljuci odsjaj

                glm::mat4 model = glm::mat4(1.0f);

                // KLJUČNO: Podižemo linije malo iznad poda (0.05 po Y osi)
                // Ovo rešava problem "isprekidanih" linija (Z-fighting)
                model = glm::translate(model, glm::vec3(0.0f, 0.05f, 0.0f));

                phongShader.setMat4("uM", model);

                // Ažuriramo podatke
                glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
                glBufferData(GL_ARRAY_BUFFER, measurementPoints.size() * sizeof(glm::vec3), &measurementPoints[0], GL_DYNAMIC_DRAW);

                glBindVertexArray(lineVAO);

                // Povećavamo debljinu linije
                glLineWidth(5.0f); // Probaj vrednost 3.0f ili 5.0f

                glDrawArrays(GL_LINE_STRIP, 0, measurementPoints.size());

                // Vraćamo debljinu na normalu da ne utiče na ostale objekte (ako crtamo wireframe)
                glLineWidth(1.0f);
            }
        }


        // ===========================================
        // 2. FAZA: 2D UI (Tekst i ikone)
        // ===========================================
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(uiShader.ID);

        glm::mat4 uiProjection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
        glUniformMatrix4fv(glGetUniformLocation(uiShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(uiProjection));

        // Prikaz teksta zavisno od rezima
        std::stringstream ss;
        if (isWalkingMode) {
            ss << "Ukupna predjena distanca: " << std::fixed << totalDistance << "m";
            // Ispis moda
            RenderText(uiShader.ID, "Rezim: SETANJE", 25.0f, 25.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        else {
            ss << "Izmereno: " << std::fixed  << measurementTotalLen << "m";
            // Ispis moda
            RenderText(uiShader.ID, "Rezim: MERENJE", 25.0f, 25.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        // Glavni ispis (Gore Levo) - Zuta boja
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
            cameraFront = glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f));
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

void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Reagujemo na klik samo ako NIJE walking mode
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !isWalkingMode)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // ---------------------------------------------------------
        // KORAK 1: REKONSTRUKCIJA MATRICA (Moraju biti ISTE kao u main-u)
        // ---------------------------------------------------------

        // A) Projekcija: 45 stepeni FOV, Aspect ratio prozora
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

        // B) View: Kamera pozicija i smer
        // cameraFront je globalna promenljiva koja se azurira u processInput. 
        // Ovim osiguravamo da klik "gleda" u istom pravcu kao i kamera.
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, glm::vec3(0.0f, 1.0f, 0.0f));

        // ---------------------------------------------------------
        // KORAK 2: PRETVARANJE 2D KLIKA U 3D ZRAK (Ray)
        // ---------------------------------------------------------

        // Viewport definise velicinu ekrana (x, y, sirina, visina)
        glm::vec4 viewport = glm::vec4(0, 0, width, height);

        // OpenGL krece od dole-levo, a Windows od gore-levo. Zato (height - ypos).
        // Trazimo dve tacke u 3D prostoru koje se poklapaju sa tim pikselom:
        // winPosNear -> Tacka na samom ekranu (Z=0.0)
        // winPosFar  -> Tacka u beskonacnosti (Z=1.0)

        glm::vec3 winPosNear = glm::vec3(xpos, height - ypos, 0.0f);
        glm::vec3 winPosFar = glm::vec3(xpos, height - ypos, 1.0f);

        // glm::unProject radi "magiju" - vraca 3D koordinate
        glm::vec3 rayStart = glm::unProject(winPosNear, view, projection, viewport);
        glm::vec3 rayEnd = glm::unProject(winPosFar, view, projection, viewport);

        // Smer zraka je vektor od pocetka ka kraju
        glm::vec3 rayDir = glm::normalize(rayEnd - rayStart);

        // ---------------------------------------------------------
        // KORAK 3: GDE ZRAK UDARA U POD (Y=0)?
        // ---------------------------------------------------------

        // Jednacina zraka: P = Start + Dir * t
        // Nama treba P.y = 0 (Visina poda)
        // 0 = Start.y + Dir.y * t
        // -Start.y = Dir.y * t
        // t = -Start.y / Dir.y

        if (rayDir.y != 0.0f) // Zastita od deljenja sa nulom (ako gledamo skroz horizontalno)
        {
            float t = -rayStart.y / rayDir.y;

            // t > 0 znaci da je presek ispred kamere (a ne iza ledja)
            if (t > 0.0f)
            {
                // Konacna tacka preseka
                glm::vec3 hitPoint = rayStart + rayDir * t;

                // ---------------------------------------------------------
                // KORAK 4: LOGIKA IGRE (Dodaj/Brisi ciodu)
                // ---------------------------------------------------------

                // Provera granica mape (-10 do 10)
                if (hitPoint.x >= -10.0f && hitPoint.x <= 10.0f && hitPoint.z >= -10.0f && hitPoint.z <= 10.0f)
                {
                    bool deleted = false;
                    // Provera za brisanje (da li smo kliknuli blizu postojece)
                    for (int i = 0; i < measurementPoints.size(); i++) {
                        if (glm::distance(measurementPoints[i], hitPoint) < 0.5f) { // Radijus 0.5
                            measurementPoints.erase(measurementPoints.begin() + i);
                            deleted = true;
                            break;
                        }
                    }

                    // Ako nismo obrisali, dodaj novu
                    if (!deleted) {
                        measurementPoints.push_back(hitPoint);
                    }

                    // Ponovo izracunaj ukupnu duzinu
                    measurementTotalLen = 0.0f;
                    if (measurementPoints.size() > 1) {
                        for (int i = 0; i < measurementPoints.size() - 1; i++) {
                            measurementTotalLen += glm::distance(measurementPoints[i], measurementPoints[i + 1]);
                        }
                    }
                }
            }
        }
    }
}