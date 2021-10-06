#include "shaders/player.h"
#include "shaders/map.h"

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
GLuint CreateObject(GLfloat object[12]);

GLuint getIndices(int out, int mid, int in);

void fillMap();
void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect);

void TransformMap(const GLuint);
void TransformPlayer(const GLuint, float lerpProg, float lerpStart[], float lerpStop[]);

void getLerpCoords();

void changeDir();

bool getLegalDir(const int dir);

GLfloat getCoordsWithInt(int y, int x, int type);

std::vector<std::vector<int>> loadFromFile();

//void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader);

void CleanVAO(GLuint &vao);

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

int width = 24, height = 24, walls = 0;

const int spriteSize = 64, resize = 3;

int wallMap[spriteSize*2][spriteSize*2];        //A soft max for map sizes

int playerDir = 9, prevDir = 1; //set to 1 to avoid intDiv by 0

GLfloat map[maxMapCoordNumber];

GLfloat player[4 * 3];

GLuint map_indices[708 * 6];

float pi = glm::pi<float>();

float lerpStep = 1.0f / 20.0f;
float lerpProg = lerpStep;
float lerpStart[2], lerpStop[2], playerPos[2];
float Xshift, Yshift;

int playerXY[2];

// -----------------------------------------------------------------------------
// Classes
// -----------------------------------------------------------------------------

class Character {
    private:
        float lerpPos[2] = { 0 }, lerpStart[2], lerpsStop[2];
        float lerpProg = lerpStep;
        int dir = 9, prevDir = 1, XYpos[2] = {0};
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
};

class Pellets {
    private:
        int XYpos[2];
        bool enabled = true;
        GLfloat vertices[4 * 3] = { 0 };
    public:
        Pellets() {};
        Pellets(int x, int y);
};

//Inital definition
std::vector<Character*> Pacman;
Character Ghosts[4];
Pellets pellets[28 * 34]; // set to maximum number of "slots"


// -----------------------------------------------------------------------------
// Class function definition
// -----------------------------------------------------------------------------

void Character::initAI() {
    AI = true;
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
            vertices[loop] = (getCoordsWithInt(y, x, loop));
            if (x == 1) { vertices[loop]+= 1; }
            printf("%f ", vertices[loop]);
            loop++;
        }
    }
};

auto Character::initVao() {
    return CreateObject(vertices);
};

void Character::testFunk() { printf("CORRECT, Xpos %i, Ypos: %i\n"), XYpos[0], XYpos[1]; }

bool Character::getLegalDir(int dir) {
    if (0 < lerpProg && lerpProg < 1) { return true; }

    int testPos[2] = { playerXY[0], playerXY[1] };
    switch (dir) {
    case 2: testPos[1] += 1; break;      //UP test
    case 4: testPos[1] -= 1; break;      //DOWN test
    case 3: testPos[0] -= 1; break;      //LEFT test
    case 9: testPos[0] += 1; break;      //RIGHT test
    }

    if ((testPos[0] < width && testPos[1] < height) && (0 <= testPos[0] && 0 <= testPos[1])) {
        if (wallMap[testPos[1]][testPos[0]] != 1) { return true; }
        else { return false; }
    }
    else { return false; }    //incase moving outside map illegal untill further notice
    return false;
};

void Character::getLerpCoords() {

};

void Character::changeDir() {

};

void Character::Transform(const GLuint ShaderProgram) {
    TransformPlayer(ShaderProgram, lerpProg, lerpStart, lerpStop);
};

Pellets::Pellets(int x, int y) {
    XYpos[0] = x; XYpos[1] = y;
};

// -----------------------------------------------------------------------------
// Key Callsback
// -----------------------------------------------------------------------------
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //if (prevDir != playerDir) { prevDir = playerDir; 
    switch (key) {
    case GLFW_KEY_UP:           if (Pacman[0]->getLegalDir(2)) playerDir = 2;   break;
    case GLFW_KEY_DOWN:         if (Pacman[0]->getLegalDir(4)) playerDir = 4;   break;
    case GLFW_KEY_LEFT:         if (Pacman[0]->getLegalDir(3)) playerDir = 3;   break;
    case GLFW_KEY_RIGHT:        if (Pacman[0]->getLegalDir(9)) playerDir = 9;   break;
    }
    changeDir();
    //};
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
  if(!glfwInit())
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

  auto window = glfwCreateWindow(width*spriteSize/resize, height*spriteSize/resize, "Pacman", nullptr, nullptr);

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
        
  auto playerVAO = Pacman[0]->initVao();
  Pacman[0]->testFunk();
  auto playerShaderProgram =    CompileShader(  playerVertexShaderSrc,
                                                playerFragmentShaderSrc);

  auto mapVAO =                 CreateMap();
  auto mapShaderProgram =       CompileShader(  mapVertexShaderSrc,
                                                mapFragmentShaderSrc);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  double currentTime = 0.0, prevTime = currentTime;
  glfwSetTime(0.0);

  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    // Time management
    prevTime = currentTime;
    currentTime = glfwGetTime();

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw MAP
    auto mapVertexColorLocation = glGetUniformLocation(mapShaderProgram, "u_Color");
    glUseProgram(mapShaderProgram);
    glBindVertexArray(mapVAO);
    glUniform4f(mapVertexColorLocation, 0.1f, 0.0f, 0.6f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6*mapSquareNumber, GL_UNSIGNED_INT, (const void*)0);

    auto vertexColorLocation = glGetUniformLocation(playerShaderProgram, "u_Color");
    glUseProgram(playerShaderProgram);
    glBindVertexArray(playerVAO);
    glUniform4f(vertexColorLocation, 1.0f, 1.0f, 0.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
    Pacman[0]->Transform(playerShaderProgram);
    //TransformPlayer(playerShaderProgram);

    if (lerpProg >= 1 || lerpProg <= 0) { changeDir(); }
    else if (prevTime != currentTime) { printf("\n %i", int(currentTime*100)); lerpProg += lerpStep; }

    glfwSwapBuffers(window);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      break; }

  }

  glUseProgram(0);
  glDeleteProgram(playerShaderProgram);
  glDeleteProgram(mapShaderProgram);

  CleanVAO(playerVAO);
  CleanVAO(mapVAO);

  glfwTerminate();

  return EXIT_SUCCESS;
}

GLfloat getCoordsWithInt(int y, int x, int loop) {
    Xshift = 2.0f / (float(width));
    Yshift = 2.0f / (float(height));
    GLfloat tempXs, tempYs;
    if (x == 0 && y == 0) { tempXs = 0, tempYs = 0; }
    else { tempXs = (Xshift * x), tempYs = (Yshift * y);}

    switch (loop) {
    case 0:   tempXs;             return (tempXs - 1);  // Top Left
    case 1:   tempYs;             return (tempYs - 1);  // Top Left

    case 3:   tempXs;             return (tempXs - 1);  // Bot Left
    case 4:   tempYs += Yshift;   return (tempYs - 1);  // Bot Left

    case 6:   tempXs += Xshift;   return (tempXs - 1);  // Bot Right
    case 7:   tempYs += Yshift;   return (tempYs - 1);  // Bot Right

    case 9:   tempXs += Xshift;   return (tempXs - 1);  // Top Right
    case 10:  tempYs;             return (tempYs - 1);  // Top Right
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
GLuint CreateObject(GLfloat object[12])
{

  GLuint object_indices[6] = {0,1,2,0,2,3};

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo;
  glGenBuffers(1, &vbo);

  GLuint ebo;
  glGenBuffers(1, &ebo);

  glBindBuffer( GL_ARRAY_BUFFER, vbo);
  glBufferData( GL_ARRAY_BUFFER,
                sizeof(object),
                object,
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

void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect) {
    for (int i = 0; i < height; i++) {    // creates map
        for (int j = 0; j < width; j++) {
            if (levelVect[i][j] == 1) {
                walls++;
                int loop = 0;
                for (int inner = 0; inner < 4; inner++) {
                    for (int outer = 0; outer < 3; outer++) {
                        float temp = getCoordsWithInt(i,j,loop);
                        coords.push_back(temp);
                        loop++;
                    }
                }
            }
            else if (levelVect[i][j] == 2) {
                Pacman.push_back(new Character(j, i));
                Pacman[0]->testFunk();
            }
        }
    }
}

bool getLegalDir(int dir) {
    if (0 < lerpProg && lerpProg < 1) { return true; }

    int testPos[2] = {playerXY[0], playerXY[1]};
    switch (dir) {
        case 2: testPos[1] += 1; break;      //UP test
        case 4: testPos[1] -= 1; break;      //DOWN test
        case 3: testPos[0] -= 1; break;      //LEFT test
        case 9: testPos[0] += 1; break;      //RIGHT test
    }

    if ((testPos[0] < width && testPos[1] < height) && (0 <= testPos[0] && 0 <= testPos[1])) {
        if (wallMap[testPos[1]][testPos[0]] != 1) { return true;  }
                                              else{ return false; }
    } else { return false; }    //incase moving outside map illegal untill further notice
    return false;
}

void getLerpCoords() {
    switch (playerDir) {
        case 2: playerXY[1] += 1; printf("\nMOVE UP\n"); break;        //UP
        case 4: playerXY[1] -= 1; printf("\nMOVE DOWN\n"); break;      //DOWN
        case 3: playerXY[0] -= 1; printf("\nMOVE LEFT\n"); break;      //LEFT
        case 9: playerXY[0] += 1; printf("\nMOVE RIGHT\n"); break;     //RIGHT
    }
    //printf("\nPRETRANS X: %f\tPRETRANS Y: %f", lerpStop[0], lerpStop[1]);
    //printf("\Xshift: %f\tYshift: %f", Xshift, Yshift);
    lerpStop[0] = (playerXY[0] * Xshift);
    lerpStop[1] = ((playerXY[1] * Yshift)-1);
    //printf("\nX: %i\tY: %i", playerXY[0], playerXY[1]);

}

void changeDir() {
    //printf("\nCHANGEDIR %f", lerpProg);
    if (getLegalDir(playerDir) && playerDir % prevDir == 0 && playerDir != prevDir) {
        float coordHolder[2];
        coordHolder[0]  = lerpStop[0];      coordHolder[1]  = lerpStop[1];
        lerpStop[0]     = lerpStart[0];     lerpStop[1]     = lerpStart[1];
        lerpStart[0]    = coordHolder[0];   lerpStart[1]    = coordHolder[1];
        lerpProg = (1-lerpProg);
    }
    else if (getLegalDir(playerDir) && (lerpProg <= 0 || lerpProg >= 1)) {
        lerpStart[0] = lerpStop[0];
        lerpStart[1] = lerpStop[1];
        getLerpCoords();
        lerpProg = 0;
        lerpProg = lerpStep / 2;
    }
}


// -----------------------------------------------------------------------------
// Clean VAO
// -----------------------------------------------------------------------------
void CleanVAO(GLuint &vao)
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

  for(auto vbo : vbos)
    {
    glDeleteBuffers(1, &vbo);
    }

  glDeleteVertexArrays(1, &vao);
}


// -----------------------------------------------------------------------------
// MessageCallback (for debugging purposes)
// -----------------------------------------------------------------------------
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  std::cerr << "GL CALLBACK:" << ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ) <<
    "type = 0x" << type <<
    ", severity = 0x" << severity <<
    ", message =" << message << "\n";
}
