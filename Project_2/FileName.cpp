#define SCR_WIDTH 2500
#define SCR_HEIGHT 1500

// Includes
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <tiny_obj_loader.h>

struct Mesh {
    std::vector<float> vertices;
    unsigned int VAO, VBO;
};

// Global Variables
GLFWwindow* window;
GLuint ProgramID;
GLuint MatrixID;


bool LoadObjModel(const std::string& path, Mesh& mesh, const std::string& mtlBasePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), mtlBasePath.c_str())) {
        std::cerr << "Failed to load .obj file: " << err << std::endl;
        return false;
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];
            mesh.vertices.push_back(vx);
            mesh.vertices.push_back(vy);
            mesh.vertices.push_back(vz);

            if (!attrib.normals.empty()) {
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
                mesh.vertices.push_back(1.0f - v); // Flip V-coordinate
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
            }
        }
    }

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

// Functions to load models
void transferDataToGPUMemory(Mesh& Obj, const std::string& obj, const std::string& objmtl) {
    ProgramID = LoadShaders(
        "C:\\TransformVertexShader.vertexshader",
        "C:\\TextureFragmentShader.fragmentshader");

    if (ProgramID == 0) {
        std::cerr << "Error loading shaders!" << std::endl;
        exit(EXIT_FAILURE);
    }

    MatrixID = glGetUniformLocation(ProgramID, "MVP");
    loadDDS("nave.DDS");

    if (!LoadObjModel(obj, Obj, objmtl)) {
        std::cerr << "Error loading OBJ model!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void cleanupDataFromGPU(Mesh& Obj) {
    glDeleteVertexArrays(1, &Obj.VAO);
    glDeleteBuffers(1, &Obj.VBO);
    glDeleteProgram(ProgramID);
}

int main(void) {
    // GLFW and GLEW initialization
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Object Rendering", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, SCR_WIDTH / 2, SCR_HEIGHT / 2);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

   
    Mesh Obj1, Obj2, Obj3, Obj4;
    transferDataToGPUMemory(Obj1, "Hangar.obj", "hangar.mtl");              // Carregar o primeiro hangar
    transferDataToGPUMemory(Obj2, "Hangar.obj", "hangar.mtl");              // Carregar o segundo hangar
    transferDataToGPUMemory(Obj3, "Millennium_Falcon.obj", "hangar.mtl");   // Carregar o falcon
    transferDataToGPUMemory(Obj4, "Ball_fighter.obj", "hangar.mtl");        // Carregar a ball_F
    
    float scaleFactor = 5.0f;         // Aumenta a escala das naves
    float falconPositionX = 1000.0f;  // Posição inicial do Millennium Falcon
    float ballFPositionX = 100.0f;    // Posição inicial da Ball Fighter
    bool falconMoving = true;         // Controle para movimento do Falcon
    bool ballMoving = true;           // Controle para movimento da Ball Fighter

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(ProgramID);


        computeMatricesFromInputs();
        glm::mat4 ProjectionMatrix = getProjectionMatrix();
        glm::mat4 ViewMatrix = getViewMatrix();

        // Variáveis de iluminação
        glm::vec3 lightPos(5.0f, 5.0f, 5.0f);  // Posição da fonte de luz
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);   // Cor da luz branca
        glm::vec3 objectColor(0.70f, 1.0f, 1.0f);  // Cor do objeto (cinza)


        glUniform3fv(glGetUniformLocation(ProgramID, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(ProgramID, "lightColor"), 1, &lightColor[0]);
        glUniform3fv(glGetUniformLocation(ProgramID, "objectColor"), 1, &objectColor[0]);


        // Renderizar o primeiro hangar (objeto 1)
        glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 1);
        glm::mat4 ModelMatrix1 = glm::mat4(1.0f);
        glm::mat4 MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix1;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
        glBindVertexArray(Obj1.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj1.vertices.size() / 8);
        glBindVertexArray(0);

        // Renderizar o segundo hangar (objeto 2)
        glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 2);
        glm::mat4 ModelMatrix2 = glm::translate(glm::mat4(1.0f), glm::vec3(1000.0f, 0.0f, 0.0f));
        ModelMatrix2 = glm::rotate(ModelMatrix2, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 MVP2 = ProjectionMatrix * ViewMatrix * ModelMatrix2;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP2[0][0]);
        glBindVertexArray(Obj2.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj2.vertices.size() / 8);
        glBindVertexArray(0);

        // Renderizar o Millennium Falcon (objeto 3)
        glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 3);
        glm::mat4 ModelMatrix3 = glm::translate(glm::mat4(1.0f), glm::vec3(falconPositionX, 20.0f, 35.0f));
        ModelMatrix3 = glm::rotate(ModelMatrix3, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ModelMatrix3 = glm::scale(ModelMatrix3, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
        glm::mat4 MVP3 = ProjectionMatrix * ViewMatrix * ModelMatrix3;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP3[0][0]);
        glBindVertexArray(Obj3.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj3.vertices.size() / 8);
        glBindVertexArray(0);

        // Renderizar a Ball Fighter (objeto 4)
        glUniform1i(glGetUniformLocation(ProgramID, "objectID"), 4);
        glm::mat4 ModelMatrix4 = glm::translate(glm::mat4(1.0f), glm::vec3(ballFPositionX, 20.0f, -35.0f));
        ModelMatrix4 = glm::scale(ModelMatrix4, glm::vec3(scaleFactor, scaleFactor, scaleFactor));
        glm::mat4 MVP4 = ProjectionMatrix * ViewMatrix * ModelMatrix4;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP4[0][0]);
        glBindVertexArray(Obj4.VAO);
        glDrawArrays(GL_TRIANGLES, 0, Obj4.vertices.size() / 8);
        glBindVertexArray(0);


        // Atualizar posição do Millennium Falcon
        if (falconMoving) {
            falconPositionX -= 1.0f;  // Diminui a posição X
            if (falconPositionX <= 0.0f) falconMoving = false; // Para o movimento ao chegar na posição final
        }

        // Atualizar posição da Ball Fighter
        if (ballMoving) {
            ballFPositionX += 1.0f;  // Aumenta a posição X
            if (ballFPositionX >= 1000.0f) ballMoving = false; // Para o movimento ao chegar na posição final
        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }



    cleanupDataFromGPU(Obj1);
    cleanupDataFromGPU(Obj2);
    cleanupDataFromGPU(Obj3);
    cleanupDataFromGPU(Obj4);

    glfwTerminate();
    return 0;
}
