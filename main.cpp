#include "shaders/player.h"
#include "shaders/map.h"
#include "shaders/pellets.h"
#include "shaders/ghosts.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <set>
#include <cmath>
#include <vector>
#include <stb_image.h>

// -----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// -----------------------------------------------------------------------------
GLuint CompileShader(const std::string& vertexShader,
    const std::string& fragmentShader);

GLuint CreateMap(std::vector<GLfloat>* map, GLfloat* mapObj);
GLuint CreateObject(GLfloat *object, int size);

GLuint getIndices(int out, int mid, int in);

void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect, std::vector<float>* map);
GLuint load_opengl_texture(const std::string& filepath, GLuint slot);

void TransformMap(const GLuint);
void TransformPlayer(const GLuint, float lerpProg, float lerpStart[], float lerpStop[]);

GLfloat getCoordsWithInt(int y, int x, int type);


std::vector<std::vector<int>> loadFromFile();

//void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader);

void CleanVAO(GLuint& vao);

void GLAPIENTRY MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam);


// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

const int mapSquareNumber = 708;                    // I really wanted to avoid doing this, but due to how you initialize arrays is wierd

std::vector<float> coords;

int width = 24, height = 24;

const int spriteSize = 64, resize = 3;              //Size refrencing for map

int wallMap[spriteSize * 2][spriteSize * 2];        //FIX THIS SHIT

GLfloat player[4 * 3];

GLuint map_indices[mapSquareNumber * 6];

float pi = glm::pi<float>();

float speedDiv = 25.0f;
float Xshift, Yshift;

int playerXY[2];
bool permittPelletUpdate = false;
bool run = true;

enum type {pacman = 0, ghost = 1, pellet = 2};

// -----------------------------------------------------------------------------
// Classes
// -----------------------------------------------------------------------------

class Character {
private:
    float lerpPos[2], lerpStart[2], lerpStop[2];
    float lerpStep = 1.0f/speedDiv, lerpProg = lerpStep;
    int dir = 1, prevDir = dir, XYpos[2];
    GLfloat vertices[4 * 5] = { 0 };
    //AI values
    bool AI = false;
    int AIdelay = dir;
public:
    Character() {};
    Character(int x, int y);
    Character(int x, int y, bool ai);
    ~Character() {};
    //Initialization functions
    void characterInit();
    void convertToVert();
    auto initVao();
    bool getLegalDir(int dir);
    void getLerpCoords();
    void changeDir();
    void Transform(const GLuint ShaderProgram);
    void updateLerp();
    void updateDir(int outDir);
    GLfloat getVertCoord(int index);
    void checkPellet();
    bool checkGhostCollision();
    //AI functions
    int getRandomAIdir();
    void AIupdateVertice();
    float AIgetLerpPog();
    int AIgetXY(int xy);
};

class Pellet {
private:
    int XYpos[2];
    bool enabled = true;
    GLfloat vertices[4 * 3] = { 0 };
public:
    Pellet() {};
    Pellet(int x, int y);
    ~Pellet() {};
    void initCoords();
    GLfloat getVertCoord(int index);
    void removePellet();
    int checkCoords(int XY);
};

//Inital definition
std::vector<Character*> Pacman;     //Type 0
std::vector<Character*> Ghosts;     //Type 1
std::vector<Pellet*>   Pellets;     //Type 2

GLuint compileVertices(std::vector<Pellet*> itObj);
GLuint compileVertices(std::vector<Character*> itObj);


// -----------------------------------------------------------------------------
// Class function definition
// -----------------------------------------------------------------------------

Character::Character(int x, int y) {
    dir = 9, prevDir = 9;
    XYpos[0] = x, XYpos[1] = y;
    characterInit();
};

Character::Character(int x, int y, bool ai) {
    XYpos[0] = x, XYpos[1] = y;
    AI = ai;
    dir = getRandomAIdir();
    AIdelay = dir;
    characterInit();
};

void Character::characterInit() {
    convertToVert();
    lerpStart[0] = (vertices[0]);
    lerpStart[1] = (vertices[1]);
    if      (dir == 2) {
        lerpStop[0] = (vertices[5]);
        lerpStop[1] = (vertices[6]);
    }
    else if (dir == 4) {
        lerpStop[0] = lerpStart[0];
        lerpStop[1] = lerpStart[1];
        lerpStop[1] -= Yshift;
    }
    else if (dir == 3) {
        lerpStop[0] = lerpStart[0];
        lerpStop[1] = lerpStart[1];
        lerpStop[0] -= Xshift;
    }
    else if (dir == 9) {
        lerpStop[0] = (vertices[15]);
        lerpStop[1] = (vertices[16]);
    }
    vertices[4] = 1.0f;  vertices[13] = 1.0f;
    vertices[18] = 1.0f; vertices[19] = 1.0f;
}

void Character::convertToVert() {
    int loop = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 3; x++) {
            vertices[loop] = (getCoordsWithInt(XYpos[1], XYpos[0], loop));
            loop++;
        }
    }
}

auto Character::initVao() {
    return CreateObject(vertices, sizeof(vertices));
}

bool Character::getLegalDir(int dir) {
    int testPos[2] = { XYpos[0], XYpos[1] };
    switch (dir) {
    case 2: testPos[1] += 1; break;      //UP test
    case 4: testPos[1] -= 1; break;      //DOWN test
    case 3: testPos[0] -= 1; break;      //LEFT test
    case 9: testPos[0] += 1; break;      //RIGHT test
    }
    if ((testPos[0] < width && testPos[1] < height) && (0 <= testPos[0] && 0 <= testPos[1])) {
        if (wallMap[testPos[1]][testPos[0]] != 1) { return true; }
        else {return false; }
    }
    else { return false; }    //incase moving outside map illegal untill further notice
    return false;
};

void Character::getLerpCoords() {
    switch (dir) {
    case 2: XYpos[1] += 1; break;     //UP
    case 4: XYpos[1] -= 1; break;     //DOWN
    case 3: XYpos[0] -= 1; break;     //LEFT
    case 9: XYpos[0] += 1; break;     //RIGHT
    }

    lerpStop[0] = ( XYpos[0] * Xshift);
    lerpStop[1] = ((XYpos[1] * Yshift)-1);
    if (AI) lerpStop[0] -= 1;
};

void Character::changeDir() {
    bool legal = true;
    if (AI && AIdelay == 0) { dir = getRandomAIdir(); AIdelay = ((rand()+4)%10); }
    else {
        AIdelay--;
        legal = getLegalDir(dir);
    }

    if (legal && (dir % prevDir == 0) && dir != prevDir && !AI) {
        float coordHolder[2];
        coordHolder[0] = lerpStop[0];      coordHolder[1] = lerpStop[1];
        lerpStop[0]    = lerpStart[0];     lerpStop[1]    = lerpStart[1];
        lerpStart[0]   = coordHolder[0];   lerpStart[1]   = coordHolder[1];
        lerpProg = (1 - lerpProg);
        getLerpCoords();
        prevDir = dir;
    }
    else if (legal && (lerpProg <= 0 || lerpProg >= 1)) {
        lerpStart[0] = lerpStop[0];
        lerpStart[1] = lerpStop[1];
        getLerpCoords();
        lerpProg = lerpStep/2;
        prevDir = dir;
    }
};

void Character::Transform(const GLuint ShaderProgram) {
    TransformPlayer(ShaderProgram, lerpProg, lerpStart, lerpStop);
};

void Character::updateLerp() {
    if (lerpProg >= 1 || lerpProg <= 0) { changeDir(); }
    else { lerpProg += lerpStep; }

    if (0.5f <= lerpProg && lerpProg <= 0.6 && !AI) {
        checkPellet();
    }
    if (AI) { AIupdateVertice(); }
}

void Character::updateDir(int outDir) {
    dir = outDir;
}

GLfloat Character::getVertCoord(int index) {
    return vertices[index];
};

void Character::checkPellet() {
    for (auto& it : Pellets) {
        int check = 0;
        for (int i = 0; i < 2; i++) {
            if (XYpos[i] == it->checkCoords(i)) { check++; }
        }
        if (check == 2) { it->removePellet(); }
    }
};

bool Character::checkGhostCollision(){
    int check = 0;
    for (auto& it : Ghosts) {
        check = 0;
        for (int u = 0; u < 2; u++) {
            if (it->AIgetXY(u) == XYpos[u]) {  check++; }
        }
        if (check == 2) {
            if (AIgetLerpPog() <= (lerpProg + lerpStep) && (lerpProg - lerpStep) <= AIgetLerpPog()) { return true; }
        }
    }
    return false;
}

int Character::getRandomAIdir() {
    int temp = 0;
    do {
        temp =  (rand() % 4);
        switch (temp)
        {
        case 0: temp = 2;    break;
        case 1: temp = 4;    break;
        case 2: temp = 3;    break;
        case 3: temp = 9;    break;
        }
    } while (!getLegalDir(temp));
    return temp;
}

void Character::AIupdateVertice() {
    for (int f = 0; f < (3*5); f+=5) {
        for (int k = f; k < (f+3); k++) {
            if (k == f) {
                vertices[k] = (((1 - lerpProg) * lerpStart[0]) + (lerpProg * lerpStop[0]));
            }
            else if (k == (f+1)) {
                vertices[k] = (((1 - lerpProg) * lerpStart[1]) + (lerpProg * lerpStop[1]));
            }

            switch (k) {
            case 0:   vertices[k];            break;
            case 1:   vertices[k];            break;

            case 5:   vertices[k];            break;
            case 6:   vertices[k] += Yshift;  break;

            case 10:  vertices[k] += Xshift; break;
            case 11:  vertices[k] += Yshift; break;

            case 15:  vertices[k] += Xshift; break;
            case 16:  vertices[k];            break;
            default:  vertices[k] = 0.0f;     break;
            }
        }
    }
}


float Character::AIgetLerpPog() {
    return lerpProg;
}

int Character::AIgetXY(int xy) {
    return XYpos[xy];
}

Pellet::Pellet(int x, int y) {
    XYpos[0] = x; XYpos[1] = y;
    initCoords();
};

void Pellet::initCoords() {
    int loop = 0, coordCall = 0;
    float Xquart = Xshift / 3.0f;
    float Yquart = Yshift / 3.0f;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 3; x++) {
            vertices[loop] = (getCoordsWithInt(XYpos[1], XYpos[0], coordCall));
            switch (loop) {
            case 0:  vertices[loop] += Xquart; break;
            case 1:  vertices[loop] += Yquart; break;

            case 5:  vertices[loop] += Xquart; break;
            case 6:  vertices[loop] -= Yquart; break;

            case 10: vertices[loop] -= Xquart; break;
            case 11: vertices[loop] -= Yquart; break;

            case 15: vertices[loop] -= Xquart; break;
            case 16: vertices[loop] += Yquart; break;
            default: vertices[loop] = 0.0f;    break;
            }
            loop++; coordCall++;
        }
        loop += 2;
    }
}

void Pellet::removePellet() {
    if (enabled) {
        for (int i = 0; i < 12; i++) {
            vertices[i] = 0.0f;
        }
        enabled = false;
        permittPelletUpdate = true;
    }
}
GLfloat Pellet::getVertCoord(int index) {
    return vertices[index];
}

int Pellet::checkCoords(int XY) {
    if (enabled) return XYpos[XY];
    return -1;
}

// -----------------------------------------------------------------------------
// Key Callsback
// -----------------------------------------------------------------------------
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS){
        switch (key) {
        case GLFW_KEY_UP:           if (Pacman[0]->getLegalDir(2)) { Pacman[0]->updateDir(2); Pacman[0]->changeDir(); };  break;
        case GLFW_KEY_DOWN:         if (Pacman[0]->getLegalDir(4)) { Pacman[0]->updateDir(4); Pacman[0]->changeDir(); };  break;
        case GLFW_KEY_LEFT:         if (Pacman[0]->getLegalDir(3)) { Pacman[0]->updateDir(3); Pacman[0]->changeDir(); };  break;
        case GLFW_KEY_RIGHT:        if (Pacman[0]->getLegalDir(9)) { Pacman[0]->updateDir(9); Pacman[0]->changeDir(); };  break;
        }
    }
}

// -----------------------------------------------------------------------------
// ENTRY POINT
// -----------------------------------------------------------------------------
int main()
{
    int collected = 0;
    std::vector<GLfloat> map;

    // Creates coordinates for map
    callMapCoordinateCreation(loadFromFile(), &map);

    // Initialization of GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed." << '\n';
        std::cin.get();

        return EXIT_FAILURE;
    }

    // Setting window hints
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    auto window = glfwCreateWindow(width * spriteSize / resize, height * spriteSize / resize, "Pacman", nullptr, nullptr);

    glfwSetKeyCallback(window, key_callback);

    if (window == nullptr)
    {
        std::cerr << "GLFW failed on window creation." << '\n';
        std::cin.get();

        glfwTerminate();

        return EXIT_FAILURE;
    }

    // Setting the OpenGL context.
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Eanable capture of debug output.
    //glEnable(GL_DEBUG_OUTPUT);
    //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //glDebugMessageCallback(MessageCallback, 0);
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    auto playerVAO           = compileVertices(Pacman);
    auto playerShaderProgram = CompileShader(   playerVertexShaderSrc,
                                                playerFragmentShaderSrc);


    GLuint texAttrib = glGetAttribLocation(playerShaderProgram, "aTexcoord");
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(texAttrib);

    auto pelletVAO           = compileVertices(Pellets);
    auto pelletShaderProgram = CompileShader(   pelletVertexShaderSrc,
                                                pelletFragmentShaderSrc);

    auto mapVAO =           CreateMap(&map,(&map[0]));
    auto mapShaderProgram = CompileShader(      mapVertexShaderSrc,
                                                mapFragmentShaderSrc);

    auto ghostVAO = compileVertices(Ghosts);
    auto ghostShaderProgram = CompileShader(    ghostVertexShaderSrc,
                                                ghostFragmentShaderSrc);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_MULTISAMPLE);


    double currentTime = 0.0;
    glfwSetTime(0.0);
    float frequency = 0.01f;

    //Texture loading
    auto pacmanTexture = load_opengl_texture("assets/pacman.png", 2);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Time management
        int Xwidth, Yheight;
        glfwGetFramebufferSize(window, &Xwidth, &Yheight);

        glViewport(0,0, Xwidth, Yheight);

        currentTime = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);
        int numElements = (6 * (width * height) - Pellets.size() - 1);
        // Draw MAP
        auto mapVertexColorLocation = glGetUniformLocation(mapShaderProgram, "u_Color");
        glUseProgram(mapShaderProgram);
        glBindVertexArray(mapVAO);
        glUniform4f(mapVertexColorLocation, 0.1f, 0.0f, 0.6f, 1.0f);
        glDrawElements(GL_TRIANGLES, numElements, GL_UNSIGNED_INT, (const void*)0);


        auto pelletVertexColorLocation = glGetUniformLocation(pelletShaderProgram, "u_Color");
        glUseProgram(pelletShaderProgram);
        glBindVertexArray(pelletVAO);
        glUniform4f(pelletVertexColorLocation, 0.8f, 0.8f, 0.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6 * Pellets.size(), GL_UNSIGNED_INT, (const void*)0);

        if (permittPelletUpdate) {
            CleanVAO(pelletVAO);
            pelletVAO = compileVertices(Pellets);
            permittPelletUpdate = false;
            collected++;
        }
        auto playerTextureLocation      = glGetUniformLocation(playerShaderProgram, "u_PlayerTexture");
        auto playerVertexColorLocation  = glGetUniformLocation(playerShaderProgram, "u_Color");
        glUseProgram(playerShaderProgram);
        glBindVertexArray(playerVAO);
        glUniform4f(playerVertexColorLocation, 1.0f, 1.0f, 0.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

        Pacman[0]->Transform(playerShaderProgram);
        glUniform1i(playerTextureLocation, 2);

        CleanVAO(ghostVAO);
        ghostVAO = compileVertices(Ghosts);

        auto ghostVertexColorLocation = glGetUniformLocation(ghostShaderProgram, "u_Color");
        glUseProgram(ghostShaderProgram);
        glBindVertexArray(ghostVAO);

        glUniform4f(ghostVertexColorLocation, 0.7f, 0.0f, 0.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

        glUniform4f(ghostVertexColorLocation, 0.0f, 0.7f, 0.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)24);

        glUniform4f(ghostVertexColorLocation, 0.7f, 0.0f, 0.7f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)48);

        if (collected == Pellets.size()) {
            run = false;
        }


        //Update all Lerps
        if (currentTime > frequency && run) {
            glfwSetTime(0.0);
            Pacman[0]->updateLerp();
            for (auto& ghostIt : Ghosts) {
                ghostIt->updateLerp();
            }
            if (Pacman[0]->checkGhostCollision()) { run = false; }
        }
        glfwSwapBuffers(window);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            break;
        }

    }

    glUseProgram(0);
    glDeleteProgram(playerShaderProgram);
    glDeleteProgram(mapShaderProgram);
    glDeleteProgram(pelletShaderProgram);
    glDeleteProgram(ghostShaderProgram);

    glDeleteTextures(1, &pacmanTexture);


    CleanVAO(playerVAO);
    CleanVAO(mapVAO);
    CleanVAO(pelletVAO);
    CleanVAO(ghostVAO);

    delete (&Pellets);
    delete (&Ghosts);
    delete (&Pacman);

    glfwTerminate();

    return EXIT_SUCCESS;
}

GLfloat getCoordsWithInt(int y, int x, int loop) {
    Xshift = 2.0f / (float(width));
    Yshift = 2.0f / (float(height));
    GLfloat tempXs, tempYs;
    if (x == 0 && y == 0) { tempXs = 0, tempYs = 0; }
    else { tempXs = (Xshift * x), tempYs = (Yshift * y); }

    switch (loop) {
    case 0:   tempXs;             return (tempXs - 1.0f);  // Top Left
    case 1:   tempYs;             return (tempYs - 1.0f);  // Top Left

    case 3:   tempXs;             return (tempXs - 1.0f);  // Bot Left
    case 4:   tempYs += Yshift;   return (tempYs - 1.0f);  // Bot Left

    case 6:   tempXs += Xshift;   return (tempXs - 1.0f);  // Bot Right
    case 7:   tempYs += Yshift;   return (tempYs - 1.0f);  // Bot Right

    case 9:   tempXs += Xshift;   return (tempXs - 1.0f);  // Top Right
    case 10:  tempYs;             return (tempYs - 1.0f);  // Top Right
    default: return 0.0f;
    }
};


void TransformPlayer(const GLuint shaderprogram, float lerpProg, float lerpStart[], float lerpStop[])
{
    //Presentation below purely for ease of viewing individual components of calculation, and not at all necessary.

    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(
        (((1 - lerpProg) * lerpStart[0]) + (lerpProg * lerpStop[0])),
        (((1 - lerpProg) * lerpStart[1]) + (lerpProg * lerpStop[1])),
        0.f));


    GLuint transformationmat = glGetUniformLocation(shaderprogram, "u_TransformationMat");

    glUniformMatrix4fv(transformationmat, 1, false, glm::value_ptr(translation));

}

// -----------------------------------------------------------------------------
// COMPILE SHADER
// -----------------------------------------------------------------------------
GLuint CompileShader(const std::string& vertexShaderSrc,
    const std::string& fragmentShaderSrc)
{

    auto vertexSrc = vertexShaderSrc.c_str();
    auto fragmentSrc = fragmentShaderSrc.c_str();

    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShader);

    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShader);

    auto shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint load_opengl_texture(const std::string& filepath, GLuint slot)
{
    /**
     *  - Use the STB Image library to load a texture in here
     *  - Initialize the texture into an OpenGL texture
     *    - This means creating a texture with glGenTextures or glCreateTextures (4.5)
     *    - And transferring the loaded texture data into this texture
     *    - And setting the texture format
     *  - Finally return the valid texture
     */

     /** Image width, height, bit depth */
    int w, h, bpp;
    auto pixels = stbi_load(filepath.c_str(), &w, &h, &bpp, 0);

    /*Generate a texture objectand upload the loaded image to it.*/
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0 + slot);//Texture Unit
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    //GL_GENERATE_MIPMAP(GL_TEXTURE_2D);

    /** Set parameters for the texture */
    //Wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /** Very important to free the memory returned by STBI, otherwise we leak */
    if (pixels)
        stbi_image_free(pixels);

    return tex;
}

// -----------------------------------------------------------------------------
//  INITIALIZE OBJECT
// -----------------------------------------------------------------------------
GLuint CreateObject(GLfloat *object, int size)
{
    std::vector<GLuint> object_indices;

    int trt = 0;
    for (int i = 0; i < size; i += 4) {
        for (int o = 0; o < 2; o++) {
            for (int p = i; p < (i + 3); p++) {
                object_indices.push_back(getIndices(i, o, p));
            }
        }
    };

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        size,
        (&object[0]),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (const void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, object_indices.size()*sizeof(object_indices)[0], (&object_indices[0]), GL_STATIC_DRAW);

    return vao;
}


// -----------------------------------------------------------------------------
//  CREATE MAP
// -----------------------------------------------------------------------------
GLuint CreateMap(std::vector<GLfloat> * map, GLfloat *mapObj) {
    int counter = 0;
    for (int i = 0; i < ((*map).size()/3); i += 4) {
        for (int o = 0; o < 2; o++) {
            for (int p = i; p < (i + 3); p++) {

                map_indices[counter] = getIndices(i, o, p);
                counter++;
            }
        }
    };

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        ((*map).size()*sizeof((*map)[0])),
        (&mapObj)[0],
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (const void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(map_indices), map_indices, GL_STATIC_DRAW);

    return vao;
}

// -----------------------------------------------------------------------------
// Small Functions
// -----------------------------------------------------------------------------

GLuint getIndices(int out, int mid, int in) {
    if (in == out) { return out; }
    else { return (mid + in); };
}

std::vector<std::vector<int>> loadFromFile() {
    // Read from level file;
    std::ifstream inn("../../../../levels/level0");
    if (inn) {
        inn >> width; inn.ignore(1); inn >> height;
        std::vector<std::vector<int>> tempMapVect(height, std::vector<int>(width, 0));
        int row = 0, column = 0;
        int temp;
        inn >> temp;
        while (column < height) { // adds "walls" int vector
            int Yvalue = (height - 1 - column);
            if (row < width) {
                tempMapVect[Yvalue][row] = temp;
                wallMap[(Yvalue)][row] = temp;
                row++;
                inn >> temp;
            }
            else { row = 0; column++;}
        }
        inn.close();
        return tempMapVect;
    }
    else { printf("\n\nERROR: Couldnt find level file, check that it is in the right place.\n\n"); exit(EXIT_FAILURE); }
}

GLuint compileVertices(std::vector<Pellet*> itObj) {
    std::vector<GLfloat> veticieList;

    for (auto & it : Pellets) {
        for (int i = 0; i < 12; i++) {
            veticieList.push_back(it->getVertCoord(i));
        }
    }
    return CreateObject(&veticieList[0], veticieList.size()*sizeof(veticieList[0]));
}

GLuint compileVertices(std::vector<Character*> itObj) {
    std::vector<GLfloat> veticieList;

    for (auto& it : itObj) {
        for (int i = 0; i < 12; i++) {
            veticieList.push_back(it->getVertCoord(i));
        }
    }
    return CreateObject(&veticieList[0], veticieList.size() * sizeof(veticieList[0]));
}


void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect, std::vector<float>* map) {
    int formerPos[3] = {0,0,0}, hallCount = 0;
    for (int i = 0; i < height; i++) {    // creates map
        for (int j = 0; j < width; j++) {
            if (levelVect[i][j] == 1) {
                int loop = 0;
                for (int inner = 0; inner < 4; inner++) {
                    for (int outer = 0; outer < 3; outer++) {
                        float temp = getCoordsWithInt(i, j, loop);
                        (*map).push_back(temp);
                        loop++;
                    }
                }
            }
            else if (levelVect[i][j] == 2) {
                   Pacman.push_back(new Character(j, i));
            }
            else { hallCount++; Pellets.push_back(new Pellet(j, i)); }
        }
    }
    do {
        for (int g = 0; g < 3; g++) {
            int randPos;
            randPos = (rand() % hallCount);
            formerPos[g] = randPos;
        }
    } while (formerPos[0] == formerPos[1] || formerPos[0] == formerPos[2] || formerPos[1] == formerPos[2]);
    int count = 0;
    for (auto& it : Pellets) {
        if (count == formerPos[0] || count == formerPos[1] || count == formerPos[2]) {
            Ghosts.push_back(new Character(it->checkCoords(0), it->checkCoords(1), true));
        }
        count++;
    }
}

// -----------------------------------------------------------------------------
// Clean VAO
// -----------------------------------------------------------------------------
void CleanVAO(GLuint& vao)
{
    GLint nAttr = 0;
    std::set<GLuint> vbos;

    GLint eboId;
    glGetVertexArrayiv(vao, GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboId);
    glDeleteBuffers(1, (GLuint*)&eboId);

    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nAttr);
    glBindVertexArray(vao);

    for (int iAttr = 0; iAttr < nAttr; ++iAttr)
    {
        GLint vboId = 0;
        glGetVertexAttribiv(iAttr, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vboId);
        if (vboId > 0)
        {
            vbos.insert(vboId);
        }

        glDisableVertexAttribArray(iAttr);
    }

    for (auto vbo : vbos)
    {
        glDeleteBuffers(1, &vbo);
    }

    glDeleteVertexArrays(1, &vao);
}


// -----------------------------------------------------------------------------
// MessageCallback (for debugging purposes)
// -----------------------------------------------------------------------------
void GLAPIENTRY
MessageCallback(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    std::cerr << "GL CALLBACK:" << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") <<
        "type = 0x" << type <<
        ", severity = 0x" << severity <<
        ", message =" << message << "\n";
}
