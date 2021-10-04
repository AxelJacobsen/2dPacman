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
GLuint CreatePlayer();

GLuint getIndices(int out, int mid, int in);

void fillMap();
void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect);

void TransformMap(const GLuint);
void TransformPlayer(const GLuint);

void updatePlayerArr(glm::mat4 translation);

void getLerpCoords();

void changeDir();

bool getLegalDir();

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

void getRelativeCoordsJustInt(int y, int x, int type);

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

const int mapSquareNumber = 708;                    // I really wanted to avoid doing this, but due to how you initialize arrays is wierd
const int mapIndiceNumber = mapSquareNumber * 4;    // this was the best solution for me.
const int maxMapCoordNumber = mapSquareNumber * 12;

std::vector<float> coords;

int width = 24, height = 24, walls = 0;

const int spriteSize = 64, resize = 3;

int mockMap[spriteSize*2][spriteSize*2];        //A soft max for map sizes

int playerDir = 9, prevDir = 1; //set to 1 to avoid intDiv by 0

float hastighet = 1.0f / 500.0f;

GLfloat map[maxMapCoordNumber];

GLfloat player[4 * 3];

GLuint map_indices[708 * 6];

float pi = glm::pi<float>();

float lerpStep = 1.0f / 200.0f;
float lerpProg = lerpStep;
float lerpStart[2], lerpStop[2], playerPos[2];
float Xshift, Yshift;

int playerXY[2];
// -----------------------------------------------------------------------------
// Templates
// -----------------------------------------------------------------------------

/**
*  Gets the size of a vector and its contents in bytes
*/
template <typename T>
int sizeof_v(std::vector<T> v) { return sizeof(std::vector<T>) + sizeof(T) * v.size(); }

// -----------------------------------------------------------------------------
// Key Callsback
// -----------------------------------------------------------------------------
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    prevDir = playerDir;
    switch (key) {
        case GLFW_KEY_UP:           playerDir = 2;   break;
        case GLFW_KEY_DOWN:         playerDir = 4;   break;
        case GLFW_KEY_LEFT:         playerDir = 3;   break;
        case GLFW_KEY_RIGHT:        playerDir = 9;   break;
    }
    changeDir();
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
  
  auto playerVAO =              CreatePlayer();
  auto playerShaderProgram =    CompileShader(  playerVertexShaderSrc,
                                                playerFragmentShaderSrc);
  

  auto mapVAO =                 CreateMap();
  auto mapShaderProgram =       CompileShader(  mapVertexShaderSrc,
                                                mapFragmentShaderSrc);
  
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  
  double currentTime = 0.0;
  glfwSetTime(0.0);

  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Time management
    currentTime = glfwGetTime();

    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw MAP
    auto mapVertexColorLocation = glGetUniformLocation(mapShaderProgram, "u_Color");
    glUseProgram(mapShaderProgram);
    glBindVertexArray(mapVAO);
    glUniform4f(mapVertexColorLocation, 0.1f, 0.0f, 0.6f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6*mapSquareNumber, GL_UNSIGNED_INT, (const void*)0);
    TransformMap(mapShaderProgram);
    
    // Draw Player

    //auto playerVAO = CreatePlayer();
    //auto playerShaderProgram = CompileShader(   playerVertexShaderSrc,
    //                                            playerFragmentShaderSrc);

    auto vertexColorLocation = glGetUniformLocation(playerShaderProgram, "u_Color");
    glUseProgram(playerShaderProgram);
    glBindVertexArray(playerVAO);
    glUniform4f(vertexColorLocation, 1.0f, 1.0f, 0.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
    TransformPlayer(playerShaderProgram);

    if (lerpProg >= 1 || lerpProg <= 0) { changeDir(); }
    else { lerpProg += lerpStep; }
    
    glfwSwapBuffers(window);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      break; }

    //glDeleteProgram(playerShaderProgram);
    //CleanVAO(playerVAO);
  }

  glUseProgram(0);
  glDeleteProgram(playerShaderProgram);
  glDeleteProgram(mapShaderProgram);

  CleanVAO(playerVAO);
  CleanVAO(mapVAO);

  glfwTerminate();

  return EXIT_SUCCESS;
}

void getRelativeCoordsJustInt(int y, int x, int type) {
    int twoCounter = 0;
    Xshift = 2.0f / (float(width));
    Yshift = 2.0f / (float(height));
    float tempXs, tempYs;
    for (int i = 0; i < 4; i++) {
        if (x == 0 && y == 0) { tempXs = 0, tempYs = 0; }
        else {
            tempXs = (Xshift * x), tempYs = (Yshift * y);
        }
        // Coordinate order is:
        //      Top Left
        //      Bot Left
        //      Bot Right
        //      Top Right
        switch (i) {
            case 0:   tempXs;           tempYs;             break;  // Top Left

            case 1:   tempXs;           tempYs += Yshift;   break;  // Bot Left

            case 2:   tempXs += Xshift; tempYs += Yshift;   break;  // Bot Right

            case 3:   tempXs += Xshift; tempYs;             break;  // Top Right
        }

        if (type == 1){
            coords.push_back((tempXs -= 1.0f));
            coords.push_back((tempYs -= 1.0f));
            coords.push_back(0);
        }
        else if ( type == 2 ) {
            playerXY[0] = (x-1), playerXY[1] = y;

            player[twoCounter] = (tempXs - 1.0f);
            lerpStart[0] = (tempXs-Xshift);
            lerpStop[0] = (tempXs);
            printf("\npX: %f", player[twoCounter]);
            twoCounter++;

            player[twoCounter] = ((tempYs - 1.0f)+ Yshift);
            lerpStart[1] = player[twoCounter];
            lerpStop[1] = lerpStart[1];
            printf("\tpY: %f", player[twoCounter]);
            twoCounter++;

            player[twoCounter] = 0;
            printf("\tpZ: %f", player[twoCounter]);
            twoCounter++;
        }
        //printf("\nX: %f\tY: %f",tempXs, tempYs);
    }
};

// -----------------------------------------------------------------------------
// Code handling the transformation of objects in the Scene
// -----------------------------------------------------------------------------
void TransformMap(const GLuint shaderprogram)
{
    glm::mat4 rotation = glm::rotate(glm::mat4(1), pi, glm::vec3(0, 0, 1));

    glm::mat4 transformation = rotation;

    GLuint transformationmat = glGetUniformLocation(shaderprogram, "u_TransformationMat");

    glUniformMatrix4fv(transformationmat, 1, false, glm::value_ptr(transformation));
}

void TransformPlayer(const GLuint shaderprogram)
{

    //Presentation below purely for ease of viewing individual components of calculation, and not at all necessary.

    //Translation moves our object.        base matrix      Vector for movement along each axis
    glm::mat4 translation;
    translation = glm::translate(glm::mat4(1), glm::vec3((((1 - lerpProg) * lerpStart[0]) + (lerpProg * lerpStop[0])),
                                                         (((1 - lerpProg) * lerpStart[1]) + (lerpProg * lerpStop[1])), 
                                                            0.f));//LERP

    printf("\nX: %f", (((1 - lerpProg) * lerpStart[0]) + (lerpProg * lerpStop[0])));
    printf("\tY: %f", (((1 - lerpProg) * lerpStart[1]) + (lerpProg * lerpStop[1])));

    //translation = glm::translate(glm::mat4(1), glm::vec3(1.0f, 0.f, 0.f));                 //STILL

    /*
    switch (playerDir) {
        case 2:  translation = glm::translate(glm::mat4(1), glm::vec3(0.f, hastighet, 0.f));  break;    //UP
        case 4:  translation = glm::translate(glm::mat4(1), glm::vec3(0.f, -hastighet, 0.f)); break;    //DOWN
        case 3:  translation = glm::translate(glm::mat4(1), glm::vec3(-hastighet, 0.f, 0.f));  break;   //LEFT
        case 9:  translation = glm::translate(glm::mat4(1), glm::vec3(hastighet, 0.f, 0.f)); break;     //RIGHT
        default: translation = glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.f, 0.f));                 //STILL
    }
    */
    //Create transformation matrix      These must be multiplied in this order, or the results will be incorrect
    glm::mat4 transformation = translation;
    //Get uniform to place transformation matrix in
    //Must be called after calling glUseProgram         shader program in use   Name of Uniform
    GLuint transformationmat = glGetUniformLocation(shaderprogram, "u_TransformationMat");
    
    //updatePlayerArr(translation);

    //Send data from matrices to uniform
    //                 Location of uniform  How many matrices we are sending    value_ptr to our transformation matrix
    glUniformMatrix4fv(transformationmat, 1, false, glm::value_ptr(transformation));
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
//  CREATE SQUARE
// -----------------------------------------------------------------------------
GLuint CreatePlayer()
{

  GLuint player_indices[6] = {0,1,2,0,2,3};

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo;
  glGenBuffers(1, &vbo);

  GLuint ebo;
  glGenBuffers(1, &ebo);

  glBindBuffer( GL_ARRAY_BUFFER, vbo);
  glBufferData( GL_ARRAY_BUFFER,
                sizeof(player),
                player,
                GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (const void*)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(player_indices), player_indices, GL_STATIC_DRAW);

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
                tempMapVect[column][row] = temp;
                mockMap[column][row] = temp;
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
            if (levelVect[i][j] == 1 || levelVect[i][j] == 2) {
                //findAdjacencies(i, j, levelVect, &wallLog);
                getRelativeCoordsJustInt(i, j, levelVect[i][j]);
                walls++;
                //printf("\n%i\n", walls);
            }
        }
    }
}

void updatePlayerArr(glm::mat4 translation) {
    if (playerDir >= 0) {      
        glm::mat4 playMat;
        int cringe = 0;
        for (int i = 0; i < 4; i++) {
            printf("\n");
            for (int y = 0; y < 3; y++) {
                playMat[i][y] = player[cringe];
                if (playerDir % 2 == 0) {
                    playMat[i][1] += translation[3][1];
                }
                else if (playerDir % 3 == 0) {
                    playMat[i][0] += translation[3][0];
                }

                player[cringe] = playMat[i][y];
                printf("%f\t", translation[i][y]);
                cringe++;
            }
        }
    }
}

bool getLegalDir() {
    if (0 < lerpProg && lerpProg < 1) { return true; }

    int testPos[2] = {playerXY[0], playerXY[1]};
    switch (playerDir) {
        case 2: testPos[1] -= 1; break;      //UP test
        case 4: testPos[1] += 1; break;      //DOWN test
        case 3: testPos[0] -= 1; break;      //LEFT test 
        case 9: testPos[0] += 1; break;      //RIGHT test
    }

    if (testPos[0] < width && testPos[1] < height) {
        if (mockMap[testPos[1]][testPos[0]] != 1){ return true;  }
                                             else{ return false; }
    } else { return false; }    //incase moving outside map illegal untill further notice
    return false;
}

void getLerpCoords() {
    switch (playerDir) {
        case 2: playerXY[1] -= 1; break;      //UP
        case 4: playerXY[1] += 1; break;      //DOWN
        case 3: playerXY[0] -= 1; break;      //LEFT
        case 9: playerXY[0] += 1; break;      //RIGHT
    }
    lerpStop[0] = (playerXY[0] * Xshift);
    lerpStop[1] = (playerXY[1] * Yshift);
}

void changeDir() {
    
    if (getLegalDir() && playerDir % prevDir == 0 && playerDir != prevDir) {
        float coordHolder[2];
        for (int i = 0; i < 2; i++) {
            coordHolder[i]  = lerpStop[i];
            lerpStop[i]     = lerpStart[i];
            lerpStart[i]    = coordHolder[i];
        }
        lerpProg = (1 - lerpProg);
    }
    else if (getLegalDir() && (lerpProg <= 0 || lerpProg >= 1)) {
        lerpStart[0] = lerpStop[0];
        lerpStart[1] = lerpStop[1];
        getLerpCoords();
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

void Camera(const float time, const GLuint shaderprogram)
{
    //glOrtho(0.0f, 0.0f, float(width), float(height),1,100);
    //Matrix which helps project our 3D objects onto a 2D image. Not as relevant in 2D projects
    //The numbers here represent the aspect ratio. Since our window is a square, aspect ratio here is 1:1, but this can be changed.
    glm::mat4 projection = glm::ortho(0.0f, 0.0f, float(width), float(height));

    //Matrix which defines where in the scene our camera is
    //                           Position of camera     Direction camera is looking     Vector pointing upwards
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    //Get unforms to place our matrices into
    GLuint projmat = glGetUniformLocation(shaderprogram, "u_ProjectionMat");
    GLuint viewmat = glGetUniformLocation(shaderprogram, "u_ViewMat");

    //Send data from matrices to uniform
    glUniformMatrix4fv(projmat, 1, false, glm::value_ptr(projection));
    glUniformMatrix4fv(viewmat, 1, false, glm::value_ptr(view));
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
