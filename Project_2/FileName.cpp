#define SCR_WIDTH 1500
#define SCR_HEIGHT 800

#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <vector>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.hpp"
#include "controls.hpp"
#include "stb_image.h"
#include <tiny_obj_loader.h>

struct Mesh {
    std::vector<float> vertices;
    unsigned int VAO, VBO;
    GLuint textureID;
};

GLFWwindow* window;
GLuint ProgramID;
GLuint MatrixID;
GLuint ModelMatrixID;
GLuint LightPosID;
GLuint LightColorID;
GLuint ObjectColorID;
GLuint TextureID;
GLuint ViewPosID;

GLuint loadTexture(const char* imagepath) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(imagepath, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << imagepath << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum format = (nrChannels == 4) ? GL_RGBA : (nrChannels == 3) ? GL_RGB : GL_RED;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return textureID;
}

bool LoadObjModel(const std::string& path, Mesh& mesh) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), NULL)) {
        std::cerr << "Failed to load .obj file: " << err << std::endl;
        return false;
    }

    mesh.vertices.clear();

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];
            mesh.vertices.push_back(vx);
            mesh.vertices.push_back(vy);
            mesh.vertices.push_back(vz);

            if (!attrib.normals.empty() && index.normal_index >= 0) {
                float nx = attrib.normals[3 * index.normal_index + 0];
                float ny = attrib.normals[3 * index.normal_index + 1];
                float nz = attrib.normals[3 * index.normal_index + 2];
                mesh.vertices.push_back(nx);
                mesh.vertices.push_back(ny);
                mesh.vertices.push_back(nz);
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(1.0f);
            }

            if (!attrib.texcoords.empty() && index.texcoord_index >= 0) {
                float u = attrib.texcoords[2 * index.texcoord_index + 0];
                float v = attrib.texcoords[2 * index.texcoord_index + 1];
                mesh.vertices.push_back(u);
                mesh.vertices.push_back(1.0f - v);
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
            }
        }
    }

    mesh.textureID = 0;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), &mesh.vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return true;
}

void createBoxMesh(Mesh& mesh) {
    float boxVertices[] = {
        -2000.0f, -2000.0f, -2000.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         2000.0f, -2000.0f, -2000.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         2000.0f,  2000.0f, -2000.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         2000.0f,  2000.0f, -2000.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -2000.0f,  2000.0f, -2000.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -2000.0f, -2000.0f, -2000.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

        -2000.0f, -2000.0f,  2000.0f,   0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
         2000.0f, -2000.0f,  2000.0f,   0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
         2000.0f,  2000.0f,  2000.0f,   0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
         2000.0f,  2000.0f,  2000.0f,   0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
        -2000.0f,  2000.0f,  2000.0f,   0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
        -2000.0f, -2000.0f,  2000.0f,   0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

        -2000.0f,  2000.0f,  2000.0f, -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        -2000.0f,  2000.0f, -2000.0f, -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        -2000.0f, -2000.0f, -2000.0f, -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -2000.0f, -2000.0f, -2000.0f, -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
        -2000.0f, -2000.0f,  2000.0f, -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        -2000.0f,  2000.0f,  2000.0f, -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

         2000.0f,  2000.0f,  2000.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         2000.0f,  2000.0f, -2000.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         2000.0f, -2000.0f, -2000.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         2000.0f, -2000.0f, -2000.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         2000.0f, -2000.0f,  2000.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         2000.0f,  2000.0f,  2000.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

        -2000.0f, -2000.0f, -2000.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
         2000.0f, -2000.0f, -2000.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
         2000.0f, -2000.0f,  2000.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         2000.0f, -2000.0f,  2000.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -2000.0f, -2000.0f,  2000.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        -2000.0f, -2000.0f, -2000.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        -2000.0f,  2000.0f, -2000.0f,   0.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         2000.0f,  2000.0f, -2000.0f,   0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
         2000.0f,  2000.0f,  2000.0f,   0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
         2000.0f,  2000.0f,  2000.0f,   0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -2000.0f,  2000.0f,  2000.0f,   0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        -2000.0f,  2000.0f, -2000.0f,   0.0f, -1.0f, 0.0f, 0.0f, 1.0f
    };

    mesh.vertices.assign(boxVertices, boxVertices + sizeof(boxVertices) / sizeof(float));

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), &mesh.vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void cleanupDataFromGPU(Mesh& Obj) {
    glDeleteVertexArrays(1, &Obj.VAO);
    glDeleteBuffers(1, &Obj.VBO);
    if (Obj.textureID != 0) {
        glDeleteTextures(1, &Obj.textureID);
    }
}

void initGLFWandGLEW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Object Rendering", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to open GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
}

int main(void) {
    initGLFWandGLEW();

    ProgramID = LoadShaders(
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\x64\\Debug\\TransformVertexShader.vertexshader",
        "C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\x64\\Debug\\TextureFragmentShader.fragmentshader");

    if (ProgramID == 0) {
        std::cerr << "Error loading shaders!" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    MatrixID = glGetUniformLocation(ProgramID, "MVP");
    ModelMatrixID = glGetUniformLocation(ProgramID, "Model");
    LightPosID = glGetUniformLocation(ProgramID, "lightPos");
    LightColorID = glGetUniformLocation(ProgramID, "lightColor");
    ObjectColorID = glGetUniformLocation(ProgramID, "objectColor");
    TextureID = glGetUniformLocation(ProgramID, "textureSampler");
    ViewPosID = glGetUniformLocation(ProgramID, "viewPos");

    Mesh Obj1, Obj2, Obj3, Obj4, backgroundBox;

    LoadObjModel("C:\\Users\\bruno\\source\\repos\\KJBruninho\\3D_Object_Rendering\\Project_2\\hangar.obj", Obj1);
    LoadObjModel("C:\\Users\\bruno\\source\\repos\\KJBruninho\\3D_Object_Rendering\\Project_2\\hangar.obj", Obj2);
    LoadObjModel("C:\\Users\\bruno\\source\\repos\\KJBruninho\\3D_Object_Rendering\\Project_2\\ball.obj", Obj3);
    LoadObjModel("C:\\Users\\bruno\\source\\repos\\KJBruninho\\3D_Object_Rendering\\Project_2\\Millennium_Falcon.obj", Obj4);

    GLuint hangarTexture = loadTexture("C:\\Users\\bruno\\source\\repos\\KJBruninho\\3D_Object_Rendering\\Project_2\\metal.jpg");
    GLuint ballTexture = loadTexture("C:\\Users\\bruno\\source\\repos\\KJBruninho\\3D_Object_Rendering\\Project_2\\ball.jpg");
    GLuint spaceTexture = loadTexture("C:\\Users\\bruno\\source\\repos\\KJBruninho\\3D_Object_Rendering\\Project_2\\sky.jpg");

    Obj1.textureID = hangarTexture;
    Obj2.textureID = hangarTexture;
    Obj3.textureID = ballTexture;
    Obj4.textureID = ballTexture;

    createBoxMesh(backgroundBox);
    backgroundBox.textureID = spaceTexture;

    float scaleFactor = 10.0f;
    float falconPositionX = 1000.0f;
    float ballFPositionX = 100.0f;
    bool falconMoving = true;
    bool ballMoving = true;
    float startTime = (float)glfwGetTime();
    float waitDuration = 2.0f;

    // CORRIGIDO: Luz bem mais perto dos objetos e MÚLTIPLAS LUZES simuladas
    glm::vec3 lightPos(500.0f, 300.0f, 200.0f);  // Luz principal mais próxima
    glm::vec3 lightColor(2.0f, 2.0f, 2.0f);      // Luz 2x mais intensa!
    glm::vec3 objectColor(1.0f, 1.0f, 1.0f);

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();
        glm::vec3 cameraPos = getCameraPosition();

        glUseProgram(ProgramID);

        // Enviar uniforms de luz
        glUniform3fv(LightPosID, 1, glm::value_ptr(lightPos));
        glUniform3fv(LightColorID, 1, glm::value_ptr(lightColor));
        glUniform3fv(ObjectColorID, 1, glm::value_ptr(objectColor));
        glUniform3fv(ViewPosID, 1, glm::value_ptr(cameraPos));

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(TextureID, 0);

        // 1. Skybox
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        glBindTexture(GL_TEXTURE_2D, backgroundBox.textureID);
        glm::mat4 ModelMatrixBox = glm::mat4(1.0f);
        glm::mat4 MVPBox = ProjectionMatrix * ViewMatrix * ModelMatrixBox;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(MVPBox));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(ModelMatrixBox));
        glBindVertexArray(backgroundBox.VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);

        // 2. Hangar 1
        glBindTexture(GL_TEXTURE_2D, Obj1.textureID);
        glm::mat4 ModelMatrix1 = glm::mat4(1.0f);
        glm::mat4 MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix1;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(MVP1));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(ModelMatrix1));
        glBindVertexArray(Obj1.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj1.vertices.size() / 8);

        // 3. Hangar 2
        glBindTexture(GL_TEXTURE_2D, Obj2.textureID);
        glm::mat4 ModelMatrix2 = glm::translate(glm::mat4(1.0f), glm::vec3(1000.0f, 0.0f, 0.0f));
        ModelMatrix2 = glm::rotate(ModelMatrix2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 MVP2 = ProjectionMatrix * ViewMatrix * ModelMatrix2;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(MVP2));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(ModelMatrix2));
        glBindVertexArray(Obj2.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj2.vertices.size() / 8);

        // 4. Millennium Falcon
        glBindTexture(GL_TEXTURE_2D, Obj3.textureID);
        glm::mat4 ModelMatrix3 = glm::translate(glm::mat4(1.0f), glm::vec3(falconPositionX, 20.0f, 35.0f));
        ModelMatrix3 = glm::scale(ModelMatrix3, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
        glm::mat4 MVP3 = ProjectionMatrix * ViewMatrix * ModelMatrix3;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(MVP3));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(ModelMatrix3));
        glBindVertexArray(Obj3.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj3.vertices.size() / 8);

        // 5. Ball Fighter
        glBindTexture(GL_TEXTURE_2D, Obj4.textureID);
        glm::mat4 ModelMatrix4 = glm::translate(glm::mat4(1.0f), glm::vec3(ballFPositionX, 20.0f, -35.0f));
        ModelMatrix4 = glm::rotate(ModelMatrix4, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix4 = glm::scale(ModelMatrix4, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
        glm::mat4 MVP4 = ProjectionMatrix * ViewMatrix * ModelMatrix4;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(MVP4));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(ModelMatrix4));
        glBindVertexArray(Obj4.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj4.vertices.size() / 8);

        float currentTime = (float)glfwGetTime();
        if (currentTime - startTime >= waitDuration) {
            if (falconMoving) {
                falconPositionX -= 0.5f;
                if (falconPositionX <= 0.0f) falconMoving = false;
            }
            if (ballMoving) {
                ballFPositionX += 0.5f;
                if (ballFPositionX >= 1000.0f) ballMoving = false;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanupDataFromGPU(Obj1);
    cleanupDataFromGPU(Obj2);
    cleanupDataFromGPU(Obj3);
    cleanupDataFromGPU(Obj4);
    cleanupDataFromGPU(backgroundBox);
    glDeleteProgram(ProgramID);
    glfwTerminate();
    return 0;
}