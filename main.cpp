#include "shaders/player.h"
#include "shaders/map.h"
#include "shaders/pellets.h"

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

// -----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// -----------------------------------------------------------------------------
GLuint CompileShader(const std::string& vertexShader,
    const std::string& fragmentShader);

GLuint CreateMap();
GLuint CreateObject(GLfloat *object, int size);

GLuint getIndices(int out, int mid, int in);

void fillMap();
void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect);

void TransformMap(const GLuint);
void TransformPlayer(const GLuint, float lerpProg, float lerpStart[], float lerpStop[]);
void compileVertices();

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
const int mapIndiceNumber = mapSquareNumber * 4;    // this was the best solution for me.
const int maxMapCoordNumber = mapSquareNumber * 12;

std::vector<float> coords;

int width = 24, height = 24, walls = 0, objects = 1;

const int spriteSize = 64, resize = 3;

int wallMap[spriteSize * 2][spriteSize * 2];        //A soft max for map sizes

int playerDir = 9, prevDir = 1; //set to 1 to avoid intDiv by 0

GLfloat map[maxMapCoordNumber];

GLfloat player[4 * 3];

GLuint map_indices[708 * 6];

float pi = glm::pi<float>();

float speedDiv = 50.0f;

float lerpStart[2], lerpStop[2], playerPos[2];
float Xshift, Yshift;

int playerXY[2];

// -----------------------------------------------------------------------------
// Classes
// -----------------------------------------------------------------------------

class Character {
private:
    float lerpPos[2] = { 0 }, lerpStart[2], lerpsStop[2];
    float lerpStep = 1.0f/speedDiv, lerpProg = lerpStep;
    int dir = 9, prevDir = dir, XYpos[2] = { 0 }, ID = 0;
    GLfloat vertices[4 * 3] = { 0 };
    bool AI = false;
public:
    Character() { printf("\nPACMAN OBJECT CREATED\n"); };
    void initAI();
    Character(int x, int y);
    void characterInit();
    void convertToVert();
    auto initVao();
    bool getLegalDir(int dir);
    void getLerpCoords();
    void changeDir();
    void Transform(const GLuint ShaderProgram);
    void testFunk();
    void updateLerp();
    void updateDir(int outDir);
};

class Pellet {
private:
    int XYpos[2], ID;
    bool enabled = true;
    GLfloat vertices[4 * 3] = { 0 };
public:
    Pellet() {};
    Pellet(int x, int y);
    void initCoords();
};

//Inital definition
std::vector<Character*> Pacman;
std::vector<Character*> Ghosts;
std::vector<Pellet*>   Pellets;


// -----------------------------------------------------------------------------
// Class function definition
// -----------------------------------------------------------------------------

void Character::initAI() {
    AI = true;
    ID = objects; objects++;
    characterInit();
    printf("\n\nWrong one\n\n");
};

Character::Character(int x, int y) {
    XYpos[0] = x, XYpos[1] = y;
    printf("\n\nRight one Xpos, %i, Ypos, %i\n\n", XYpos[0], XYpos[1]);
    characterInit();
};

void Character::characterInit() {
    convertToVert();
    GLfloat temp = getCoordsWithInt(XYpos[1], XYpos[0], 9);

    lerpStart[0] = (temp - Xshift); lerpsStop[0] = temp;
    printf("\nlerpStartx: %f\tlerpStopx: %f", lerpStart[0], temp);
    lerpStart[1] = (vertices[10]);     lerpsStop[1] = lerpStart[1];
    printf("\nlerpStartY: %f\tlerpStopY: %f\n", lerpStart[1], lerpsStop[1]);
};

void Character::convertToVert() {
    int loop = 0;
    for (int y = 0; y < 4; y++) {
        printf("\n");
        for (int x = 0; x < 3; x++) {
            vertices[loop] = (getCoordsWithInt(XYpos[1], XYpos[0], loop));
            printf("%f ", vertices[loop]);
            loop++;
        }
    }
};

auto Character::initVao() {
    return CreateObject(vertices, (sizeof(vertices)/sizeof(vertices[0])));
};

void Character::testFunk() {
    printf("TEST: Xpos: %i\tYpos: %i\n", XYpos[0], XYpos[1]);
};

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
    case 2: XYpos[1] += 1;  printf("\nMOVED\n"); break;     //UP
    case 4: XYpos[1] -= 1;  printf("\nMOVED\n"); break;     //DOWN
    case 3: XYpos[0] -= 1;  printf("\nMOVED\n"); break;     //LEFT
    case 9: XYpos[0] += 1;  printf("\nMOVED\n"); break;     //RIGHT
    }

    lerpStop[0] = ( XYpos[0] * Xshift);
    lerpStop[1] = ((XYpos[1] * Yshift) -1);
};

void Character::changeDir() {
    //printf("\nCHANGEDIR %f", lerpProg);
    bool legal = getLegalDir(dir);
    if (legal && (dir % prevDir == 0) && dir != prevDir) {
        float coordHolder[2];
        coordHolder[0] = lerpStop[0];      coordHolder[1] = lerpStop[1];
        lerpStop[0]    = lerpStart[0];     lerpStop[1]    = lerpStart[1];
        lerpStart[0]   = coordHolder[0];   lerpStart[1]   = coordHolder[1];
        printf("\nCHANGED DIRECTION: %f\n", lerpProg);
        lerpProg = (1 - lerpProg);
        getLerpCoords();
        prevDir = dir;
    }
    else if (legal && (lerpProg <= 0 || lerpProg >= 1)) {
        lerpStart[0] = lerpStop[0];
        lerpStart[1] = lerpStop[1];
        getLerpCoords();
        lerpProg = lerpStep / 2;
        prevDir = dir;
    }
};

void Character::Transform(const GLuint ShaderProgram) {
    TransformPlayer(ShaderProgram, lerpProg, lerpStart, lerpStop);
};

void Character::updateLerp() {
    if (lerpProg >= 1 || lerpProg <= 0) { printf("\n%f\n", lerpProg); changeDir(); }
    else { lerpProg += lerpStep; }
};

void Character::updateDir(int outDir) {
    dir = outDir;
}

Pellet::Pellet(int x, int y) {
    XYpos[0] = x; XYpos[1] = y;
    ID = objects; objects++;
};

void Pellet::initCoords() {
    int loop = 0;
    for (int y = 0; y < 4; y++) {
        printf("\n");
        for (int x = 0; x < 3; x++) {
            vertices[loop] = (getCoordsWithInt(XYpos[1], XYpos[0], loop));
            printf("%f ", vertices[loop]);
            loop++;
        }
    }
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
    // Read from level file;
    std::vector<std::vector<int>>levelVect = loadFromFile();

    // Creates coordinates for map
    callMapCoordinateCreation(levelVect);

    // Initialization of GLFW
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed." << '\n';
        std::cin.get();

        return EXIT_FAILURE;
    }

    // Setting window hints
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    auto playerVAO           = Pacman[0]->initVao();
    auto playerShaderProgram = CompileShader(   playerVertexShaderSrc,
                                                playerFragmentShaderSrc);

    auto pelletVAO           = compileVertices();//send pellets vecotr
    auto pelletShaderProgram = CompileShader(   pelletVertexShaderSrc,
                                                pelletFragmentShaderSrc);


    auto mapVAO = CreateMap();
    auto mapShaderProgram = CompileShader(  mapVertexShaderSrc,
                                            mapFragmentShaderSrc);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    double currentTime = 0.0;
    glfwSetTime(0.0);
    float frequency = 0.01f;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // Time management
        currentTime = glfwGetTime();

        glClear(GL_COLOR_BUFFER_BIT);

        // Draw MAP
        auto mapVertexColorLocation = glGetUniformLocation(mapShaderProgram, "u_Color");
        glUseProgram(mapShaderProgram);
        glBindVertexArray(mapVAO);
        glUniform4f(mapVertexColorLocation, 0.1f, 0.0f, 0.6f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6 * mapSquareNumber, GL_UNSIGNED_INT, (const void*)0);

        auto pelletVertexColorLocation = glGetUniformLocation(pelletShaderProgram, "u_Color");
        glUseProgram(pelletShaderProgram);
        glBindVertexArray(pelletVAOs[0]);
        glUniform4f(pelletVertexColorLocation, 0.8f, 0.8f, 0.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

        auto playerVertexColorLocation = glGetUniformLocation(playerShaderProgram, "u_Color");
        glUseProgram(playerShaderProgram);
        glBindVertexArray(playerVAO);
        glUniform4f(playerVertexColorLocation, 1.0f, 1.0f, 0.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
        Pacman[0]->Transform(playerShaderProgram);

        //Update all Lerps
        if (currentTime > frequency) {
            glfwSetTime(0.0);
            Pacman[0]->updateLerp();
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

    CleanVAO(playerVAO);
    CleanVAO(mapVAO);
    for (auto& it : pelletVAOs) {
        CleanVAO(it);
    }

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
    //printf("\nProg: %f, StartX: %f, StartY: %f, StopX: %f, StopY: %f\n", lerpProg, lerpStart[0], lerpStart[1], lerpStop[0], lerpStop[1]);


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

// -----------------------------------------------------------------------------
//  INITIALIZE OBJECT
// -----------------------------------------------------------------------------
GLuint CreateObject(GLfloat *object, int size)
{
    printf("\nOBJECT CREATED\n");
    GLuint object_indices[6] = { 0,1,2,0,2,3 };
    GLfloat points[12];
    for (int i = 0; i < 12; i++) {
        points[i] = object[i];
    }

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    printf("\n\nSIZE OF: %i\n\n", sizeof(object));
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        12*sizeof(object),
        &object[0],
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (const void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(object_indices), object_indices, GL_STATIC_DRAW);

    return vao;
}


// -----------------------------------------------------------------------------
//  CREATE MAP
// -----------------------------------------------------------------------------
GLuint CreateMap() {
    int counter = 0;
    for (int i = 0; i < mapIndiceNumber; i += 4) {
        for (int o = 0; o < 2; o++) {
            for (int p = i; p < (i + 3); p++) {

                map_indices[counter] = getIndices(i, o, p);
                counter++;
            }
        }
    };
    fillMap();

    GLuint vao;
    glCreateVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    GLuint ebo;
    glGenBuffers(1, &ebo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(map),
        map,
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

void fillMap() {
    for (int i = 0; i < maxMapCoordNumber; i++) {
        map[i] = coords[i];
    }
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
        printf("\n");
        while (column < height) { // adds "walls" int vector
            if (row < width) {
                tempMapVect[(height - 1 - column)][row] = temp;
                wallMap[(height - 1 - column)][row] = temp;
                printf("%i ", temp);
                row++;
                inn >> temp;
            }
            else { row = 0; column++; printf("\n"); }
        }
        inn.close();
        return tempMapVect;
    }
    else { printf("\n\nERROR: Couldnt find level file, check that it is in the right place.\n\n"); exit(EXIT_FAILURE); }
}

void compileVertices(GLfloat* object) {
    return 0; // compile all vectors of object sent then call create object with size
              // maybe use for all objects?
              // should defo work for pellets and ghosts
}

void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect) {
    for (int i = 0; i < height; i++) {    // creates map
        for (int j = 0; j < width; j++) {
            if (levelVect[i][j] == 1) {
                walls++;
                int loop = 0;
                for (int inner = 0; inner < 4; inner++) {
                    for (int outer = 0; outer < 3; outer++) {
                        float temp = getCoordsWithInt(i, j, loop);
                        coords.push_back(temp);
                        loop++;
                    }
                }
            }
            else if (levelVect[i][j] == 2) {
                Pacman.push_back(new Character(j, i));
            }
            else { Pellets.push_back(new Pellet(j,i)); }
        }
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
