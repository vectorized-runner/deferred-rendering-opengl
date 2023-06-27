#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <random>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
using namespace glm;

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
    Face(int v[], int t[], int n[]) {
        vIndex[0] = v[0];
        vIndex[1] = v[1];
        vIndex[2] = v[2];
        tIndex[0] = t[0];
        tIndex[1] = t[1];
        tIndex[2] = t[2];
        nIndex[0] = n[0];
        nIndex[1] = n[1];
        nIndex[2] = n[2];
    }
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

struct Time {
    float time;
    float deltaTime;
};

struct Transform{
    vec3 position = vec3(0, 0, 0);
    quat rotation = quat(1, 0, 0, 0);
    vec3 scale = vec3(1, 1, 1);
    
    mat4 GetMatrix() const {
        auto id = glm::mat4(1.0f);
        auto t = glm::translate(id, position);
        auto r = glm::toMat4(rotation);
        auto s = glm::scale(id, scale);
        
        return t * r * s;
    }
    
    vec3 Up() const {
        return rotate(rotation, vec3(0, 1, 0));
    }
    
    vec3 Forward() const {
        return rotate(rotation, vec3(0, 0, 1));
    }
    
    vec3 Right() const {
        return rotate(rotation, vec3(1, 0, 0));
    }
};

struct Screen{
    int width = 800;
    int height = 600;
};

struct Camera{
    
    vec3 position;
    vec3 lookDir;
    vec3 up = vec3(0, 1, 0);
    
    float fovYDegrees = 60.0f;
    float near = 0.0001f;
    float far = 10000.0f;
    Screen screen;
    
    mat4 GetViewingMatrix(){
        auto lookPos = position + lookDir;
        return lookAt(position, lookPos, up);
    }
    
    mat4 GetProjectionMatrix(){
        float fovYRadians = radians(fovYDegrees);
        float aspect = screen.width / (float)screen.height;
        return perspective(fovYRadians, aspect, near, far);
    }
};

struct Shader {
    int programId;
};

struct Mesh {
    string path;
    vector<Vertex> vertices;
    vector<Normal> normals;
    vector<Texture> textures;
    vector<Face> faces;
    GLuint gVertexAttribBuffer;
    GLuint gIndexBuffer;
    int gVertexDataSizeInBytes;
    int gNormalDataSizeInBytes;
    int vbo;
    int vao;
    Shader forwardShader;
    Shader deferredShader;
};

struct Object {
    string name;
    Transform transform;
    vector<int> meshIndices;
};

struct Enemy {
    float speed = 0.0f;
    
    int objIndex;
};

struct Player {
    float speed = 0.0f;

    int objIndex;
    
    static constexpr float MoveSpeed = 40.0f;
};

struct Input {
    float moveForward;
    float moveRight;
    double mouseX;
    double mouseY;
    double mouseDeltaX;
    double mouseDeltaY;
    int jump;
};

struct Scene {
    vector<Object> objects;
    vector<Mesh> meshes;
    
    static constexpr int maxLightCount = 100;
    vec3 lightPos[maxLightCount];
    vec3 lightIntensity[maxLightCount];
    vec3 ligthVelocity[maxLightCount];
    int lightObjIndex[maxLightCount];
    bool lightHitGround[maxLightCount];
    
    int lightCount;
};

vector<Enemy> enemies;
Scene scene;
Player player;
Camera camera;
Input input;
Time gameTime;
int wireframeMode = 0;
int renderDeferred = 0;
bool firstFrame = true;
random_device rd;
mt19937 gen(rd());

Shader deferredLightShader;
Shader deferredGeometryShader;
Shader forwardGeometryShader;
Shader lightMeshShader;
// TODO: Ground Shader Forward/Deferred
Shader forwardGroundShader;

const float intensityMin = 5.0f;
const float intensityMax = 100.0f;
const float enemySpeed = 5.0f;
const int enemyCount = 20;

float RandomFloat(float minValue, float maxValue) {
    std::uniform_real_distribution<float> dist(minValue, maxValue);
    return dist(gen);
}

vec3 RandomPointInCircle(vec3 center, float radiusMin, float radiusMax){
    auto r = RandomFloat(radiusMin, radiusMax);
    auto angle = RandomFloat(-360.0f, 360.0f);
    auto rad = radians(angle);
    auto dir = normalize(vec3(cos(rad), 0.0f, sin(rad)));
    
    return center + r * dir;
}

float RandomFloat(){
    return RandomFloat(0.0f, 1.0f);
}

vec3 RandomVec3(float min, float max){
    return vec3(RandomFloat(min, max), RandomFloat(min, max), RandomFloat(min, max));
}

int GetMeshIndex(const string& path){
    auto meshCount = scene.meshes.size();
    
    for(int i = 0; i < meshCount; i++){
        auto& mesh = scene.meshes[i];
        if(mesh.path == path){
            return i;
        }
    }
    return -1;
}

string GetPath(string originalPath){
    // TODO: Replace after we're out of MAC
    return "/Users/bartubas/Homeworks/deferred-rendering-project/SetupOpenGLExample/" + originalPath;
    // return originalPath;
}

bool ParseObj(const string& fileName, Mesh& result)
{
    fstream myfile;
    
    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);
    
    if (myfile.is_open())
    {
        string curLine;
        
        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;
            
            if (curLine.length() >= 2)
            {
                if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        result.textures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        result.normals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        result.vertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
                    char c;
                    int vIndex[3], nIndex[3], tIndex[3];
                    str >> vIndex[0]; str >> c >> c; // consume "//"
                    str >> nIndex[0];
                    str >> vIndex[1]; str >> c >> c; // consume "//"
                    str >> nIndex[1];
                    str >> vIndex[2]; str >> c >> c; // consume "//"
                    str >> nIndex[2];
                    
                    // assert(vIndex[0] == nIndex[0] &&
                           // vIndex[1] == nIndex[1] &&
                           // vIndex[2] == nIndex[2]); // a limitation for now
                    
                    // make indices start from 0
                    for (int c = 0; c < 3; ++c)
                    {
                        vIndex[c] -= 1;
                        nIndex[c] -= 1;
                        tIndex[c] -= 1;
                    }
                    
                    result.faces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }
            
            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }
        
        myfile.close();
    }
    else
    {
        return false;
    }
    
    /*
     for (int i = 0; i < gVertices.size(); ++i)
     {
     Vector3 n;
     
     for (int j = 0; j < gFaces.size(); ++j)
     {
     for (int k = 0; k < 3; ++k)
     {
     if (gFaces[j].vIndex[k] == i)
     {
     // face j contains vertex i
     Vector3 a(gVertices[gFaces[j].vIndex[0]].x,
     gVertices[gFaces[j].vIndex[0]].y,
     gVertices[gFaces[j].vIndex[0]].z);
     
     Vector3 b(gVertices[gFaces[j].vIndex[1]].x,
     gVertices[gFaces[j].vIndex[1]].y,
     gVertices[gFaces[j].vIndex[1]].z);
     
     Vector3 c(gVertices[gFaces[j].vIndex[2]].x,
     gVertices[gFaces[j].vIndex[2]].y,
     gVertices[gFaces[j].vIndex[2]].z);
     
     Vector3 ab = b - a;
     Vector3 ac = c - a;
     Vector3 normalFromThisFace = (ab.cross(ac)).getNormalized();
     n += normalFromThisFace;
     }
     
     }
     }
     
     n.normalize();
     
     gNormals.push_back(Normal(n.x, n.y, n.z));
     }
     */
    
    // assert(result.vertices.size() == result.normals.size());
    
    return true;
}

// Add m_ prefix to math methods that I've written over GLM
float m_lerp(float a, float b, float t){
    return (1.0f - t) * a + b * t;
}

void DebugAssert(bool condition, const char* message){
    if(!condition){
        cout << "AssertionFailed: " << message << endl;
        // exit(-1);
    }
}

void SetWindowTitle(GLFWwindow* window){
    char rendererInfo[512] = { 0 };
    strcpy(rendererInfo, (const char*)glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char*)glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);
}

void InitGlew(){
    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        exit(-1);
    }
}

GLFWwindow* CreateWindow(){
    const auto screenName = "HW2";
    auto window = glfwCreateWindow(camera.screen.width, camera.screen.height, screenName, NULL, NULL);
    return window;
}

void AddWindowHints(){
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
}

void printGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cout << "OpenGL Error: " << gluErrorString(error) << std::endl;
    }
}


void InitVBO(Mesh& mesh){
    GLuint vao;
    glGenVertexArrays(1, &vao);
    mesh.vao = vao;
    assert(mesh.vao > 0);
    glBindVertexArray(mesh.vao);
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    printGLError();
    
    glGenBuffers(1, &mesh.gVertexAttribBuffer);
    glGenBuffers(1, &mesh.gIndexBuffer);
    
    assert(mesh.gVertexAttribBuffer > 0 && mesh.gIndexBuffer > 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh.gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.gIndexBuffer);
    
    mesh.gVertexDataSizeInBytes = mesh.vertices.size() * 3 * sizeof(GLfloat);
    mesh.gNormalDataSizeInBytes = mesh.normals.size() * 3 * sizeof(GLfloat);
    int indexDataSizeInBytes = mesh.faces.size() * 3 * sizeof(GLuint);
    GLfloat* vertexData = new GLfloat[mesh.vertices.size() * 3];
    GLfloat* normalData = new GLfloat[mesh.normals.size() * 3];
    GLuint* indexData = new GLuint[mesh.faces.size() * 3];
    
    float minX = 1e6, maxX = -1e6;
    float minY = 1e6, maxY = -1e6;
    float minZ = 1e6, maxZ = -1e6;
    
    auto& vertices = mesh.vertices;
    
    for (int i = 0; i < vertices.size(); ++i)
    {
        vertexData[3 * i] = vertices[i].x;
        vertexData[3 * i + 1] = vertices[i].y;
        vertexData[3 * i + 2] = vertices[i].z;
        
        minX = std::min(minX, vertices[i].x);
        maxX = std::max(maxX, vertices[i].x);
        minY = std::min(minY, vertices[i].y);
        maxY = std::max(maxY, vertices[i].y);
        minZ = std::min(minZ, vertices[i].z);
        maxZ = std::max(maxZ, vertices[i].z);
    }
    
    // std::cout << "minX = " << minX << std::endl;
    // std::cout << "maxX = " << maxX << std::endl;
    // std::cout << "minY = " << minY << std::endl;
    // std::cout << "maxY = " << maxY << std::endl;
    // std::cout << "minZ = " << minZ << std::endl;
    // std::cout << "maxZ = " << maxZ << std::endl;
    
    auto& normals = mesh.normals;
    
    for (int i = 0; i < normals.size(); ++i)
    {
        normalData[3 * i] = normals[i].x;
        normalData[3 * i + 1] = normals[i].y;
        normalData[3 * i + 2] = normals[i].z;
    }
    
    auto& faces = mesh.faces;
    
    for (int i = 0; i < faces.size(); ++i)
    {
        indexData[3 * i] = faces[i].vIndex[0];
        indexData[3 * i + 1] = faces[i].vIndex[1];
        indexData[3 * i + 2] = faces[i].vIndex[2];
    }
    
    
    glBufferData(GL_ARRAY_BUFFER, mesh.gVertexDataSizeInBytes + mesh.gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.gVertexDataSizeInBytes, vertexData);
    glBufferSubData(GL_ARRAY_BUFFER, mesh.gVertexDataSizeInBytes, mesh.gNormalDataSizeInBytes, normalData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);
    
    // done copying; can free now
    delete[] vertexData;
    delete[] normalData;
    delete[] indexData;
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(mesh.gVertexDataSizeInBytes));
}

unsigned int gBuffer;
unsigned int gPosition;
unsigned int gNormal;
unsigned int gAlbedoSpec;

void InitDeferredRendering(){
    cout << "InitDeferredRendering" << endl;

    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    
    auto width = camera.screen.width;
    auto height = camera.screen.height;
      
    // - position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
      
    // - normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
      
    // - color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
      
    // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
      
    // TODO: Pass lighting info to the Shader.
    
    // TODO: Shader configuration.
    
    // TODO: Render passes.
}

void InitForwardRendering(){
    cout << "InitForwardRendering" << endl;
}

void OnKeyAction(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_REPEAT){
        return;
    }
    
    auto isPress = action == GLFW_PRESS;
    
    if (key == GLFW_KEY_ESCAPE){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    else if(key == GLFW_KEY_K){
        if(wireframeMode != 1){
            wireframeMode = 1;
        }
        else{
            wireframeMode = 0;
        }
    }
    else if(key == GLFW_KEY_W){
        if(isPress){
            input.moveForward = 1;
        }
        else{
            input.moveForward = 0;
        }
    }
    else if(key == GLFW_KEY_S){
        if(isPress){
            input.moveForward = -1;
        }
        else{
            input.moveForward = 0;
        }
    }
    else if(key == GLFW_KEY_A){
        if(isPress){
            input.moveRight = -1;
        }
        else{
            input.moveRight = 0;
        }
    }
    else if(key == GLFW_KEY_D){
        if(isPress){
            input.moveRight = 1;
        }
        else{
            input.moveRight = 0;
        }
    }
    else if(key == GLFW_KEY_SPACE){
        // TODO: Jump
    }
    else if(key == GLFW_KEY_F){
        if(isPress){
            renderDeferred = !renderDeferred;
            
            if(renderDeferred == 1){
                InitDeferredRendering();
            }
            else{
                InitForwardRendering();
            }
        }
    }
}

void OnWindowResized(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
    
    camera.screen.width = width;
    camera.screen.height = height;
}

void RegisterKeyPressEvents(GLFWwindow* window){
    glfwSetKeyCallback(window, OnKeyAction);
}

void RegisterWindowResizeEvents(GLFWwindow* window){
    // https://stackoverflow.com/questions/52730164/what-should-i-pass-to-glfwsetwindowsizecallback
    // glfwSetWindowSizeCallback(window, OnWindowResized);
    glfwSetFramebufferSizeCallback(window, OnWindowResized);
}

bool ReadDataFromFile(
                      const string& fileName, ///< [in]  Name of the shader file
                      string& data)     ///< [out] The contents of the file
{
    fstream myfile;
    
    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);
    
    if (myfile.is_open())
    {
        string curLine;
        
        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }
        
        myfile.close();
    }
    else
    {
        return false;
    }
    
    return true;
}

GLuint CreateShader(const char* name, GLenum shaderType){
    string shaderSource;
    string filename(name);
    
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }
    
    const char* vertexShaderSource = shaderSource.c_str();
    auto shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &vertexShaderSource, NULL);
    glCompileShader(shaderId);
    
    int success;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if(!success){
        const int outputLength = 512;
        char infoLog[outputLength];
        glGetShaderInfoLog(shaderId, outputLength, NULL, infoLog);
        cout << name << " Shader Compile Log: \n" << infoLog << endl;
    }
    
    
    return shaderId;
}

GLuint CreateVertexShader(const char* name){
    return CreateShader(name, GL_VERTEX_SHADER);
}

GLuint CreateFragmentShader(const char* name){
    return CreateShader(name, GL_FRAGMENT_SHADER);
}

Shader CreateShaderProgram(const char* vertexShaderName, const char* fragmentShaderName){
    auto shaderProgramId = glCreateProgram();
    DebugAssert(shaderProgramId != -1, "ShaderProgram Failed.");
    
    const auto vertexShaderId = CreateVertexShader(vertexShaderName);
    const auto fragmentShaderId  = CreateFragmentShader(fragmentShaderName);
    
    glAttachShader(shaderProgramId, vertexShaderId);
    glAttachShader(shaderProgramId, fragmentShaderId);
    
    glLinkProgram(shaderProgramId);
    GLint success;
    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
    
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgramId, 512, NULL, infoLog);
        cout << "Program link failed: " << infoLog << endl;
        exit(-1);
    }
    
    // We don't need the Shaders after linking them to the program anymore
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);
    
    Shader shader;
    shader.programId = shaderProgramId;
    
    return shader;
}

const Mesh& GetMesh(int index){
    return scene.meshes[index];
}


int CreateMesh(const string& objPath, Shader forwardShader, Shader deferredShader){
    auto idx = GetMeshIndex(objPath);
    if(idx != -1){
        return idx;
    }
    
    Mesh mesh;
    mesh.path = objPath;
    assert(ParseObj(GetPath(objPath), mesh));
    mesh.forwardShader = forwardShader;
    mesh.deferredShader = deferredShader;
    InitVBO(mesh);
    idx = scene.meshes.size();
    scene.meshes.push_back(mesh);
    
    return idx;
}


Object& GetPlayerObj(){
    return scene.objects[player.objIndex];
}

Object& GetEnemyObj(int index){
    return scene.objects[index];
}

void InitEnemies() {
    const float spawnRadiusMin = 100.0f;
    const float spawnRadiusMax = 300.0f;
    const float enemyScale = 3.0f;
    
    for(int i = 0; i < enemyCount; i++){
        auto enemy = Enemy();
        
        auto obj = Object();
        obj.name = "Enemy";
        auto playerPos = GetPlayerObj().transform.position;
        auto enemyPos = RandomPointInCircle(playerPos, spawnRadiusMin, spawnRadiusMax);
        
        auto dist = distance(playerPos, enemyPos);
        
        cout << "Dist is: " << to_string(dist) << endl;
        
        obj.transform.position = enemyPos + vec3(0, enemyScale, 0);
        obj.transform.scale = vec3(enemyScale);
        
        auto mesh = CreateMesh("armadillo.obj", forwardGeometryShader, deferredGeometryShader);
        obj.meshIndices.push_back(mesh);
        
        auto objIndex = scene.objects.size();
        scene.objects.push_back(obj);
        enemy.objIndex = objIndex;
        
        enemies.push_back(enemy);
    }
    
    cout << "Enemies created." << endl;
}


void InitPlayer(){
    auto playerObj = Object();
    playerObj.name = "Player";
    
    playerObj.transform.position = vec3(0, 0, 0);
    
    auto forward = vec3(0.0, 0.0f, 1.0f); // The direction vector to look at
    auto up = vec3(0.0f, 1.0f, 0.0f); // The up vector
    auto rotation = quatLookAt(forward, up);
    playerObj.transform.rotation = rotation;
        
    auto index = scene.objects.size();
    scene.objects.push_back(playerObj);
    player.objIndex = index;
}

Transform groundTransform;
unsigned int groundVbo;
unsigned int groundVao;
unsigned int groundEbo;
unsigned int ourTexture;

void InitGround(){
    groundTransform.position = vec3(0, -0.1f, 0);
    groundTransform.rotation = rotate(groundTransform.rotation, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    groundTransform.scale = vec3(1000.0f, 1000.0f, 1000.0f);
    
    forwardGroundShader = CreateShaderProgram(GetPath("shaders/vert_ground.glsl").data(), GetPath("shaders/frag_ground.glsl").data());

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // texture coords
         0.5f,  0.5f, 0.0f,   100.0f, 100.0f, // top right
         0.5f, -0.5f, 0.0f,   100.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   0.0f, 100.0f  // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &groundVao);
    glGenBuffers(1, &groundVbo);
    glGenBuffers(1, &groundEbo);

    glBindVertexArray(groundVao);

    glBindBuffer(GL_ARRAY_BUFFER, groundVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // load and create a texture
    // -------------------------
    // texture 1
    // ---------
    glGenTextures(1, &ourTexture);
    glBindTexture(GL_TEXTURE_2D, ourTexture);
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);    // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
    unsigned char *data = stbi_load(GetPath("doom-ground.jpg").c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    glUseProgram(forwardGroundShader.programId);
    glUniform1i(glGetUniformLocation(forwardGroundShader.programId, "ourTexture"), 0);
}

void CheckError(){
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        // Handle the error, e.g., print an error message or perform error-specific actions
        const char* errorMessage = reinterpret_cast<const char*>(gluErrorString(error));
        printf("Error setting uniform int: %s\n", errorMessage);
    }
}

void UpdateLightDataForShader(Shader shader){
    auto lightCount = scene.lightCount;
    auto shaderId = shader.programId;
    glUseProgram(shaderId);

    auto lightPosLoc = glGetUniformLocation(shaderId, "lightPositions");
    assert(lightPosLoc != -1);
    glUniform3fv(lightPosLoc, lightCount, glm::value_ptr(scene.lightPos[0]));
    CheckError();
    auto lightIntensityLoc = glGetUniformLocation(shaderId, "lightIntensities");
    glUniform3fv(lightIntensityLoc, lightCount, glm::value_ptr(scene.lightIntensity[0]));
    CheckError();
    auto lightCountLoc = glGetUniformLocation(shaderId, "lightCount");
    glUniform1i(lightCountLoc, scene.lightCount);
    CheckError();
}

int GetRenderShader(const Mesh& mesh){
    if(renderDeferred == 0){
        // Forward
        return mesh.forwardShader.programId;
    }
    
    return mesh.deferredShader.programId;
}

// TODO: Ensure light data uses the same naming/offsets in the Shaders.
// TODO: Use different shader for lights (not the one used for forward/deferred rendering)
void CreateLight(vec3 pos, vec3 vel){
    if(scene.lightCount >= scene.maxLightCount)
    {
        cout << "max lights reached." << endl;
        return;
    }
        
    auto idx = scene.lightCount++;
    auto intensity = RandomVec3(intensityMin, intensityMax);
    scene.lightPos[idx] = pos;
    scene.lightIntensity[idx] = intensity;
    scene.ligthVelocity[idx] = vel;
    
    auto lightObj = Object();
    lightObj.name = "Light";
    lightObj.transform.position = pos;
    lightObj.transform.scale = vec3(0.1f);
    // TODO: Handle lights differently
    auto lightMesh = CreateMesh("sphere.obj", lightMeshShader, lightMeshShader);
    auto lightShader = GetRenderShader(GetMesh(lightMesh));
    glUseProgram(lightShader);
    auto unlitLoc = glGetUniformLocation(lightShader, "unlit");
    assert(unlitLoc != -1);
    glUniform3fv(unlitLoc, 1, glm::value_ptr(intensity));
    CheckError();
    
    lightObj.meshIndices.push_back(lightMesh);
    auto objIndex = scene.objects.size();
    scene.objects.push_back(lightObj);
    
    scene.lightObjIndex[idx] = objIndex;    
}

void InitLights(){
    int lightCount = 50;
    float posMin = -25.0f;
    float posMax = 25.0f;
    
    for(int i = 0; i < lightCount; i++){
        auto randomPos = RandomVec3(posMin, posMax);
        auto randomIntensity = RandomVec3(intensityMin, intensityMax);
        auto idx = scene.lightCount++;
        scene.lightPos[idx] = randomPos;
        scene.lightIntensity[idx] = randomIntensity;
    }
}

void AddRandomObjs(){
    auto armadillo = Object();
    armadillo.name = "Armadillo";
    armadillo.transform.position = vec3(15.0f, 0.0f, 15.0f);
    armadillo.transform.scale = vec3(5, 5, 5);
    armadillo.meshIndices.push_back(CreateMesh("armadillo.obj", forwardGeometryShader, deferredGeometryShader));
    auto programId = GetRenderShader(GetMesh(armadillo.meshIndices[0]));
    glUseProgram(programId);
    float color[] = {0.8f, 0.0f, 0.0f};
    int colorLoc = glGetUniformLocation(programId, "tint");
    glUniform3fv(colorLoc, 1, color);
    scene.objects.push_back(armadillo);
    
    auto bunny = Object();
    bunny.name = "Bunny";
    bunny.transform.position = vec3(-15, 0.0f, -15.0f);
    bunny.transform.scale = vec3(10, 10, 10);
    bunny.meshIndices.push_back(CreateMesh("bunny.obj", forwardGeometryShader, deferredGeometryShader));
    programId = GetRenderShader(GetMesh(bunny.meshIndices[0]));
    glUseProgram(programId);
    float color1[] = {0.8f, 0.8f, 0.0f};
    glUniform3fv(colorLoc, 1, color1);
    scene.objects.push_back(bunny);
    
    auto teapot = Object();
    teapot.name = "Teapot";
    teapot.transform.position = vec3(-15, 0.0f, 15.0f);
    teapot.transform.scale = vec3(5, 5, 5);
    teapot.meshIndices.push_back(CreateMesh("teapot.obj", forwardGeometryShader, deferredGeometryShader));
    programId = GetRenderShader(GetMesh(teapot.meshIndices[0]));
    glUseProgram(programId);
    float color2[] = {0.0f, 0.8f, 0.8f};
    glUniform3fv(colorLoc, 1, color2);
    scene.objects.push_back(teapot);
}

void InitScene(){

    // AddRandomObjs();
    
    int cubeCountX = 100;
    int cubeCountZ = 100;
    int idx = 0;
    auto separation = 20.0f;
    auto scaleY = 5.0f;
    auto scaleXZ = 2.0f;
    
    for(int x = 0; x < cubeCountX; x++){
        for(int z = 0; z < cubeCountZ; z++){
            auto cube = Object();
            cube.name = "Cube " + to_string(idx);
            cube.transform.position = vec3(x * separation, scaleY, z * separation);
            cube.transform.scale = vec3(scaleXZ, scaleY, scaleXZ);
            cube.meshIndices.push_back(CreateMesh("cube.obj", forwardGeometryShader, deferredGeometryShader));
        
            scene.objects.push_back(cube);
            
            idx++;
        }
    }
    
    // InitLights();
}


void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {

        auto tf = GetPlayerObj().transform;
        auto playerPos = tf.position;
        auto upOffset = vec3(0, 5.0f, 0);
        auto lightPos = playerPos + upOffset;
        
        auto lightVelocity = tf.Forward() * 25.0f;
        
        CreateLight(lightPos, lightVelocity);
        
        cout << "light created" << endl;
    }
}

void CreateShaders(){
    forwardGeometryShader = CreateShaderProgram(
                                        GetPath("shaders/vert_forward.glsl").data(),
                                        GetPath("shaders/frag_forward.glsl").data());
    
    deferredGeometryShader = CreateShaderProgram(
                                        GetPath("shaders/vert_deferred_geometry.glsl").data(),
                                        GetPath("shaders/frag_deferred_geometry.glsl").data());
    
    deferredLightShader = CreateShaderProgram(
                                        GetPath("shaders/vert_deferred_light.glsl").data(),
                                        GetPath("shaders/frag_deferred_light.glsl").data());
    
    lightMeshShader = CreateShaderProgram(
                                        GetPath("shaders/vert_lights.glsl").data(),
                                        GetPath("shaders/frag_lights.glsl").data());
    
    glUseProgram(deferredGeometryShader.programId);
    
    glUniform1i(glGetUniformLocation(deferredLightShader.programId, "gPosition"), 0);
    glUniform1i(glGetUniformLocation(deferredLightShader.programId, "gNormal"), 1);
    glUniform1i(glGetUniformLocation(deferredLightShader.programId, "gAlbedoSpec"), 2);
}

void InitProgram(GLFWwindow* window){
    glEnable(GL_DEPTH_TEST);
    
    CreateShaders();
    
    InitPlayer();
    InitGround();
    InitScene();
    InitEnemies();
    
    // Hide the cursor
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetMouseButtonCallback(window, MouseButtonCallback);
}


void DrawMesh(const mat4& projectionMatrix, const mat4& viewingMatrix, const mat4& modelingMatrix, const Mesh& mesh, const Shader& shader){
    if(wireframeMode){
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    auto shaderId = shader.programId;
    glUseProgram(shaderId);
    
    glBindVertexArray(mesh.vao);

    glUniformMatrix4fv(glGetUniformLocation(shaderId, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    CheckError();
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "view"), 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    CheckError();
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "model"), 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    CheckError();
    glUniform3fv(glGetUniformLocation(shaderId, "cameraPos"), 1, glm::value_ptr(camera.position));
    CheckError();
    
    // Not sure if these are required after we're doing vao already, a question for later.
    glBindBuffer(GL_ARRAY_BUFFER, mesh.gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.gIndexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(mesh.gVertexDataSizeInBytes));
    
    glDrawElements(GL_TRIANGLES, mesh.faces.size() * 3, GL_UNSIGNED_INT, 0);
}

void DrawObject(const mat4& projectionMatrix, const mat4& viewingMatrix, const Object& obj, bool deferred) {
    const auto modelingMatrix = obj.transform.GetMatrix();
    
    for(int i = 0; i < obj.meshIndices.size(); i++){
        auto& mesh = GetMesh(obj.meshIndices[i]);
        auto shader = deferred ? mesh.deferredShader : mesh.forwardShader;
        DrawMesh(projectionMatrix, viewingMatrix, modelingMatrix, mesh, shader);
    }
}

vec3 ClampLength(vec3 vector, float clampLength){
    DebugAssert(clampLength >= 0.0f, "ClampLength");
    
    if(length(vector) > clampLength){
        return normalize(vector) * clampLength;
    }
    
    return vector;
}

bool IsLengthEqual(vec3 v, float l){
    return abs(length(v) - l) < 0.001f;
}


vec3 ProjectOnPlane(vec3 vec, vec3 planeNormal){
    return vec - dot(vec, planeNormal) * planeNormal;
}

vec3 GetPlayerMoveVector(){
    
    auto& tf = GetPlayerObj().transform;
    auto forward = input.moveForward * tf.Forward();
    // Not sure why this needs to be negative
    auto right = -input.moveRight * tf.Right();
    auto sum = forward + right;
    
    if(length(sum) < 0.01f){
        return vec3(0, 0, 0);
    }
    
    return normalize(ProjectOnPlane(sum, vec3(0, 1, 0)));
}

void UpdateEnemies(){
    auto playerPos = GetPlayerObj().transform.position;
    
    for(int i = 0; i < enemyCount; i++){
        auto& enemy = enemies[i];
        auto& obj = GetEnemyObj(enemy.objIndex);
        auto pos = obj.transform.position;
        auto stoppingDistance = 2.0f;
        auto distance = glm::distance(pos, playerPos);
        
        if (distance <= stoppingDistance){
            continue;
        }
        
        auto dirToPlayer = normalize(playerPos - pos);
        auto moveAmount = dirToPlayer * enemySpeed * gameTime.deltaTime;
        auto newPos = pos + moveAmount;
        
        // move towards player
        obj.transform.position = newPos;
        
        // look at player
        obj.transform.rotation = quatLookAt(dirToPlayer, vec3(0, 1, 0));
        
        // cout << "NewPosForEnemy" << to_string(newPos) << "pPos" << to_string(playerPos) << endl;
    }
}

void UpdatePlayer(){
    
    auto vec = GetPlayerMoveVector();
    auto& tf = GetPlayerObj().transform;
    auto dt = gameTime.deltaTime;
    auto deltaMove = vec * Player::MoveSpeed * dt;
    tf.position += deltaMove;
    
    // Rotate left by mouseScrollX * constant
    const float rotateMultiplier = 0.5f;
    auto up = vec3(0, 1, 0);
    float rotateAnglesZ = rotateMultiplier * -input.mouseDeltaX;
    auto rotateZ = angleAxis(radians(rotateAnglesZ), up);
    
    auto right = vec3(1, 0, 0);
    float rotateAnglesX = rotateMultiplier * input.mouseDeltaY;
    auto rotateX = angleAxis(radians(rotateAnglesX), right);
        
    tf.rotation = tf.rotation * rotateX * rotateZ;
    
    // TODO: Player model should have constant rotation relative to the Camera, achieve that.
}

void UpdateCamera(){
    vec3 offset = vec3(0.0f, 5.0f, -5.0f);
    
    vec3 targetPos;
    auto carTf = GetPlayerObj().transform;
    targetPos = carTf.position + carTf.Right() * offset.x + carTf.Up() * offset.y + carTf.Forward() * offset.z;
    camera.position = targetPos;
    camera.lookDir = carTf.Forward();
}

void UpdateTime() {
    auto newTime = (float)glfwGetTime();
    gameTime.deltaTime = newTime - gameTime.time;
    gameTime.time = newTime;
}

void UpdateLightData(){
    UpdateLightDataForShader(deferredLightShader);
    UpdateLightDataForShader(forwardGeometryShader);
}

void UpdateLights(){
    auto acceleration = vec3(0, -10, 0);
    auto dt = gameTime.deltaTime;
    auto minHeight = 1.0f;
    
    for(int i = 0; i < scene.lightCount; i++){
        
        if(scene.lightHitGround[i]){
            // Just add friction
            auto& velocity = scene.ligthVelocity[i];
            velocity.y = 0.0f;
            
            if(length(velocity) > 0.01f){
                // Stop in one second
                velocity = normalize(velocity) * length(velocity) * (1.0f - gameTime.deltaTime);
            }
            
            auto& pos = scene.lightPos[i];
            pos += velocity * dt;
            
            scene.objects[scene.lightObjIndex[i]].transform.position = pos;
        }
        else{
            auto& velocity = scene.ligthVelocity[i];
            velocity += acceleration * dt;
            auto& pos = scene.lightPos[i];
            pos += velocity * dt;
        
            if(pos.y <= minHeight){
                pos.y = minHeight;
                scene.lightHitGround[i] = true;
            }
            
            scene.objects[scene.lightObjIndex[i]].transform.position = pos;
        }
    }
    
    UpdateLightData();
}

void RunSimulation(){
    UpdateTime();
    UpdatePlayer();
    UpdateEnemies();
    UpdateLights();
    UpdateCamera();
    firstFrame = false;
}

void DrawGround(const mat4& projectionMatrix, const mat4& viewingMatrix){
    
    auto modelingMatrix = groundTransform.GetMatrix();
    
    glUseProgram(forwardGroundShader.programId);
    // bind textures on corresponding texture units
    // glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ourTexture);
    
    // TODO: Make this matrix naming same in all Shaders
    glUniformMatrix4fv(glGetUniformLocation(forwardGroundShader.programId, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(glGetUniformLocation(forwardGroundShader.programId, "view"), 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(glGetUniformLocation(forwardGroundShader.programId, "model"), 1, GL_FALSE, glm::value_ptr(modelingMatrix));

    glBindVertexArray(groundVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void DrawSceneForward(){
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    auto objectCount = scene.objects.size();
    auto projectionMatrix = camera.GetProjectionMatrix();
    auto viewingMatrix = camera.GetViewingMatrix();
    
    for(int i = 0; i < objectCount; i++){
        const auto& obj = scene.objects[i];
        
        // TODO: Remove later
        if(obj.name == "Player")
            continue;
        
        DrawObject(projectionMatrix, viewingMatrix, obj, renderDeferred);
    }
    
    DrawGround(projectionMatrix, viewingMatrix);
}

// TODO: Actual implementation of each method in here.
void DrawSceneDeferred(){
    
    /*
    // render
    // ------
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 1. geometry pass: render scene's geometry/color data into gbuffer
    // -----------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);
    shaderGeometryPass.use();
    shaderGeometryPass.setMat4("projection", projection);
    shaderGeometryPass.setMat4("view", view);
    for (unsigned int i = 0; i < objectPositions.size(); i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, objectPositions[i]);
        model = glm::scale(model, glm::vec3(0.25f));
        shaderGeometryPass.setMat4("model", model);
        backpack.Draw(shaderGeometryPass);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
    // -----------------------------------------------------------------------------------------------------------------------
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shaderLightingPass.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    // send light relevant uniforms
    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
        shaderLightingPass.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
        // update attenuation parameters and calculate radius
        const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
        const float linear = 0.7f;
        const float quadratic = 1.8f;
        shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
        shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
        // then calculate radius of light volume/sphere
        const float maxBrightness = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
        float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
        shaderLightingPass.setFloat("lights[" + std::to_string(i) + "].Radius", radius);
    }
    shaderLightingPass.setVec3("viewPos", camera.Position);
    // finally render quad
    renderQuad();

    // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
    // ----------------------------------------------------------------------------------
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
    // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
    // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3. render lights on top of scene
    // --------------------------------
    shaderLightBox.use();
    shaderLightBox.setMat4("projection", projection);
    shaderLightBox.setMat4("view", view);
    for (unsigned int i = 0; i < lightPositions.size(); i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, lightPositions[i]);
        model = glm::scale(model, glm::vec3(0.125f));
        shaderLightBox.setMat4("model", model);
        shaderLightBox.setVec3("lightColor", lightColors[i]);
        renderCube();
    }
     
     */
}

void DrawScene(){
    if(renderDeferred == 0){
        DrawSceneForward();
    } else{
        DrawSceneDeferred();
    }
}

void Render(GLFWwindow* window){
    DrawScene();
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void UpdateInput(GLFWwindow* window){
    auto prevX = input.mouseX;
    auto prevY = input.mouseY;
    
    glfwGetCursorPos(window, &input.mouseX, &input.mouseY);
    
    if(firstFrame) {
        input.mouseDeltaX = 0;
        input.mouseDeltaY = 0;
    }
    else{
        input.mouseDeltaX = input.mouseX - prevX;
        input.mouseDeltaY = input.mouseY - prevY;
    }
}

void ProgramLoop(GLFWwindow* window){
    while (!glfwWindowShouldClose(window))
    {
        UpdateInput(window);
        RunSimulation();
        Render(window);
    }
}


int main()
{
    if (!glfwInit())
    {
        cout << "GLFWInit Failed." << endl;
        return -1;
    }
    
    AddWindowHints();
    
    auto window = CreateWindow();
    if (!window)
    {
        cout << "CreateWindow Failed." << endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    
    InitGlew();
    SetWindowTitle(window);
    
    InitProgram(window);
    
    RegisterKeyPressEvents(window);
    RegisterWindowResizeEvents(window);
    
    ProgramLoop(window);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
