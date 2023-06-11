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

enum class Direction {
    Left,
    Right,
    Front,
    Back
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
    vec3 lookPosition;
    vec3 up = vec3(0, 1, 0);
    
    float fovYDegrees = 45.0f;
    float near = 0.0001f;
    float far = 10000.0f;
    Screen screen;
    
    mat4 GetViewingMatrix(){
        return lookAt(position, lookPosition, up);
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
    vector<Vertex> vertices;
    vector<Normal> normals;
    vector<Texture> textures;
    vector<Face> faces;
    GLuint gVertexAttribBuffer;
    GLuint gIndexBuffer;
    int gVertexDataSizeInBytes;
    int gNormalDataSizeInBytes;
    int shaderId;
    int vbo;
    int vao;
    Shader shader;
};

struct Skybox{
    Shader shader;
    int cubemapTexture;
    int vbo;
    int vao;
    
    vector<string> faces
    {
        "right.png",
        "left.png",
        "top.png",
        "bottom.png",
        "front.png",
        "back.png"
    };

    float vertices[108] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
};

struct Object {
    Transform transform;
    vector<Mesh> meshes;
};

struct Ground {
    Object obj;
};

struct Car {
    float speed = 0.0f;
    
    Object obj;
    
    static constexpr float InputAcceleration = 25.0f;
    static constexpr float Deceleration = 10.0f;
};

struct Input {
    int move = 0;
    int rotate = 0;
    Direction direction = Direction::Back;
};

struct Statue{
    Object obj;
};

Car car;
Statue armadillo;
Statue bunny;
Statue teapot;
Camera camera;
Input input;
Time gameTime;
Skybox skybox;
Ground ground;
int wireframeMode = 0;

string GetPath(string originalPath){
    // TODO: Replace after we're out of MAC
    return "/Users/bartubas/Homeworks/deferred-rendering-project/SetupOpenGLExample/" + originalPath;
    // return originalPath;
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        auto path = GetPath(faces[i]);
                
        unsigned char *data = stbi_load(path.data(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
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
                    
                    assert(vIndex[0] == nIndex[0] &&
                           vIndex[1] == nIndex[1] &&
                           vIndex[2] == nIndex[2]); // a limitation for now
                    
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
    
    assert(result.vertices.size() == result.normals.size());
    
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


void InitVBO(Mesh& mesh){
    GLuint vao;
    glGenVertexArrays(1, &vao);
    mesh.vao = vao;
    assert(mesh.vao > 0);
    glBindVertexArray(mesh.vao);
    
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NONE);
    
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
            input.move = 1;
        }
        else{
            input.move = 0;
        }
    }
    else if(key == GLFW_KEY_S){
        if(isPress){
            input.move = -1;
        }
        else{
            input.move = 0;
        }
    }
    else if(key == GLFW_KEY_A){
        if(isPress){
            input.rotate = 1;
        }
        else{
            input.rotate = 0;
        }
    }
    else if(key == GLFW_KEY_D){
        if(isPress){
            input.rotate = -1;
        }
        else{
            input.rotate = 0;
        }
    }
    else if(key == GLFW_KEY_Q){
        if(isPress){
            input.direction = Direction::Left;
        }
    }
    else if(key == GLFW_KEY_E){
        if(isPress){
            input.direction = Direction::Right;
        }
    }
    else if(key == GLFW_KEY_R){
        if(isPress){
            input.direction = Direction::Back;
        }
    }
    else if(key == GLFW_KEY_T){
        if(isPress){
            input.direction = Direction::Front;
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

int CreateShaderProgram(const char* vertexShaderName, const char* fragmentShaderName){
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
    
    return shaderProgramId;
}

Mesh CreateMesh(const string& objPath, const string& vertexPath, const string& fragPath){
    Mesh mesh;
    assert(ParseObj(GetPath(objPath), mesh));
    mesh.shader.programId = CreateShaderProgram(GetPath(vertexPath).data(), GetPath(fragPath).data());
    InitVBO(mesh);
    
    return mesh;
}

void InitCar(){
    auto bodyMesh = CreateMesh("cybertruck_body.obj", "vert_body.glsl", "frag_body.glsl");
    auto programId = bodyMesh.shader.programId;
    glUseProgram(programId);
    float color[] = {0.0f, 0.3f, 0.0f};
    int colorLoc = glGetUniformLocation(programId, "tint");
    glUniform3fv(colorLoc, 1, color);
    
    car.obj.meshes.push_back(bodyMesh);
    car.obj.meshes.push_back(CreateMesh("cybertruck_tires.obj", "vert_tire.glsl", "frag_tire.glsl"));
    car.obj.meshes.push_back(CreateMesh("cybertruck_windows.obj", "vert_window.glsl", "frag_window.glsl"));
}

void InitSkybox(){
    GLuint cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox.vertices), &skybox.vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glBindVertexArray(0);
    skybox.vao = cubeVAO;
    skybox.vbo = cubeVBO;
    // cout << "CubeVao: " << cubeVAO << endl;
    // cout << "CubeVbo: " << cubeVBO << endl;
    
    skybox.shader.programId = CreateShaderProgram(GetPath("vert_skybox.glsl").data(), GetPath("frag_skybox.glsl").data());
    skybox.cubemapTexture = loadCubemap(skybox.faces);
}

Transform groundTransform;
int groundShaderId;
unsigned int groundVbo;
unsigned int groundVao;
unsigned int groundEbo;
unsigned int ourTexture;

void InitGround(){
    groundTransform.position = vec3(0, -10, 0);
    groundTransform.rotation = rotate(groundTransform.rotation, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    groundTransform.scale = vec3(1000.0f, 1000.0f, 1000.0f);
    
    groundShaderId = CreateShaderProgram(GetPath("vert_ground.glsl").data(), GetPath("frag_ground.glsl").data());

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
    unsigned char *data = stbi_load(GetPath("dry-mud.jpg").c_str(), &width, &height, &nrChannels, 0);
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

    glUseProgram(groundShaderId);
    glUniform1i(glGetUniformLocation(groundShaderId, "ourTexture"), 0);
}

void InitStatue(){
    armadillo.obj.transform.position = vec3(15.0f, 0.0f, 15.0f);
    armadillo.obj.transform.scale = vec3(5, 5, 5);
    armadillo.obj.meshes.push_back(CreateMesh("armadillo.obj", "vert_statue.glsl", "frag_statue.glsl"));
    auto programId = armadillo.obj.meshes[0].shader.programId;
    glUseProgram(programId);
    float color[] = {0.8f, 0.0f, 0.0f};
    int colorLoc = glGetUniformLocation(programId, "tint");
    glUniform3fv(colorLoc, 1, color);
    
    bunny.obj.transform.position = vec3(-15, 0.0f, -15.0f);
    bunny.obj.transform.scale = vec3(10, 10, 10);
    bunny.obj.meshes.push_back(CreateMesh("bunny.obj", "vert_statue.glsl", "frag_statue.glsl"));
    programId = bunny.obj.meshes[0].shader.programId;
    glUseProgram(programId);
    float color1[] = {0.8f, 0.8f, 0.0f};
    glUniform3fv(colorLoc, 1, color1);
    
    teapot.obj.transform.position = vec3(-15, 0.0f, 15.0f);
    teapot.obj.transform.scale = vec3(5, 5, 5);
    teapot.obj.meshes.push_back(CreateMesh("teapot.obj", "vert_statue.glsl", "frag_statue.glsl"));
    programId = teapot.obj.meshes[0].shader.programId;
    glUseProgram(programId);
    float color2[] = {0.0f, 0.8f, 0.8f};
    glUniform3fv(colorLoc, 1, color2);
}

GLuint fbo;
GLuint dynamicCubemap;

void InitDynamic(){
    glGenFramebuffers(1, &fbo);
    
    glGenTextures(1, &dynamicCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubemap);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
}

void InitProgram(){
    glEnable(GL_DEPTH_TEST);
    
    InitCar();
    InitSkybox();
    InitGround();
    InitStatue();
    InitDynamic();
}

void ClearScreen(){
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void DrawMesh(const mat4& projectionMatrix, const mat4& viewingMatrix, const mat4& modelingMatrix, const Mesh& mesh){
    if(wireframeMode){
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else{
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    auto shaderId = mesh.shader.programId;
    glUseProgram(shaderId);
    
    glBindVertexArray(mesh.vao);

    glUniformMatrix4fv(glGetUniformLocation(shaderId, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "view"), 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(glGetUniformLocation(shaderId, "model"), 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    glUniform3fv(glGetUniformLocation(shaderId, "cameraPos"), 1, glm::value_ptr(camera.position));
    
    auto cubemapLocation = glGetUniformLocation(shaderId, "cubemap");
    if(cubemapLocation != -1){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, dynamicCubemap);
        glUniform1i(cubemapLocation, 0);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, mesh.gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.gIndexBuffer);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(mesh.gVertexDataSizeInBytes));
    
    glDrawElements(GL_TRIANGLES, mesh.faces.size() * 3, GL_UNSIGNED_INT, 0);
}

void DrawObject(const mat4& projectionMatrix, const mat4& viewingMatrix, const Object& obj) {
    const auto modelingMatrix = obj.transform.GetMatrix();
    
    for(int i = 0; i < obj.meshes.size(); i++){
        DrawMesh(projectionMatrix, viewingMatrix, modelingMatrix, obj.meshes[i]);
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


void UpdateCarPosition(){
    auto& tf = car.obj.transform;
    
    DebugAssert(IsLengthEqual(tf.Forward(), 1.0f), "CarVelocity");
    auto deltaSpeed = 0.0f;
    
    if(input.move != 0){
        auto carAccel = Car::InputAcceleration * input.move;
        deltaSpeed = carAccel * gameTime.deltaTime;
        car.speed += deltaSpeed;
    }
    else{
        auto direction = car.speed > 0.0f ? -1 : 1;
        auto carAccel = Car::Deceleration * direction;
        deltaSpeed = carAccel * gameTime.deltaTime;
        if(abs(deltaSpeed) > abs(car.speed)){
             deltaSpeed = -car.speed;
        }
        
    }
    car.speed += deltaSpeed;
    
    auto carVelocity = tf.Forward() * car.speed;
    tf.position += carVelocity * gameTime.deltaTime;
}

void UpdateCarRotation(){
    const float rotationDegreesPerSecond = 120.0f;
    auto angle = rotationDegreesPerSecond * gameTime.deltaTime * input.rotate;
    auto& carRotation = car.obj.transform.rotation;
    carRotation = rotate(carRotation, radians(angle), vec3(0, 1, 0));
}

void UpdateCar(){
    UpdateCarPosition();
    UpdateCarRotation();
}

void UpdateCamera(){
    const float distance = 20.0f;
    const float upOffset = 5.0f;
    
    vec3 targetPos;
    auto carTf = car.obj.transform;
    
    switch(input.direction){
        case Direction::Left:{
            targetPos = carTf.position + carTf.Right() * distance;
            break;
        }
        case Direction::Back:{
            targetPos = carTf.position - carTf.Forward() * distance;
            break;
        }
        case Direction::Front:{
            targetPos = carTf.position + carTf.Forward() * distance;
            break;
        }
        case Direction::Right:{
            targetPos = carTf.position - carTf.Right() * distance;
            break;
        }
    }
    
    targetPos += vec3(0, 1, 0) * upOffset;
    
    camera.position = targetPos;
    camera.lookPosition = carTf.position;
}

void UpdateTime() {
    auto newTime = (float)glfwGetTime();
    gameTime.deltaTime = newTime - gameTime.time;
    gameTime.time = newTime;
}

void RunSimulation(){
    UpdateTime();
    UpdateCar();
    UpdateCamera();
}

void DrawSkybox(){
    
    auto originalFov = camera.fovYDegrees;
    camera.fovYDegrees = 90.0f;
    
    glDepthMask(GL_FALSE);
    
    auto shader = skybox.shader.programId;
    glUseProgram(shader);
    
    // Removes position from viewing matrix
    auto view = mat4(mat3(camera.GetViewingMatrix()));
    auto projection = camera.GetProjectionMatrix();
    
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
    glBindVertexArray(skybox.vao);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);

    camera.fovYDegrees = originalFov;
}

void DrawGround(const mat4& projectionMatrix, const mat4& viewingMatrix){
    
    auto modelingMatrix = groundTransform.GetMatrix();
    
    glUseProgram(groundShaderId);
    // bind textures on corresponding texture units
    //glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ourTexture);
    
    glUniformMatrix4fv(glGetUniformLocation(groundShaderId, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(glGetUniformLocation(groundShaderId, "view"), 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(glGetUniformLocation(groundShaderId, "model"), 1, GL_FALSE, glm::value_ptr(modelingMatrix));


    glBindVertexArray(groundVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void Render(GLFWwindow* window){
    ClearScreen();
        
    DrawSkybox();
    
    auto projectionMatrix = camera.GetProjectionMatrix();
    auto viewingMatrix = camera.GetViewingMatrix();
    DrawObject(projectionMatrix, viewingMatrix, car.obj);
    DrawObject(projectionMatrix, viewingMatrix, armadillo.obj);
    DrawObject(projectionMatrix, viewingMatrix, bunny.obj);
    DrawObject(projectionMatrix, viewingMatrix, teapot.obj);
    
    DrawGround(projectionMatrix, viewingMatrix);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void ProgramLoop(GLFWwindow* window){
    while (!glfwWindowShouldClose(window))
    {
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
    
    InitProgram();
    
    RegisterKeyPressEvents(window);
    RegisterWindowResizeEvents(window);
    
    ProgramLoop(window);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
