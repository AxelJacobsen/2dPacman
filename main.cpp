#include "shaders/triangle.h"
#include "shaders/square.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>|
#include <set>
#include <cmath>
#include <vector>

// -----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// -----------------------------------------------------------------------------
GLuint CompileShader(const std::string& vertexShader,
                     const std::string& fragmentShader);
GLuint CreateTriangle();
GLuint CreateSquare();

void CleanVAO(GLuint &vao);

void GLAPIENTRY MessageCallback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message,
                                const void* userParam);
std::vector<float> findAdjacencies(const int y, const int x, const std::vector<std::vector<int>> levelList, std::vector<std::vector<int>> *logWalls);
std::vector<float> getRelativeCoords(std::vector<int> numPos);

int width = 24, height = 24;
int globTest = 3;

GLfloat triangle[3 * 3 * 2] =
{
  -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
  0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f
};

// -----------------------------------------------------------------------------
// ENTRY POINT
// -----------------------------------------------------------------------------
int main()
{
    int spriteSize = 64;
    int resize = 3;
    
  // Read from level file;
  std::ifstream inn("../../../../levels/level0");
  if (inn) {
      inn >> width; inn.ignore(1); inn >> height;
      printf("\nWidth: %i\nHeight: %i\n", width, height);
      std::vector<std::vector<int>> levelVect(height, std::vector<int>(width, 0));
      std::vector<std::vector<int>> wallLog(height, std::vector<int>(width, 0));
      int row = 0, column = 0;
      int temp;
      inn >> temp;
      printf("\n");
      while (column < height) {
          if (row < width) {
              levelVect[column][row] = temp;
              printf("%i ",temp);
              row++;
              inn >> temp;
          }
          else { row = 0; column++; printf("\n"); }
      }
      inn.close();
      
      for (int i = 0; i < height; i++) {
          for (int j = 0; j < width; j++) {
              if (levelVect[i][j] == 1 && wallLog[i][j] == 0){//wallLog[i][j] >= 5 && wallLog[i][j] != 6) { // 5 means that the "coord" has already been included in an earlier draw
                  findAdjacencies(i, j, levelVect, &wallLog);
              }
          }
      }

      printf("\n\n\n");
      row = 0, column = 0;
      while (column < height) {
          if (row < width) {
              levelVect[column][row];
              printf("%i ", wallLog[column][row]);
              row++;
          }
          else { row = 0; column++; printf("\n"); }
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

  auto window = glfwCreateWindow(width*spriteSize/resize, height*spriteSize/resize, "Lab02", nullptr, nullptr);
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

  auto squareVAO = CreateSquare();
  auto squareShaderProgram = CompileShader(squareVertexShaderSrc,
                                           squareFragmentShaderSrc);

  auto triangleVAO = CreateTriangle();
  auto triangleShaderProgram = CompileShader(triangleVertexShaderSrc,
                                             triangleFragmentShaderSrc);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  bool alternate = false;
  double currentTime = 0.0;
  double lastTime = 0.0;
  glfwSetTime(0.0);
  while(!glfwWindowShouldClose(window))
    {
    glfwPollEvents();

    // Time management
    currentTime = glfwGetTime();
    if (currentTime - lastTime > 1.0)
      {
      alternate = !alternate;
      lastTime = currentTime;
      }

    glClear(GL_COLOR_BUFFER_BIT);

    // Draw SQUARE
    auto greenValue = (sin(currentTime) / 2.0f) + 0.5f;
    auto vertexColorLocation = glGetUniformLocation(squareShaderProgram, "u_Color");
    glUseProgram(squareShaderProgram);
    glBindVertexArray(squareVAO);
    glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

    // Draw TRIANGLE
    auto alternateFlagLocation = glGetUniformLocation(triangleShaderProgram, "u_AlternateFlag");
    glUseProgram(triangleShaderProgram);
    glBindVertexArray(triangleVAO);
    glUniform1ui(alternateFlagLocation, static_cast<unsigned int>(alternate));
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      {
      break;
      }
    }

  glUseProgram(0);
  glDeleteProgram(triangleShaderProgram);
  glDeleteProgram(squareShaderProgram);

  CleanVAO(triangleVAO);
  CleanVAO(squareVAO);

  glfwTerminate();

  return EXIT_SUCCESS;
}

std::vector<float> findAdjacencies(const int y, const int x, const std::vector<std::vector<int>> levelList, std::vector<std::vector<int>> *logWalls) {
    std::vector<int>    tempCoords;
    int loggedAmmount = 0;
    int kol = y, rad = x;
    tempCoords.push_back(y);
    tempCoords.push_back(x);
    if (levelList[kol][rad] != 0 && levelList[kol][rad] != 2){
        while (rad < width && levelList[kol][rad] != 0 && levelList[kol][rad] != 2) {
            (*logWalls)[kol][rad] = globTest;// 5;
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
            (*logWalls)[kol][rad] = globTest; //5;
            printf("\nLog%i: C: %i R: %i", loggedAmmount, kol, rad);
            kol++;
        }
    }
    if (levelList[kol][rad] == 2) {(*logWalls)[kol][rad]++; rad++;  }
    globTest++;
    tempCoords.push_back(kol);
    tempCoords.push_back(rad);
    //relativeCoords.push_back(1.0f);
    return getRelativeCoords(tempCoords);
};

std::vector<float> getRelativeCoords(std::vector<int> numPos) {
    std::vector<float>  relativeCoords;
    //create an array with the format for squares
    return relativeCoords;
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
//  CREATE TRIANGLE
// -----------------------------------------------------------------------------
GLuint CreateTriangle()
{

  GLuint vao;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6, (const void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*6, (const void*)12);
   
  GLfloat alternateColor[4] = {0.0f, 0.0f, 1.0f, 1.0f};
  glVertexAttrib4fv(2, alternateColor);

  return vao;
}

// -----------------------------------------------------------------------------
//  CREATE SQUARE
// -----------------------------------------------------------------------------
GLuint CreateSquare()
{
  GLfloat square[4*3] =
    {
      -0.5f, 0.5f, 0.0f,
      -0.5f, -0.5f, 0.0f,
      0.5f, -0.5f, 0.0f,
      0.5f, 0.5f, 0.0f,
    };

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
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*3, (const void *)0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(square_indices), square_indices, GL_STATIC_DRAW);

  return vao;
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

    //Matrix which helps project our 3D objects onto a 2D image. Not as relevant in 2D projects
    //The numbers here represent the aspect ratio. Since our window is a square, aspect ratio here is 1:1, but this can be changed.
    glm::mat4 projection = glm::ortho(-1.f, 1.f, -1.f, 1.f);

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
