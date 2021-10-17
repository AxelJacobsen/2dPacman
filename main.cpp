/**
 *   OpenGL Pacman Group 34
 *
 *   The Program:
 *     - Draws necessary objects
 *     - Runs wall collision for pacman and ghosts
 *     - Permits consuming pellets
 *     - Utilizes sprites to draw characters
 *     - Has victory conditions with both ghosts and pellets
 *
 *   @file     main.cpp
 *   @author   Axel E.W. Jacobsen, Rafael P. Avalos, Mekides A. Abebe
 */

//Shader Inclusion
#include "shaders/map.h"
#include "shaders/pellets.h"
#include "shaders/ghosts.h"
#include "shaders/player.h"

//Library Inclusion
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <time.h>

//STD Inclusions
#include <iostream>
#include <fstream>
#include <set>
#include <cmath>
#include <vector>


// -----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
// -----------------------------------------------------------------------------

void    callMapCoordinateCreation(std::vector<std::vector<int>> levelVect, std::vector<float>* map);
void    CleanVAO(GLuint& vao);
GLuint  CompileShader(const std::string& vertexShader, const std::string& fragmentShader);
GLuint  CreateMap(std::vector<GLfloat>* map, GLfloat* mapObj);
GLuint  CreateObject(GLfloat *object, int size, const int stride);
void    drawGhosts( const GLuint shader);
void    drawMap(    const GLuint shader, const GLuint vao);
void    drawPacman( const GLuint shader);
void    drawPellets(const GLuint shader, const GLuint vao);
GLfloat getCoordsWithInt(int y,   int x,   int type);
GLuint  getIndices(      int out, int mid, int in);
GLuint  load_opengl_texture(const std::string& filepath, GLuint slot);
void    TransformMap(   const GLuint);
void    TransformPlayer(const GLuint, float lerpProg, float lerpStart[], float lerpStop[]);

std::vector<std::vector<int>> loadFromFile();

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
const int spriteSize = 64;

std::vector<float> coords;

int      width,  height;                   ///< Width and Height kept global due to frequent use in various places      
int      ghostCount = 10;                  ///< Ammount of ghosts, soft cap at 20 due to processing power
float    Xshift, Yshift;                   ///< Width and Height of one "square"
bool        permittPelletUpdate = false,   ///< Reloads Pellet VAO
            run = true;                    ///< End condition

// -----------------------------------------------------------------------------
// Classes
// -----------------------------------------------------------------------------
/**
 *  Character class used for both Pacman and Ghosts
 *  Couldve Used two subclasses, but realized that most functions are used by both,
 *  so i decided to just keep them togheter
 */
class Character {
private:
    //Shared Values
    float   lerpStart[2],               //Contains start coords of LERP
            lerpStop[2],                //Contains stop  coords of LERP 
            speedDiv = 10.0f,           //Higher number = slower speed
            lerpStep = 1.0f/speedDiv,   //Speed of LERP, also slowed by frequency in main
            lerpProg = lerpStep;        //defines progress as step to avoid hickups

    int     dir,                        //Direction character is heading
            prevDir,                    //Previous direction character was heading
            XYpos[2],                   //Value of characters current XY
            animVal = 0;                //A number between 0 - 3 used to define 

    bool    animFlip = true;            //For ghosts flipflops between frames for pac decides which direction to animate
    GLfloat vertices[4 * 5] = { 0.0f }; //Holds character vertices,  X Y Z T1 T2
    

    //AI values
    bool    AI = false;                 //Decides whether object is pacman or ghost
    int     AIdelay = dir;              //Delay for deciding direction of ghost

    //Initialization functions
    void characterInit();
    void convertToVert();
public:
    std::vector<std::vector<int>> mapH; //Holds the level0 map in Pacman[0]
    Character() {};
    Character(int x, int y);
    Character(int x, int y, bool ai);
    ~Character() {  delete vertices; 
                    delete XYpos;
                    delete lerpStart;
                    delete lerpStop;};
    //Shared Functions
    bool getLegalDir(int dir);
    void getLerpCoords();
    void changeDir();
    void Transform(const GLuint ShaderProgram);
    void updateLerp();
    void updateDir(int outDir);
    GLfloat getVertCoord(int index);
    void checkPellet();
    bool checkGhostCollision();
    void characterAnimate(float hMin, float wMin, float hMax, float wMax);
    void pacAnimate();
    void recieveMap(std::vector<std::vector<int>> lvlVect);
    int  getMapVal(int x, int y);
    //AI functions
    int   getRandomAIdir();
    void  AIupdateVertice();
    float AIgetLerpPog();
    int   AIgetXY(int xy);
    void  AIanimate();
};

/**
 *  Pellet class used to store data about all pellets, and
 *  apply functions.
 */
class Pellet {
private:
    int     XYpos[2];                   //Pellets XY
    bool    enabled = true;             //Decides whether to do collision or not
    GLfloat vertices[4 * 3] = { 0.0f }; //Hold pellets vertices X Y Z
public:
    Pellet() {};
    Pellet(int x, int y);
    ~Pellet() { delete vertices; 
                delete XYpos; };
    void initCoords();
    GLfloat getVertCoord(int index);
    void removePellet();
    int checkCoords(int XY);
};

//Container definition
std::vector<Character*> Pacman;     ///< Contains only pacman, done for ease of use
std::vector<Character*> Ghosts;     ///< Contains ghosts
std::vector<Pellet*>   Pellets;     ///< Contains All pellets

//Container Functions
GLuint compileVertices(std::vector<Pellet*> itObj);
GLuint compileVertices(std::vector<Character*> itObj);


// -----------------------------------------------------------------------------
// Class function definition
// -----------------------------------------------------------------------------

/**
 *  Initializes Pacman
 *
 *  @param    x  - Initialization pos X
 *  @param    y  - Initializaiton pos Y
 *  @see      Character::characterInit();
 */
Character::Character(int x, int y) {
    dir = 9, prevDir = 9;
    XYpos[0] = x, XYpos[1] = y;
    characterInit();
};

/**
 *  Initializes Ghosts
 *
 *  @param    x  - Initialization pos X
 *  @param    y  - Initializaiton pos Y
 *  @param    ai - upsates AI to define as Ghost
 *  @see      Character:: getRandomAIdir();
 *  @see      Character:: characterInit();
 */
Character::Character(int x, int y, bool ai) {
    XYpos[0] = x, XYpos[1] = y;
    AI = ai;
    dir = getRandomAIdir();
    AIdelay = dir;
    characterInit();
};

/**
 *  Initializes LERP coords
 *
 *  @see      Character:: convertToVert();
 *  @see      Character:: pacAnimate();
 *  @see      Character:: AIanimate();
 */
void Character::characterInit() { 
    convertToVert();
    lerpStart[0] = vertices[0];
    lerpStart[1] = vertices[1];
    if      (dir == 2) {
        lerpStop[0] = vertices[5];
        lerpStop[1] = vertices[6];
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
        lerpStop[0] = vertices[15];
        lerpStop[1] = vertices[16];
    }
    if (!AI){
        pacAnimate();
    }
    else {
        AIanimate();
    }
    
}

/**
 *  Initializes verticies list
 *
 *  @see      GLfloat getCoordsWithInt(int y,   int x,   int type);
 */
void Character::convertToVert() {
    int loop = 0, callCount = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 3; x++) {
            vertices[loop] = (getCoordsWithInt(XYpos[1], XYpos[0], callCount));
            loop++; callCount++;
        }
        loop += 2;
    }
}

/**
 *  Checks if requested drection is legal/wall or not
 *
 *  @param    dir  - pacmans requested direction
 *  @return   bool whether is is a legal direction or not
 */
bool Character::getLegalDir(int dir) {
    int testPos[2] = { XYpos[0], XYpos[1] };
    switch (dir) {
    case 2: testPos[1] += 1; break;      //UP test
    case 4: testPos[1] -= 1; break;      //DOWN test
    case 3: testPos[0] -= 1; break;      //LEFT test
    case 9: testPos[0] += 1; break;      //RIGHT test
    }
    if ((testPos[0] < width && testPos[1] < height) && (0 <= testPos[0] && 0 <= testPos[1])) {
        if (Pacman[0]->getMapVal(testPos[1],testPos[0]) != 1) { return true; }
        else {return false; }
    }
    else { return false; }    //incase moving outside map illegal untill further notice
    return false;
};

/**
 *  Updates LERP coords
 */
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

/**
 *  Handles direction change requests
 * 
 *  @see      Character:: getRandomAIdir();
 *  @see      Character:: getLegalDir(int dir);
 *  @see      Character:: getLerpCoords();
 */
void Character::changeDir() {
    bool legal = true;
    if (AI && AIdelay == 0) { dir = getRandomAIdir(); AIdelay = ((rand()+4)%10); }
    else {
        AIdelay--;
        legal = getLegalDir(dir);
    }

    if (legal && (dir % prevDir == 0) && dir != prevDir && !AI) {   //Incase you are trying to turn 180 degrees this procs
        float coordHolder[2];
        coordHolder[0] = lerpStop[0];      coordHolder[1] = lerpStop[1];
        lerpStop[0]    = lerpStart[0];     lerpStop[1]    = lerpStart[1];
        lerpStart[0]   = coordHolder[0];   lerpStart[1]   = coordHolder[1];
        lerpProg = (1 - lerpProg);
        getLerpCoords();
        prevDir = dir;
    }
    else if (legal && (lerpProg <= 0 || lerpProg >= 1)) {           //else this handles updating lerp
        lerpStart[0] = lerpStop[0];
        lerpStart[1] = lerpStop[1];
        getLerpCoords();
        lerpProg = lerpStep/2;
        prevDir = dir;
    }
};

/**
 *  passes Transform request onwards with aditional parameters
 *
 *  @param    ShaderProgram  - objects shaderprogram
 *  @see      void TransformPlayer(const GLuint shaderprogram, float lerpProg, float lerpStart[], float lerpStop[])
 */
void Character::Transform(const GLuint ShaderProgram) {
    TransformPlayer(ShaderProgram, lerpProg, lerpStart, lerpStop);
};

/**
 *  Handles LERP updates
 *
 *  @see      Character:: changeDir();
 *  @see      Character:: checkPellet();
 *  @see      Character:: AIupdateVertice();
 */
void Character::updateLerp() {
    if (lerpProg >= 1 || lerpProg <= 0) { changeDir(); }
    else { lerpProg += lerpStep; }

    if (0.5f <= lerpProg && lerpProg <= 0.6 && !AI) {
        checkPellet();
    }
    if (AI) { AIupdateVertice(); }
}

/**
 *  updatesDir with dir from key input
 *
 *  @param    outDir  - new dir from key input
 */
void Character::updateDir(int outDir) {
    dir = outDir;
}

/**
 *  Returns specific vertice
 *
 *  @param    index - desired index
 *  @return   returns the desired vertex
 */
GLfloat Character::getVertCoord(int index) {
    return vertices[index];
};

/**
 *  Checks whether pacman is on a pellet
 *
 *  @see      Pellets:: checkCoords( int XY);
 *  @see      Pellets:: removePellet();
 */
void Character::checkPellet() {
    for (auto& it : Pellets) {
        int check = 0;
        for (int i = 0; i < 2; i++) {
            if (XYpos[i] == it->checkCoords(i)) { check++; }
        }
        if (check == 2) { it->removePellet(); }
    }
};

/**
 *  Checks if pacman is crashing with ghost
 *
 *  @see      Character:: AIgetXY();
 *  @see      Character:: AIgetLerpPog();
 *  @return   returns wheter or not pacman and ghost are crashing
 */
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

/**
 *  Updates texture coordinates
 *
 *  @param    hMin - smallets height point on spritesheet
 *  @param    wMin - smallets width  point on spritesheet
 *  @param    hMax - largest  height point on spritesheet
 *  @param    wMax - largest  height point on spritesheet
 */
void Character::characterAnimate(float hMin, float wMin, float hMax, float wMax) {
    vertices[3]  = wMin;   vertices[4]  =   hMax; // Bot Left
    vertices[8]  = wMin;   vertices[9]  =   hMin; // Top Left
    vertices[13] = wMax;   vertices[14] =   hMin; // Top Right
    vertices[18] = wMax;   vertices[19] =   hMax; // Bot Right
}

/**
 *  Handles pacman animation
 *
 *  @see  Character:: characterAnimate(float hMin, float wMin, float hMax, float wMax);
 */
void Character::pacAnimate() {
    if (animFlip)   { animVal++; }
    else            { animVal--; }
    float wMod = (1.0f / 6.0f);
    float hMod = (1.0f / 4.0f);
    float mhMod = hMod;
    float mwMod = (wMod * animVal) + wMod;
          wMod *= animVal;

    switch (dir) {
    case 2: hMod *= 1; mhMod *= 2; break;   //UP
    case 4: hMod *= 0; mhMod *= 1; break;   //DOWN
    case 3: hMod *= 2; mhMod *= 3; break;   //LEFT
    case 9: hMod *= 3; mhMod *= 4; break;   //RIGHT
    }
    characterAnimate(hMod, wMod, mhMod, mwMod);
    if      (animVal == 3) animFlip = false;
    else if (animVal == 0) animFlip = true;
}

/**
 *  Recieves lvlVect int Pacman[0]
 */
void Character::recieveMap(std::vector<std::vector<int>> lvlVect) {
    mapH = lvlVect;
};

/**
 *  Returns map value
 *
 *  @return returns if coord is wall or not
 */
int Character::getMapVal(int x, int y) {
    return mapH[x][y];
};

/**
 *  Bruteforces a legal direction for AI
 *
 *  @see      Character::getLegalDir(int dir);
 *  @return   returns a legal direction for the AI to take
 */
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

/**
 *  Handles AI movement
 */
void Character::AIupdateVertice() {
    for (int f = 0; f < (4*5); f+=5) {
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

            case 10:  vertices[k] += Xshift;  break;
            case 11:  vertices[k] += Yshift;  break;

            case 15:  vertices[k] += Xshift;  break;
            case 16:  vertices[k];            break;
            default:  vertices[k] = 0.0f;     break;
            }
        }
    }
}

/**
 *  Returns the AIs lerpProg
 * 
 *  @return   AI lerpProg
 */
float Character::AIgetLerpPog() {
    return lerpProg;
}

/**
 *  Returns the AIs XY
 *
 *  @return   AI XY
 */
int Character::AIgetXY(int xy) {
    return XYpos[xy];
}

/**
 *  Handles AI animation
 *
 *  @see  Character:: characterAnimate(float hMin, float wMin, float hMax, float wMax);
 */
void Character::AIanimate() {
    float wMod = (1.0f / 6.0f);
    float hMod = (1.0f / 4.0f);
    float mhMod = hMod, mwMod = wMod;
    if (!animFlip) {
        animFlip = true; 
        animVal = 1; 
        wMod   *= 5;
        mwMod   = 1.0f;
    }
    else {
        animFlip = false; 
        animVal = 0; 
        wMod   *= 4;
        mwMod  *= 5;
    }
    switch (dir) {
        case 2: hMod *= 1; mhMod *= 2; break;   //UP
        case 4: hMod *= 0; mhMod *= 1; break;   //DOWN
        case 3: hMod *= 2; mhMod *= 3; break;   //LEFT
        case 9: hMod *= 3; mhMod *= 4; break;   //RIGHT
    }
    characterAnimate(hMod, wMod, mhMod, mwMod);
};

// -----------------------------------------------------------------------------
// Pellet Functions
// -----------------------------------------------------------------------------

/**
 *  Initializes pellet with x and y
 *
 *  @param    x - pellets designated x
 *  @param    y - pellets designated y
 *  @see      Pellet::initCoords();
 */
Pellet::Pellet(int x, int y) {
    XYpos[0] = x; XYpos[1] = y;
    initCoords();
};

/**
 *  Initializes pellet vertices
 *
 *  @see   GLfloat getCoordsWithInt(int y,   int x,   int type);
 */
void Pellet::initCoords() {
    int loop = 0;
    float Xquart = Xshift / 3.0f;
    float Yquart = Yshift / 3.0f;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 3; x++) {
            vertices[loop] = (getCoordsWithInt(XYpos[1], XYpos[0], loop));
            switch (loop) {
            case 0:  vertices[loop] += Xquart; break;
            case 1:  vertices[loop] += Yquart; break;

            case 3:  vertices[loop] += Xquart; break;
            case 4:  vertices[loop] -= Yquart; break;

            case 6:  vertices[loop] -= Xquart; break;
            case 7:  vertices[loop] -= Yquart; break;

            case 9:  vertices[loop] -= Xquart; break;
            case 10: vertices[loop] += Yquart; break;
            default: vertices[loop] =  0.0f;   break;
            }
            loop++;
        }
    }
}

/**
 *  sets Pellet vertices to 0, effectively removing it
 */
void Pellet::removePellet() {
    if (enabled) {
        for (int i = 0; i < 12; i++) {
            vertices[i] = 0.0f;
        }
        enabled = false;
        permittPelletUpdate = true;
    }
}

/**
 *  Returns desired vertex from pellet
 *
 *  @param    index - desired vertex
 * 
 *  @return   pellets desired vertex
 */
GLfloat Pellet::getVertCoord(int index) {
    return vertices[index];
}

/**
 *  Returns XYpos if pellet is enabled
 *
 *  @param    XY - either x or y
 * 
 *  @return either pellet XY or -1 if disabled
 */
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
/**
 *  main function
 */
int main()
{
    int collected = 0,  //collected pellet counter
        resize = 3;     //resizes window initially 
    std::vector<std::vector<int>> levelVect = loadFromFile();
    std::vector<GLfloat> map;

    // Creates coordinates for map
    callMapCoordinateCreation(levelVect, &map);
    

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

    auto playerShaderProgram = CompileShader(  playerVertexShaderSrc,
                                               playerFragmentShaderSrc);
    
    GLint pposAttrib = glGetAttribLocation(playerShaderProgram, "pPosition");
    glEnableVertexAttribArray(pposAttrib);
    glVertexAttribPointer(pposAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

    auto pelletVAO           = compileVertices(Pellets);
    auto pelletShaderProgram = CompileShader(  pelletVertexShaderSrc,
                                               pelletFragmentShaderSrc);

    auto mapVAO              = CreateMap(&map,(&map[0]));
    auto mapShaderProgram    = CompileShader(  mapVertexShaderSrc,
                                               mapFragmentShaderSrc);

    auto ghostShaderProgram  = CompileShader(  ghostVertexShaderSrc,
                                               ghostFragmentShaderSrc);
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_MULTISAMPLE);

    //Texture loading
    auto spriteSheetP = load_opengl_texture("assets/pacman.png", 0);
    auto spriteSheetG = load_opengl_texture("assets/pacman.png", 1);

    double currentTime = 0.0;
    glfwSetTime(0.0);
    float frequency = 0.01f;
    int animDelay = 10;
    

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        //int Xwidth, Yheight;
        //glfwGetFramebufferSize(window, &Xwidth, &Yheight);

        //glViewport(0,0, Xwidth, Yheight); //Can be uncommented to permit screen stretching

        currentTime = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw Objects
        drawMap(mapShaderProgram, mapVAO);
        drawPellets(pelletShaderProgram, pelletVAO);
        drawPacman(playerShaderProgram);
        drawGhosts(ghostShaderProgram);

        // Reloads pellets after consumption
        if (permittPelletUpdate) {
            CleanVAO(pelletVAO);
            pelletVAO = compileVertices(Pellets);
            permittPelletUpdate = false;
            collected++;
            if (collected == Pellets.size()) {
                run = false;
            }
        }

        //Update all Lerps
        if (currentTime > frequency && run) {
            bool animate = false;

            if (animDelay == 0) { animate = true; animDelay = 3; }  //the effective speed of animation
            else { animDelay--; }

            glfwSetTime(0.0);
            Pacman[0]->updateLerp();

            if (animate) Pacman[0]->pacAnimate();
            for (auto& ghostIt : Ghosts) {
                ghostIt->updateLerp();
                if (animate) ghostIt->AIanimate();
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

    glDeleteTextures(1, &spriteSheetP);
    glDeleteTextures(1, &spriteSheetG);

    CleanVAO(mapVAO);
    CleanVAO(pelletVAO);

    glfwTerminate();

    return EXIT_SUCCESS;
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

    glBindFragDataLocation(shaderProgram, 0, "color");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint load_opengl_texture(const std::string& filepath, GLuint slot)
{
     /** Image width, height, bit depth */
    int w, h, bpp;
    auto pixels = stbi_load(filepath.c_str(), &w, &h, &bpp, STBI_rgb_alpha);

    /*Generate a texture object and upload the loaded image to it.*/
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0 + slot); //Texture Unit
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    /** Set parameters for the texture */
    //Wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //Filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /** Free memory */
    if (pixels)
        stbi_image_free(pixels);

    return tex;
};

// -----------------------------------------------------------------------------
// DRAW FUNCTIONS
// -----------------------------------------------------------------------------
/**
 *  Draws map
 * 
 *  @param shader - shaderprogram to use for drawing
 *  @param vao    - vao of object
 */
void drawMap(const GLuint shader, const GLuint vao) {
    int numElements = (6 * (width * height) - Pellets.size() - 1);
    auto mapVertexColorLocation = glGetUniformLocation(shader, "u_Color");
    glUseProgram(shader);
    glBindVertexArray(vao);
    glUniform4f(mapVertexColorLocation, 0.1f, 0.0f, 0.6f, 1.0f);
    glDrawElements(GL_TRIANGLES, numElements, GL_UNSIGNED_INT, (const void*)0);
}
/**
 *  Draws Pellets
 *
 *  @param shader - shaderprogram to use for drawing
 *  @param vao    - vao of object
 */
void drawPellets(const GLuint shader, const GLuint vao) {
    auto pelletVertexColorLocation = glGetUniformLocation(shader, "u_Color");
    glUseProgram(shader);
    glBindVertexArray(vao);
    glUniform4f(pelletVertexColorLocation, 0.8f, 0.8f, 0.0f, 1.0f);
    glDrawElements(GL_TRIANGLES, int(6 * Pellets.size()), GL_UNSIGNED_INT, (const void*)0);
}
/**
 *  Draws Ghost
 *
 *  @param shader - shaderprogram to use for drawing
 * 
 *  @see compileVertices(std::vector<Character*> itObj)
 *  @see CleanVAO(GLuint& vao)
 */
void drawGhosts(const GLuint shader) {

    auto ghostVAO = compileVertices(Ghosts);

    GLuint gtexAttrib = glGetAttribLocation(shader, "gTexcoord");
    glEnableVertexAttribArray(gtexAttrib);
    glVertexAttribPointer(gtexAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    auto ghostVertexColorLocation = glGetUniformLocation(shader, "gColor");
    auto ghostTextureLocation = glGetUniformLocation(shader, "g_GhostTexture");
    glUseProgram(shader);
    glBindVertexArray(ghostVAO);
    glUniform1i(ghostTextureLocation, 1);

    for (int g = 0; g < (ghostCount*24); g+=24) {
        glUniform4f(ghostVertexColorLocation, 1.0f, 0.0f, 0.0f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)g);
    }

    CleanVAO(ghostVAO);
}
/**
 *  Draws Ghost
 *
 *  @param shader - shaderprogram to use for drawing
 *
 *  @see compileVertices(std::vector<Character*> itObj)
 *  @see Character::Transform(const GLuint ShaderProgram);
 *  @see CleanVAO(GLuint& vao)
 */
void drawPacman(const GLuint shader) {
    auto playerVAO = compileVertices(Pacman);

    GLuint ptexAttrib = glGetAttribLocation(shader, "pTexcoord");
    glEnableVertexAttribArray(ptexAttrib);
    glVertexAttribPointer(ptexAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    auto playerTextureLocation = glGetUniformLocation(shader, "u_PlayerTexture");
    glUseProgram(shader);
    glBindVertexArray(playerVAO);
    glUniform1i(playerTextureLocation, 0);
    Pacman[0]->Transform(shader);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
    CleanVAO(playerVAO);
}

// -----------------------------------------------------------------------------
//  INITIALIZE OBJECT
// -----------------------------------------------------------------------------
/**
 *  Creates object 
 *
 *  @param object    - pointer to object to be created
 *  @param size      - size of object
 *  @param stride    - stride used in object
 * 
 *  @return     returns vao
 */
GLuint CreateObject(GLfloat* object, int size, const int stride)
{
    std::vector<GLuint> object_indices;

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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (sizeof(GLfloat) * stride), (const void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, object_indices.size() * sizeof(object_indices)[0], (&object_indices[0]), GL_STATIC_DRAW);

    return vao;
};


// -----------------------------------------------------------------------------
//  CREATE MAP
// -----------------------------------------------------------------------------
/**
 *  Creates map
 *
 *  @param *map       - pointer to map vector
 *  @param *mapObje   - pointer to first item in map vector, easier to work with this
 * 
 *  @see GLuint getIndices(int out, int mid, int in)
 * 
 *  @return returns vao of map
 */
GLuint CreateMap(std::vector<GLfloat> * map, GLfloat *mapObj) {
    std::vector<GLuint> mapIndices;
    for (int o = 0; o < ((*map).size()/3); o += 4) {
        for (int m = 0; m < 2; m++) {
            for (int i = o; i < (o + 3); i++) {
                mapIndices.push_back(getIndices(o, m, i));
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
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mapIndices.size() * sizeof(mapIndices[0]) , (&mapIndices[0]), GL_STATIC_DRAW);

    return vao;
}
// -----------------------------------------------------------------------------
// SHADER TRANSFORMATIONS
// -----------------------------------------------------------------------------
/**
 *  Performs shader transformation for Pacman
 *
 *  @param shaderprogram - pacmans shaderprogram
 *  @param lerpProg      - current pacman lerpprog
 *  @param lerpStart     - pacman lerpstartXY
 *  @param lerpStop      - pacman lerpstopXY
 */
void TransformPlayer(const GLuint shaderprogram, float lerpProg, float lerpStart[], float lerpStop[])
{
    //LERP performed in the shader for the pacman object
    glm::mat4 translation = glm::translate(glm::mat4(1), glm::vec3(
        (((1 - lerpProg) * lerpStart[0]) + (lerpProg * lerpStop[0])),
        (((1 - lerpProg) * lerpStart[1]) + (lerpProg * lerpStop[1])),
        0.f));

    GLuint transformationmat = glGetUniformLocation(shaderprogram, "u_TransformationMat");

    glUniformMatrix4fv(transformationmat, 1, false, glm::value_ptr(translation));
}

// -----------------------------------------------------------------------------
// GLOBAL FUNCTIONS
// -----------------------------------------------------------------------------

/**
 *  Creates map coordinates and initializes all objects
 *
 *  @param *map       - pointer to map vector
 *  @param *mapObje   - pointer to first item in map vector, easier to work with this
 *
 *  @see GLfloat getCoordsWithInt(int y, int x, int loop);
 *  @see Pellet::checkCoords(int XY);
 */
void callMapCoordinateCreation(std::vector<std::vector<int>> levelVect, std::vector<float>* map) {
    int hallCount = 0;
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
                Pacman[0]->recieveMap(levelVect);
            }
            else { hallCount++;  Pellets.push_back(new Pellet(j, i)); }
        }
    }
    time_t t;
    srand((unsigned)time(&t));
    std::vector<int> formerPositions;
    bool noDouble = false;
    do {
        for (int g = 0; g < ghostCount; g++) {
            int randPos;
            
            randPos = (rand() % hallCount);
            formerPositions.push_back(randPos);
        }
        noDouble = true;
        for (int n = 0; n < ghostCount; n++) {
            for (int m = (n+1); m < (ghostCount); m++) {
                if (formerPositions[n] == formerPositions[m]) {
                    noDouble = false;
                }
            }
        }
    } while (!noDouble);
    int count = 0;
    for (auto& it : Pellets) {
        for (int l = 0; l < ghostCount; l++) {
            if (count == formerPositions[l]) {
                Ghosts.push_back(new Character(it->checkCoords(0), it->checkCoords(1), true));
            }
        }
        count++;
    }
}

/**
 *  returns coordinates based on give x y and "loop"
 *
 *  @param y    - y coordinate for coords
 *  @param x    - x coordinate for coords
 *  @param loop - current repitition / desired vertice
 *
 *  @return returns float for correct coord
 */
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

/**
 *  Returns indice 
 *
 *  @param out  -   outer  for loop value "o"
 *  @param mid  -   middle for loop value "m"
 *  @param in   -   inner for loop value "i"
 */
GLuint getIndices(int out, int mid, int in) {
    if (in == out) { return out; }
    else { return (mid + in); };
}

/**
 *  Reads inn map from file
 *
 *  @return returns map vector
 */
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

/**
 *  Compiles all Pellet verticie lists into a large vector and calls CreateObject
 *
 *  @param itObj - which type of object to iterate, here always pellet
 * 
 *  @see Pellet::getVertCoord(int index);
 *  @see GLuint CreateObject(GLfloat* object, int size, const int stride);
 * 
 *  @return returns VAO gotten from CreateObject func
 */
GLuint compileVertices(std::vector<Pellet*> itObj) {
    std::vector<GLfloat> veticieList;
    int stride = 3;
    for (auto & it : itObj) {
        for (int i = 0; i < 12; i++) {
            veticieList.push_back(it->getVertCoord(i));
        }
    }
    return CreateObject(&veticieList[0], veticieList.size()*sizeof(veticieList[0]), stride);
}

/**
 *  Compiles all verticie lists into a large vector and calls CreateObject
 *
 *  @param itObj - which type of object to iterate, here either Pacman or ghost
 *
 *  @see Character::getVertCoord(int index);
 *  @see GLuint CreateObject(GLfloat* object, int size, const int stride);
 *
 *  @return returns VAO gotten from CreateObject func
 */
GLuint compileVertices(std::vector<Character*> itObj) {
    std::vector<GLfloat> veticieList;
    int stride = 5;
    for (auto& it : itObj) {
        for (int i = 0; i < 20; i++) {
            veticieList.push_back(it->getVertCoord(i));
        }
    }
    return CreateObject(&veticieList[0], veticieList.size() * sizeof(veticieList[0]), stride);
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
