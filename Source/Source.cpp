/*Gino Murin - 3D Modeling Using OpenGL
* The following code expands on previous iterations by implementing
* the following changes:
*
* --Created two seperate textures on the laptop
* screen to texture the front and back.
*
* These changes were implemented by:
* 1. Updating the fragment shader with a second texture container
* 2. Loading the second texture from file
* 3. Loading both the first and second texture into our shader program
* 4. Rendering the mesh with the second texture
*/

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include "camera.h"
// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//image library implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Gino Murin - 6/5/2022"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Declares a camera wit specific x,y,z position
    Camera camera(glm::vec3(0.0f, 5.0f, 8.0f));
    float lastX = WINDOW_WIDTH / 2.0f;
    float lastY = WINDOW_HEIGHT / 2.0f;
    bool firstMouse = true;

    // timing
    float deltaTime = 0.0f;	// time between current frame and last frame
    float lastFrame = 0.0f;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Triangle mesh data
    GLMesh gMesh, tblMesh, lidMesh, cylMesh, screenMesh;
    unsigned int texture, baseTexture, lidTexture, screenTexture, desktopTexture;

    // Shader program
    GLuint gProgramId;
}



/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
void CreateLaptopBase(GLMesh& gMesh);
void RenderLaptopBase();
void CreateLaptopLid(GLMesh& lidMesh);
void RenderLaptopLid();
void CreateLaptopScreen(GLMesh& screenMesh);
void RenderLaptopScreen();
void CreateTable(GLMesh& tblMesh);
void RenderTable();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
//**Callback functions added to handle keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
//flag to toggle orthogonal/perspective views
bool isOrtho = false;

/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0

layout(location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
//Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
    TexCoord = aTexCoord;
}
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    out vec4 fragmentColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform sampler2D uExtraTexture;
uniform bool multipleTextures;

void main()
{
    fragmentColor = mix(texture(ourTexture, TexCoord), texture(uExtraTexture, TexCoord), 1.0);
   
}
);


// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    //Functions to create meshes for objects
    CreateLaptopBase(gMesh);
    CreateLaptopLid(lidMesh);
    CreateTable(tblMesh);
    CreateLaptopScreen(screenMesh);
    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // input
        // -----
        UProcessInput(gWindow);
        glBindTexture(GL_TEXTURE_2D, texture);
        // Render this frame
        URender();
        glm::mat4 view = camera.GetViewMatrix();
        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(gMesh);
    UDestroyMesh(lidMesh);
    UDestroyMesh(tblMesh);  
    UDestroyMesh(screenMesh);
    // Release shader program
    UDestroyShaderProgram(gProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, mouse_callback);
    glfwSetScrollCallback(*window, scroll_callback);
    //Initialize key callback
    glfwSetKeyCallback(*window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// Called when a key is pressed. Necessary to create a toggle for perspective. 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE) return; //only handle press events
    if (key == GLFW_KEY_P) isOrtho = !isOrtho;
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));

}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    //***Maps Q AND E Keys to up and down respectively***
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    //Calls to render each individual object
    RenderTable();
    RenderLaptopBase();
    RenderLaptopLid();
    RenderLaptopScreen();


    // Deactivate the Vertex Array Object
    glBindVertexArray(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

//DEPRECATED
//
//// Implements the UCreateMesh function
//void UCreateMesh(GLMesh& mesh)
//{
//    // Position and Color data for pyramid
//    GLfloat verts[] = {
//        // Vertex Positions    // Colors (r,g,b,a)
//         0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f, // Top Vertex 0
//         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f, // Bottom Right Vertex 1
//        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f, // Bottom Left Vertex 2
//         0.5f, -0.5f, -1.0f,  1.0f, 0.0f, 1.0f, 1.0f, // Bottom Right  back 3
//        -0.5f, -0.5f, -1.0f,  1.0f, 0.1f, 0.0f, 1.0f  // Bottom Left Back 4
//    };
//
//    // Index position of each individual triangle used to draw the pyramid
//    GLushort indices[] = {
//        0, 1, 3,  // Triangle 1
//        0, 4, 3,  // Triangle 2
//        0, 4, 2,  // Triangle 3
//        0, 2, 1,  // Triangle 4
//        1, 2 ,4,  // Triangle 5
//        1, 3, 4,  // Triangle 6
//    };
//
//    const GLuint floatsPerVertex = 3;
//    const GLuint floatsPerColor = 4;
//
//    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
//    glBindVertexArray(mesh.vao);
//
//    // Create 2 buffers: first one for the vertex data; second one for the indices
//    glGenBuffers(2, mesh.vbos);
//    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
//    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU
//
//    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
//
//    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
//    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each
//
//    // Create Vertex Attribute Pointers
//    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
//    glEnableVertexAttribArray(0);
//
//    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
//    glEnableVertexAttribArray(1);
//}

//// Implements the UCreateMesh function
//void UCylinderMesh(GLMesh& cylMesh)
//{   
//    
//    // Position and Color data for pyramid
//    GLfloat verts[] = {1};
//   
//    // Index position of each individual triangle used to draw the pyramid
//    GLushort indices[] = {
//        0, 1, 3,  // Triangle 1
//        0, 4, 3,  // Triangle 2
//        0, 4, 2,  // Triangle 3
//        0, 2, 1,  // Triangle 4
//        1, 2 ,4,  // Triangle 5
//        1, 3, 4,  // Triangle 6
//    };
//
//    const GLuint floatsPerVertex = 3;
//    const GLuint floatsPerColor = 4;
//
//    glGenVertexArrays(1, &cylMesh.vao); // we can also generate multiple VAOs or buffers at the same time
//    glBindVertexArray(cylMesh.vao);
//
//    // Create 2 buffers: first one for the vertex data; second one for the indices
//    glGenBuffers(2, cylMesh.vbos);
//    glBindBuffer(GL_ARRAY_BUFFER, cylMesh.vbos[0]); // Activates the buffer
//    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU
//
//    cylMesh.nIndices = cyl.getIndexCount();
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
//
//    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
//    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each
//
//    // Create Vertex Attribute Pointers
//    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
//    glEnableVertexAttribArray(0);
//
//    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
//    glEnableVertexAttribArray(1);
//}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}

// loads vertex, index, and color data into for laptop base into mesh
void CreateLaptopBase(GLMesh& mesh) {
    GLfloat verts[] = {
        // Vertex Positions    
         0.5f,  0.03f, 0.0f,      1.0f, 1.0f,  // Top-Right Vertex 0
         0.5f, -0.03f, 0.0f,      1.0f, 0.0f,    // Bottom-Right Vertex 1
        -0.5f, -0.03f, 0.0f,      0.0f, 0.0f,    // Bottom-Left Vertex 2
        -0.5f,  0.03f, 0.0f,      0.0f, 1.0f,    // Top-Left Vertex 3

         0.5f, -0.03f, -1.0f,      1.0f, 1.0f,   // 4 br  right
         0.5f,  0.03f, -1.0f,      1.0f, 0.0f,   //  5 tl  right
        -0.5f,  0.03f, -1.0f,      0.0f, 0.0f,   //  6 tl  top
        -0.5f, -0.03f, -1.0f,      0.0f, 1.0f    //  7 bl back
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3, // Triangle 1
        1, 2, 3, // Triangle 2
        0, 1, 4, // Triangle 3
        0, 4, 5, // Triangle 4
        0, 5, 6, // Triangle 5
        0, 3, 6, // Triangle 6
        4, 5, 6, // Triangle 7
        4, 6, 7, // Triangle 8
        2, 3, 6, // Triangle 9
        2, 6, 7, // Triangle 10
        1, 4, 7, // Triangle 11
        1, 2, 7  // Triangle 12
    };

    const GLuint floatsPerVertex = 3;

    const GLuint floatsPerTexture = 2;



    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);



    glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerTexture));
    glEnableVertexAttribArray(2);

    glGenTextures(1, &baseTexture);
    glBindTexture(GL_TEXTURE_2D, baseTexture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load("assets/textures/base.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        flipImageVertically(data, width, height, nrChannels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (nrChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        else if (nrChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}

// Renders laptop base
void RenderLaptopBase() {
    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(3.9f, 2.0f, 2.3f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, -2.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // Transforms the camera: move the camera back (z axis)
    //glm::mat4 view = glm::translate(glm::vec3(0.0f, -1.0f, -6.0f));

    //takes in the current camera position
    glm::mat4 view = camera.GetViewMatrix();
    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    //**Updates projection to orthogonal if flag is set
    if (isOrtho == true) {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }
    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, baseTexture);
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(gMesh.vao);

    // Draws pyramid
    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle
    glBindVertexArray(0);
}

// loads vertex, index, and color data into for laptop lid into mesh
void CreateLaptopLid(GLMesh& lidMesh) {
    // Position and Color data
    GLfloat verts[] = {
        // Vertex Positions    
         0.5f,  0.02f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.02f, 0.0f,   1.0f, 0.0f,
        -0.5f, -0.02f, 0.0f,   0.0f, 0.0f,
        -0.5f,  0.02f, 0.0f,   0.0f, 1.0f,
         0.5f, -0.02f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.02f, -1.0f,  1.0f, 0.0f,
        -0.5f,  0.02f, -1.0f,  0.0f, 0.0f,
        -0.5f, -0.02f, -1.0f,  0.0f, 1.0f
    };



    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3, // Triangle 1
        1, 2, 3, // Triangle 2
        0, 1, 4, // Triangle 3
        0, 4, 5, // Triangle 4
        0, 5, 6, // Triangle 5
        0, 3, 6, // Triangle 6
        4, 5, 6, // Triangle 7
        4, 6, 7, // Triangle 8
        2, 3, 6, // Triangle 9
        2, 6, 7, // Triangle 10
        1, 4, 7, // Triangle 11
        1, 2, 7  // Triangle 12
    };



    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerTexture = 2;

    glGenVertexArrays(1, &lidMesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(lidMesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, lidMesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, lidMesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    lidMesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lidMesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);


    glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerTexture));
    glEnableVertexAttribArray(2);

    glGenTextures(1, &lidTexture);
    glBindTexture(GL_TEXTURE_2D, lidTexture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load("assets/textures/lid.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        flipImageVertically(data, width, height, nrChannels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (nrChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        else if (nrChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}

// Renders laptop lid
void RenderLaptopLid() {
    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(3.9f, 2.0f, 2.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(4.6f, glm::vec3(1.0f, 0.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 2.0f, -4.5f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // Transforms the camera: move the camera back (z axis)
    //glm::mat4 view = glm::translate(glm::vec3(0.0f, 1.3f, -6.0f));
    //takes in the current camera postion
    glm::mat4 view = camera.GetViewMatrix();
    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    //**Updates projection to orthogonal if flag is set
    if (isOrtho == true) {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }
    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lidTexture);
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(lidMesh.vao);

    // Draws pyramid
    glDrawElements(GL_TRIANGLES, lidMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle
    glBindVertexArray(0);
}

// loads vertex, index, and color data into for laptop lid into mesh
void CreateTable(GLMesh& tblMesh) {
    // Position and Color data
    GLfloat verts[] = {
        // Vertex Positions          //Textures
         1.0f,  0.04f, 0.0f,      1.0f, 1.0f, // Top-Right Vertex 0
         1.0f, -0.04f, 0.0f,      1.0f, 0.0f, // Bottom-Right Vertex 1
        -1.0f, -0.04f, 0.0f,     0.0f, 0.0f, // Bottom-Left Vertex 2
        -1.0f,  0.04f, 0.0f,     0.0f, 1.0f, // Top-Left Vertex 3

         1.0f, -0.04f, -1.0f,   1.0f, 1.0f, // 4 br  right
         1.0f,  0.04f, -1.0f,   1.0f, 0.0f, //  5 tl  right
        -1.0f,  0.04f, -1.0f,    0.0f, 0.0f, //  6 tl  top
        -1.0f, -0.04f, -1.0f,    0.0f, 1.0f  //  7 bl back
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3, // Triangle 1
        1, 2, 3, // Triangle 2
        0, 1, 4, // Triangle 3
        0, 4, 5, // Triangle 4
        0, 5, 6, // Triangle 5
        0, 3, 6, // Triangle 6
        4, 5, 6, // Triangle 7
        4, 6, 7, // Triangle 8
        2, 3, 6, // Triangle 9
        2, 6, 7, // Triangle 10
        1, 4, 7, // Triangle 11
        1, 2, 7  // Triangle 12
    };

    const GLuint floatsPerVertex = 3;

    const GLuint floatsPerTexture = 2;



    glGenVertexArrays(1, &tblMesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(tblMesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, tblMesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, tblMesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    tblMesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tblMesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);



    glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerTexture));
    glEnableVertexAttribArray(2);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    unsigned char* data = stbi_load("assets/textures/marble.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
}

// Renders laptop lid
void RenderTable() {
    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(8.0f, 2.0f, 10.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(-2.0f, -0.15f, 2.0f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // Transforms the camera: move the camera back (z axis)
    //glm::mat4 view = glm::translate(glm::vec3(0.0f, 1.3f, -6.0f));
    //takes in the current camera postion
    glm::mat4 view = camera.GetViewMatrix();
    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    //**Updates projection to orthogonal if flag is set
    if (isOrtho == true) {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }
    // Set the shader to be used
    glUseProgram(gProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(tblMesh.vao);

    // Draws pyramid
    glDrawElements(GL_TRIANGLES, tblMesh.nIndices, GL_UNSIGNED_SHORT, 0); // Draws the triangle
    glBindVertexArray(0);
}

// loads vertex, index, and color data into for laptop lid into mesh
void CreateLaptopScreen(GLMesh& screenMesh) {

    //// Position and Color data
    //GLfloat verts[] = {
    //    // Vertex Positions    
    //     0.5f,  0.02f, 0.0f,   1.0f, 1.0f,
    //     0.5f, -0.02f, 0.0f,   1.0f, 0.0f,
    //    -0.5f, -0.02f, 0.0f,   0.0f, 0.0f,
    //    -0.5f,  0.02f, 0.0f,   0.0f, 1.0f,
    //     0.5f, -0.02f, -1.0f,  1.0f, 1.0f,
    //     0.5f,  0.02f, -1.0f,  1.0f, 0.0f,
    //    -0.5f,  0.02f, -1.0f,  0.0f, 0.0f,
    //    -0.5f, -0.02f, -1.0f,  0.0f, 1.0f
    //};

        // Position and Color data
    GLfloat verts[] = {
        // Vertex Positions    
         0.5f,  0.02f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.02f, 0.0f,   1.0f, 0.0f,
        -0.5f, -0.02f, 0.0f,   0.0f, 1.0f,
        -0.5f,  0.02f, 0.0f,   0.0f, 1.0f,
         0.5f, -0.02f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.02f, -1.0f,  1.0f, 0.0f,
        -0.5f,  0.02f, -1.0f,  0.0f, 0.0f,
        -0.5f, -0.02f, -1.0f,  0.0f, 1.0f
    };

    // Index data to share position data
    GLushort indices[] = {
        0, 1, 3, // Triangle 1
        3,2,1, // Triangle 2
        0, 1, 4, // Triangle 3
        0, 4, 5, // Triangle 4
        0, 5, 6, // Triangle 5
        0, 3, 6, // Triangle 6
        4, 5, 6, // Triangle 7
        4, 6, 7, // Triangle 8
        2, 3, 6, // Triangle 9
        2, 6, 7, // Triangle 10
        1, 4, 7, // Triangle 11
        1, 2, 7  // Triangle 12
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerTexture = 2;

    glGenVertexArrays(1, &screenMesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(screenMesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, screenMesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, screenMesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    screenMesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screenMesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerTexture);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, floatsPerTexture, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerTexture));
    glEnableVertexAttribArray(2);

    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    
    unsigned char *data = stbi_load("assets/textures/desktop.png", &width, &height, &nrChannels, 0);
    //flipImageVertically(data, width, height, nrChannels);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        std::cout << width << std::endl;
        std::cout << height << std::endl;
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    //Loading second texture
    glGenTextures(1, &desktopTexture);
    glBindTexture(GL_TEXTURE_2D, desktopTexture);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    data = stbi_load("assets/textures/screen.png", &width, &height, &nrChannels, 0);
    //flipImageVertically(data, width, height, nrChannels);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    // Set the shader to be used
    glUseProgram(gProgramId);
    // We set the texture as texture unit 0
    glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 0);
    // We set the texture as texture unit 1
    glUniform1i(glGetUniformLocation(gProgramId, "uExtraTexture"), 1);
}

// Renders laptop lid
void RenderLaptopScreen() {
    // 1. Scales the object by 2
    glm::mat4 scale = glm::scale(glm::vec3(3.89f, 1.99f, 2.0f));
    // 2. Rotates shape by 15 degrees in the x axis
    glm::mat4 rotation = glm::rotate(4.6f, glm::vec3(1.0f, 0.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 translation = glm::translate(glm::vec3(0.0f, 2.0f, -4.49f));
    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = translation * rotation * scale;

    // Transforms the camera: move the camera back (z axis)
    //glm::mat4 view = glm::translate(glm::vec3(0.0f, 1.3f, -6.0f));
    //takes in the current camera postion
    glm::mat4 view = camera.GetViewMatrix();
    // Creates a perspective projection
    glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    //**Updates projection to orthogonal if flag is set
    if (isOrtho == true) {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }
    // Set the shader to be used
    glUseProgram(gProgramId);


    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, desktopTexture);
    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(screenMesh.vao);

    // Draws pyramid
    glDrawElements(GL_TRIANGLES, screenMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle
    glBindVertexArray(0);
}

