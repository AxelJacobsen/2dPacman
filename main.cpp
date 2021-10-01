#include "shaders/square.h"
#include "shaders/map.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>|
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

GLuint getIndices(int out, int mid, int in);
void fillMap();

void Transform(const float, const GLuint);

//void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader);

void CleanVAO(GLuint &vao);

void GLAPIENTRY MessageCallback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message,
                                const void* userParam);

std::vector<float> findAdjacencies(const int y, const int x, const std::vector<std::vector<int>> levelList, std::vector<std::vector<int>>* logWalls);
std::vector<float> getRelativeCoords(std::vector<int> numPos);
void getRelativeCoordsJustInt(int y, int x);
// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

const int mapSquareNumber = 708;                    // I really wanted to avoid doing this, but due to how you initialize arrays is wierd
const int mapIndiceNumber = mapSquareNumber * 4;    // this was the best solution for me.
const int maxMapCoordNumber = mapSquareNumber * 12;

std::vector<float> coords;

int width = 24, height = 24, walls = 0;

/*
GLfloat square[4 * 3] =
{
  -0.5f, 0.5f, 0.0f,
  -0.5f, -0.5f, 0.0f,
  0.5f, -0.5f, 0.0f,
  0.5f, 0.5f, 0.0f,
};
*/
int spriteSize = 64, resize = 3;
//708 * 12
GLfloat map[maxMapCoordNumber];

GLuint map_indices[708 * 6];

// -----------------------------------------------------------------------------
// Templates
// -----------------------------------------------------------------------------

/**
*  Gets the size of a vector and its contents in bytes
*/
template <typename T>
int sizeof_v(std::vector<T> v) { return sizeof(std::vector<T>) + sizeof(T) * v.size(); }

// -----------------------------------------------------------------------------
// ENTRY POINT
// -----------------------------------------------------------------------------
int main()
{
    
  // Read from level file;
  std::ifstream inn("../../../../levels/level0");
  if (inn) {
      inn >> width; inn.ignore(1); inn >> height;
      std::vector<std::vector<int>> levelVect(height, std::vector<int>(width, 0));
      std::vector<std::vector<int>> wallLog(height, std::vector<int>(width, 0));
      int row = 0, column = 0;
      int temp;
      inn >> temp;
      printf("\n");
      while (column < height) { // adds "walls" int vector
          if (row < width) {
              levelVect[column][row] = temp;
              printf("%i ",temp);
              //if (temp == 1) { walls++; }
              row++;
              inn >> temp;
          }
          else { row = 0; column++; printf("\n"); }
      }
      inn.close();
      
      for (int i = 0; i < height; i++) {    // creates map
          for (int j = 0; j < width; j++) {
              if (levelVect[i][j] == 1){
                  //findAdjacencies(i, j, levelVect, &wallLog);
                  getRelativeCoordsJustInt(i, j);
                  walls++;
                  //printf("\n%i\n", walls);
              }
          }
      }
  }
  else { system("dir"); printf("\n\nBIG PROBLEMO, couldnt find level file\n\n"); }
  
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
  /*
  auto squareVAO =      CreateSquare();
  auto squareShaderProgram =    CompileShader(  squareVertexShaderSrc,
                                                squareFragmentShaderSrc);
  */

  auto mapVAO = CreateMap();
  auto mapShaderProgram = CompileShader(mapVertexShaderSrc,
                                        mapFragmentShaderSrc);
  
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  
  double currentTime = 0.0;
  glfwSetTime(0.0);
  
  while(!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Time management
    currentTime = glfwGetTime();

    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw SQUARE
    /*
    auto greenValue = (sin(currentTime) / 2.0f) + 0.5f;
    auto vertexColorLocation = glGetUniformLocation(squareShaderProgram, "u_Color");
    glUseProgram(squareShaderProgram);
    glBindVertexArray(squareVAO);
    glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
    */
    
    // Draw MAP
    auto mapVertexColorLocation = glGetUniformLocation(mapShaderProgram, "u_Color");
    glUseProgram(mapShaderProgram);
    glBindVertexArray(mapVAO);
    glUniform4f(mapVertexColorLocation, 0.0f, 0.0f, 0.8f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6*mapSquareNumber, GL_UNSIGNED_INT, (const void*)0);
    Transform(currentTime, mapShaderProgram);
    
    glfwSwapBuffers(window);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      break; }
  }

  glUseProgram(0);
  //glDeleteProgram(squareShaderProgram);
  glDeleteProgram(mapShaderProgram);

  //CleanVAO(squareVAO);
  CleanVAO(mapVAO);

  glfwTerminate();

  return EXIT_SUCCESS;
}
/*
std::vector<float> findAdjacencies(const int y, const int x, const std::vector<std::vector<int>> levelList, std::vector<std::vector<int>> *logWalls) {
    std::vector<int>    tempCoords;
    int loggedAmmount = 0;
    int kol = y, rad = x;
    tempCoords.push_back(y);
    tempCoords.push_back(x);
    if (levelList[kol][rad] != 0 && levelList[kol][rad] != 2){
        while (rad < width && levelList[kol][rad] != 0 && levelList[kol][rad] != 2) {
            (*logWalls)[kol][rad] = 5;
            printf("\nLog%i: C: %i R: %i", loggedAmmount, kol, rad);
            rad++;
            if (rad == width && kol != height-1) {
                rad = 0; kol++; 
            }
            else if (rad == width && kol == height - 1) { rad--; break; }
            loggedAmmount++;
        }
        if (levelList[kol][rad] != 2) { rad--; };
    }
    if (loggedAmmount > 1 && rad == 0) {
        (*logWalls)[kol][rad] = 0; 
        printf("\n\nWRAP FIXED\n\n");
        rad = width-1; kol--;
        loggedAmmount--;
        (*logWalls)[kol][rad] = 0;
    }
    if (loggedAmmount == 1) {
        while (levelList[kol][rad] != 0 && kol<height-1 && levelList[kol][rad] != 2) {
            (*logWalls)[kol][rad] = 5;
            printf("\nLog%i: C: %i R: %i", loggedAmmount, kol, rad);
            kol++;
        }
    }
    if (levelList[kol][rad] == 2) {(*logWalls)[kol][rad]++; rad++;  }
    tempCoords.push_back(kol);
    tempCoords.push_back(rad);
    //relativeCoords.push_back(1.0f);
    return getRelativeCoords(tempCoords);
};

std::vector<float> getRelativeCoords(std::vector<int> numPos) {
    std::vector<float>  relativeCoords;
    //create an array with the format for squares
    //TO BE REMOVED
    return relativeCoords;
}
*/
void getRelativeCoordsJustInt(int y, int x) {
    //int current = (walls*12);
    float Xshift = 2.0f / (float(width));
    float Yshift = 2.0f / (float(height));
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


        coords.push_back((tempXs -= 1.0f));
        coords.push_back((tempYs -= 1.0f));
        coords.push_back(0);
        //printf("\nX: %f\tY: %f",tempXs, tempYs);
    }
};

// -----------------------------------------------------------------------------
// Code handling the transformation of objects in the Scene
// -----------------------------------------------------------------------------
void Transform(const float time, const GLuint shaderprogram)
{

    //Presentation below purely for ease of viewing individual components of calculation, and not at all necessary.

    //Translation moves our object.        base matrix      Vector for movement along each axis
    //glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(sin(time), 0.f, 0.f));

    //Rotate the object                    base matrix      degrees to rotate          axis to rotate around
    float pi = glm::pi<float>();
    glm::mat4 rotation = glm::rotate(glm::mat4(1), pi, glm::vec3(0, 0, 1));

    //Scale the object                     base matrix      vector containing how much to scale along each axis (here the same for all axis)
    //glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(sin(time)));

    //Create transformation matrix      These must be multiplied in this order, or the results will be incorrect
    //glm::mat4 transformation = translation * rotation * scale;
    glm::mat4 transformation = rotation;


    //Get uniform to place transformation matrix in
    //Must be called after calling glUseProgram         shader program in use   Name of Uniform
    GLuint transformationmat = glGetUniformLocation(shaderprogram, "u_TransformationMat");

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
/*
// -----------------------------------------------------------------------------
//  CREATE SQUARE
// -----------------------------------------------------------------------------
GLuint CreateSquare()
{

  GLuint square_indices[6] = {0,1,2,0,2,3};

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo;
  glGenBuffers(1, &vbo);

  GLuint ebo;
  glGenBuffers(1, &ebo);

  glBindBuffer( GL_ARRAY_BUFFER, vbo);
  glBufferData( GL_ARRAY_BUFFER,
                sizeof(square),
                square,
                GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, (const void*)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(square_indices), square_indices, GL_STATIC_DRAW);

  return vao;
}
*/

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
    for (int test = 0; test < maxMapCoordNumber; test++) {
        map[test] = coords[test];
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
