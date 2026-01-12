#define SCR_WIDTH 1500
#define SCR_HEIGHT 800

#define GLM_ENABLE_EXPERIMENTAL
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <random>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers do seu projeto
#include "shader.hpp" 
#include "controls.hpp" 
#include "stb_image.h"
#include <tiny_obj_loader.h>

struct Mesh {
    std::vector<float> vertices;
    unsigned int VAO, VBO;
    GLuint textureID;
    size_t vertexCount;
};

struct Ball {
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    float lifetime;
    bool isEnemyBall;
    bool hasCollided;
};

struct Enemy {
    glm::vec3 position;
    glm::vec3 velocity;
    float shootTimer;
    bool active;
    float radius; // Hitbox aumentada
    float angle;
    float targetTimer;
    glm::vec3 targetPoint;
    int type; // 0: basic, 1: fast, 2: tank, 3: shooter
    glm::vec3 color;
    int health;
    float scale;
    float speedMultiplier;
};

// Variáveis Globais
GLFWwindow* window;
GLuint ProgramID, MatrixID, LightPosID, LightColorID, ObjectColorID, TextureID, ModelMatrixID, IsSkyboxID;
glm::vec3 position;
std::list<Ball> activeBalls;
std::vector<Enemy> enemies;
int playerHealth = 100;
int maxEnemies = 10;
int kills = 0;
bool gameOver = false;

// Constantes ajustadas
float ballSpeed = 800.0f; // AUMENTADO: Tiros mais rápidos
float ballLifetime = 5.0f;
float enemyShootInterval = 2.0f;
float spawnRadius = 200.0f;
float enemyBallSpeed = 100.0f; // AUMENTADO: Tiros inimigos mais rápidos
float playerRadius = 5.0f;
float ballRadius = 2.0f; // DIMINUÍDO: Tamanho dos projéteis reduzido
float enemyBaseRadius = 10.0f; // AUMENTADO: Hitbox maior
float enemySpeed = 15.0f;
float minDistanceToPlayer = 50.0f;
float maxDistanceToPlayer = 200.0f;
float targetChangeTime = 3.0f;
bool leftMousePressed = false;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dis(0.0f, 1.0f);

float randomFloat(float min, float max) {
    return min + dis(gen) * (max - min);
}

// Função para carregar imagens e criar texturas OpenGL
GLuint loadTexture(const char* imagepath, bool isSkybox = false) {
    int width, height, nrChannels;

    // Flip verticalmente para OpenGL
    stbi_set_flip_vertically_on_load(!isSkybox);

    unsigned char* data = stbi_load(imagepath, &width, &height, &nrChannels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << imagepath << std::endl;
        std::cerr << "Error: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum format;
    if (nrChannels == 1)
        format = GL_RED;
    else if (nrChannels == 3)
        format = GL_RGB;
    else if (nrChannels == 4)
        format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (isSkybox) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    stbi_set_flip_vertically_on_load(false);

    return textureID;
}

// Carrega arquivos .obj usando tiny_obj_loader
bool LoadObjModel(const std::string& path, Mesh& mesh) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), NULL, true, true)) {
        std::cerr << "Failed to load .obj file: " << path << std::endl;
        if (!warn.empty()) std::cerr << "Warning: " << warn << std::endl;
        if (!err.empty()) std::cerr << "Error: " << err << std::endl;
        return false;
    }

    if (!warn.empty()) {
        std::cout << "Warning when loading " << path << ": " << warn << std::endl;
    }

    mesh.vertices.clear();
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            // Posições
            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];
            mesh.vertices.push_back(vx);
            mesh.vertices.push_back(vy);
            mesh.vertices.push_back(vz);

            // Normais
            if (index.normal_index >= 0) {
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

            // UVs
            if (index.texcoord_index >= 0) {
                float u = attrib.texcoords[2 * index.texcoord_index + 0];
                float v = attrib.texcoords[2 * index.texcoord_index + 1];
                mesh.vertices.push_back(u);
                mesh.vertices.push_back(v);
            }
            else {
                mesh.vertices.push_back(0.0f);
                mesh.vertices.push_back(0.0f);
            }
        }
    }

    mesh.vertexCount = mesh.vertices.size() / 8;
    std::cout << "Loaded " << path << " with " << mesh.vertexCount << " vertices" << std::endl;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);

    // Layout: Pos(3), Normal(3), UV(2) = 8 floats total
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return true;
}

// Cria um cubemap para skybox (6 faces diferentes)
void createSkyboxMesh(Mesh& mesh) {
    // Vertices para um cubo (posições normalizadas)
    float skyboxVertices[] = {
        // Back face
        -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
        -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
         1.0f, -1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
         1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,

        // Front face
        -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

        // Right face
         1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         1.0f, -1.0f,  1.0f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         1.0f,  1.0f, -1.0f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

         // Left face
         -1.0f, -1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         -1.0f,  1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
          1.0f,  1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
          1.0f,  1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
          1.0f, -1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         -1.0f, -1.0f,  1.0f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,

         // Top face
         -1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
          1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
          1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
          1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         -1.0f,  1.0f,  1.0f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         -1.0f,  1.0f, -1.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

         // Bottom face
         -1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
          1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
          1.0f, -1.0f, -1.0f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
         -1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
          1.0f, -1.0f,  1.0f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f
    };

    mesh.vertices.assign(skyboxVertices, skyboxVertices + sizeof(skyboxVertices) / sizeof(float));
    mesh.vertexCount = mesh.vertices.size() / 8;

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(float), mesh.vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void cleanupDataFromGPU(Mesh& Obj) {
    if (Obj.VAO != 0) glDeleteVertexArrays(1, &Obj.VAO);
    if (Obj.VBO != 0) glDeleteBuffers(1, &Obj.VBO);
    if (Obj.textureID != 0) glDeleteTextures(1, &Obj.textureID);
}

// Verifica colisão entre esferas
bool checkSphereCollision(const glm::vec3& pos1, float radius1, const glm::vec3& pos2, float radius2) {
    float distance = glm::length(pos1 - pos2);
    return distance < (radius1 + radius2);
}

// Gera ponto de destino aleatório para inimigos
glm::vec3 generateTargetPoint(const glm::vec3& playerPos, const glm::vec3& enemyPos) {
    float angle = randomFloat(0, 2.0f * 3.14159f);
    float distance = randomFloat(minDistanceToPlayer, maxDistanceToPlayer);

    return playerPos + glm::vec3(
        distance * cos(angle),
        randomFloat(-20.0f, 20.0f),
        distance * sin(angle)
    );
}

// Cria um inimigo
Enemy createEnemy(int type) {
    Enemy enemy;
    float angle = randomFloat(0, 2.0f * 3.14159f);

    enemy.position = glm::vec3(
        spawnRadius * cos(angle),
        randomFloat(-spawnRadius * 0.5f, spawnRadius * 0.5f),
        spawnRadius * sin(angle)
    );

    enemy.type = type;
    enemy.active = true;
    enemy.targetTimer = randomFloat(0, targetChangeTime);
    enemy.targetPoint = generateTargetPoint(position, enemy.position);
    enemy.shootTimer = randomFloat(0, enemyShootInterval);
    enemy.velocity = glm::vec3(0.0f);

    switch (type) {
    case 0: // Basic
        enemy.radius = enemyBaseRadius * 1.0f;
        enemy.health = 1;
        enemy.color = glm::vec3(0.5f, 0.5f, 1.0f);
        enemy.scale = 2.0f;
        enemy.speedMultiplier = 1.0f;
        break;
    case 1: // Fast
        enemy.radius = enemyBaseRadius * 0.8f;
        enemy.health = 1;
        enemy.color = glm::vec3(1.0f, 1.0f, 0.0f);
        enemy.scale = 1.5f;
        enemy.speedMultiplier = 2.0f;
        break;
    case 2: // Tank
        enemy.radius = enemyBaseRadius * 2.0f;
        enemy.health = 3;
        enemy.color = glm::vec3(1.0f, 0.3f, 0.3f);
        enemy.scale = 3.0f;
        enemy.speedMultiplier = 0.5f;
        break;
    case 3: // Shooter
        enemy.radius = enemyBaseRadius * 1.2f;
        enemy.health = 2;
        enemy.color = glm::vec3(0.0f, 1.0f, 0.0f);
        enemy.scale = 2.2f;
        enemy.speedMultiplier = 0.8f;
        break;
    }

    return enemy;
}

// Inicializa inimigos
void initEnemies() {
    enemies.clear();
    for (int i = 0; i < maxEnemies; i++) {
        int type = i % 4; // Distribui tipos
        enemies.push_back(createEnemy(type));
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

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Space Invaders 3D", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void checkGLError(const char* location) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error at " << location << ": " << std::hex << err << std::dec << std::endl;
    }
}

int main(void) {
    initGLFWandGLEW();

    // Carregar shaders
    ProgramID = LoadShaders(
        "TransformVertexShader.vertexshader",
        "TextureFragmentShader.fragmentshader"
    );

    if (ProgramID == 0) {
        std::cerr << "Failed to load shaders!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glUseProgram(ProgramID);

    // Obter localizações dos uniforms
    MatrixID = glGetUniformLocation(ProgramID, "MVP");
    ModelMatrixID = glGetUniformLocation(ProgramID, "ModelMatrix");
    LightPosID = glGetUniformLocation(ProgramID, "lightPos");
    LightColorID = glGetUniformLocation(ProgramID, "lightColor");
    ObjectColorID = glGetUniformLocation(ProgramID, "objectColor");
    TextureID = glGetUniformLocation(ProgramID, "textureSampler");
    IsSkyboxID = glGetUniformLocation(ProgramID, "isSkybox");

    // Configuração de luz
    glUniform3f(LightPosID, 0.0f, 100.0f, 0.0f);
    glUniform3f(LightColorID, 1.0f, 1.0f, 1.0f);

    // Malhas e Texturas
    Mesh playerShip, enemyShip, projectile, skybox;

    // Carregar modelos
    std::cout << "\nLoading models..." << std::endl;

    if (!LoadObjModel("C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\Millennium_Falcon.obj", playerShip)) {
        std::cerr << "Failed to load Millennium_Falcon.obj" << std::endl;
        return -1;
    }

    if (!LoadObjModel("C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\mf.obj", enemyShip)) {
        std::cerr << "Failed to load enemy ship.obj" << std::endl;
        return -1;
    }

    if (!LoadObjModel("C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\fireball.obj", projectile)) {
        std::cerr << "Failed to load projectile.obj" << std::endl;
        return -1;
    }

    // Criar skybox
    std::cout << "Creating skybox..." << std::endl;
    createSkyboxMesh(skybox);

    // Carregar texturas
    std::cout << "\nLoading textures..." << std::endl;

    playerShip.textureID = loadTexture("C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\nave.DDS");
    enemyShip.textureID = loadTexture("C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\inimigo.DDS");
    projectile.textureID = loadTexture("C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\metal2.DDS");
    skybox.textureID = loadTexture("C:\\Users\\bruno\\source\\repos\\KJBruninho\\SpaceInvaders_3D\\P2\\DDS\\sky.jpg", true);

    if (playerShip.textureID == 0 || enemyShip.textureID == 0 || projectile.textureID == 0) {
        std::cerr << "Failed to load one or more textures" << std::endl;
        return -1;
    }

    // Inicializar inimigos
    initEnemies();

    // Configurar posição inicial do jogador
    position = glm::vec3(0.0f, 0.0f, 0.0f);

    // Variáveis para controle de tempo
    float lastTime = glfwGetTime();
    float deltaTime = 0.0f;

    std::cout << "\nStarting game loop..." << std::endl;

    // Loop principal do jogo
    while (!gameOver && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        // Calcular delta time
        float currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Limpar buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Atualizar controles
        computeMatricesFromInputs();
        glm::mat4 Projection = getProjectionMatrix();
        glm::mat4 View = getViewMatrix();

        // Usar programa de shaders
        glUseProgram(ProgramID);

        // ========== RENDERIZAR SKYBOX ==========
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        // Matriz de view sem translação para skybox
        glm::mat4 skyboxView = glm::mat4(glm::mat3(View));
        glm::mat4 skyboxModel = glm::scale(glm::mat4(1.0f), glm::vec3(1000.0f)); // Skybox grande
        glm::mat4 skyboxMVP = Projection * skyboxView * skyboxModel;

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(skyboxMVP));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(skyboxModel));
        glUniform1i(IsSkyboxID, 1);
        glUniform3f(ObjectColorID, 1.0f, 1.0f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skybox.textureID);
        glUniform1i(TextureID, 0);

        glBindVertexArray(skybox.VAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)skybox.vertexCount);

        // Restaurar estado para objetos normais
        glDepthMask(GL_TRUE);
        glEnable(GL_CULL_FACE);
        glUniform1i(IsSkyboxID, 0);

        // ========== ATUALIZAR E RENDERIZAR INIMIGOS ==========
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;

            // Atualizar posição
            enemy.targetTimer -= deltaTime;
            if (enemy.targetTimer <= 0.0f) {
                enemy.targetPoint = generateTargetPoint(position, enemy.position);
                enemy.targetTimer = targetChangeTime;
            }

            glm::vec3 direction = glm::normalize(enemy.targetPoint - enemy.position);
            enemy.position += direction * enemySpeed * enemy.speedMultiplier * deltaTime;

            // Atirar
            enemy.shootTimer -= deltaTime;
            if (enemy.shootTimer <= 0.0f) {
                Ball enemyBall;
                enemyBall.position = enemy.position;
                enemyBall.direction = glm::normalize(position - enemy.position);
                enemyBall.speed = enemyBallSpeed;
                enemyBall.lifetime = ballLifetime;
                enemyBall.isEnemyBall = true;
                enemyBall.hasCollided = false;
                activeBalls.push_back(enemyBall);
                enemy.shootTimer = enemyShootInterval;
            }

            // Renderizar inimigo
            glm::mat4 enemyModel = glm::translate(glm::mat4(1.0f), enemy.position);
            enemyModel = glm::rotate(enemyModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            enemyModel = glm::scale(enemyModel, glm::vec3(enemy.scale));

            glm::mat4 enemyMVP = Projection * View * enemyModel;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(enemyMVP));
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(enemyModel));
            glUniform3fv(ObjectColorID, 1, glm::value_ptr(enemy.color));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, enemyShip.textureID);
            glUniform1i(TextureID, 0);

            glBindVertexArray(enemyShip.VAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)enemyShip.vertexCount);
        }

        // ========== RENDERIZAR NAVE DO JOGADOR ==========
        glm::vec3 cameraForward = glm::vec3(View[0][2], View[1][2], View[2][2]);
        glm::vec3 cameraUp = glm::vec3(View[0][1], View[1][1], View[2][1]);
        glm::vec3 cameraRight = glm::cross(cameraForward, cameraUp);

        // Posição da nave: um pouco à frente da câmera
        glm::vec3 shipPosition = position - cameraForward * 5.0f + cameraUp * 1.0f;

        glm::mat4 shipModel = glm::translate(glm::mat4(1.0f), shipPosition);

        // Alinhar a nave com a câmera
        glm::mat4 rotationMatrix(1.0f);
        rotationMatrix[0] = glm::vec4(cameraRight, 0.0f);
        rotationMatrix[1] = glm::vec4(cameraUp, 0.0f);
        rotationMatrix[2] = glm::vec4(-cameraForward, 0.0f);
        shipModel = shipModel * rotationMatrix;

        shipModel = glm::scale(shipModel, glm::vec3(0.2f)); // Escala adequada

        glm::mat4 shipMVP = Projection * View * shipModel;

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(shipMVP));
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(shipModel));
        glUniform3f(ObjectColorID, 0.8f, 0.9f, 1.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, playerShip.textureID);
        glUniform1i(TextureID, 0);

        glBindVertexArray(playerShip.VAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)playerShip.vertexCount);

        // ========== ATUALIZAR E RENDERIZAR PROJÉTEIS ==========
        // Controle do mouse para atirar
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !leftMousePressed) {
            leftMousePressed = true;

            Ball newBall;
            newBall.position = position - cameraForward * 3.0f;
            newBall.direction = -cameraForward;
            newBall.speed = ballSpeed; // Tiros mais rápidos
            newBall.lifetime = ballLifetime;
            newBall.isEnemyBall = false;
            newBall.hasCollided = false;
            activeBalls.push_back(newBall);
        }
        else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
            leftMousePressed = false;
        }

        // Atualizar e renderizar projéteis
        auto ballIt = activeBalls.begin();
        while (ballIt != activeBalls.end()) {
            ballIt->position += ballIt->direction * ballIt->speed * deltaTime;
            ballIt->lifetime -= deltaTime;

            // Verificar colisões
            if (ballIt->isEnemyBall) {
                if (checkSphereCollision(ballIt->position, ballRadius, position, playerRadius)) {
                    playerHealth -= 10;
                    std::cout << "Player hit! Health: " << playerHealth << std::endl;
                    if (playerHealth <= 0) {
                        gameOver = true;
                        std::cout << "Game Over!" << std::endl;
                    }
                    ballIt = activeBalls.erase(ballIt);
                    continue;
                }
            }
            else {
                for (auto& enemy : enemies) {
                    if (enemy.active && checkSphereCollision(ballIt->position, ballRadius, enemy.position, enemy.radius)) {
                        enemy.health--;
                        if (enemy.health <= 0) {
                            enemy.active = false;
                            kills++;
                            std::cout << "Enemy destroyed! Kills: " << kills << std::endl;
                        }
                        ballIt = activeBalls.erase(ballIt);
                        goto nextBall;
                    }
                }
            }

            // Remover projéteis com tempo esgotado
            if (ballIt->lifetime <= 0.0f) {
                ballIt = activeBalls.erase(ballIt);
                continue;
            }

            // Renderizar projétil
            glm::mat4 ballModel = glm::translate(glm::mat4(1.0f), ballIt->position);
            ballModel = glm::scale(ballModel, glm::vec3(0.3f)); // Projéteis menores

            glm::mat4 ballMVP = Projection * View * ballModel;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, glm::value_ptr(ballMVP));
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, glm::value_ptr(ballModel));

            if (ballIt->isEnemyBall) {
                glUniform3f(ObjectColorID, 1.0f, 0.2f, 0.2f);
            }
            else {
                glUniform3f(ObjectColorID, 0.2f, 0.2f, 1.0f);
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, projectile.textureID);
            glUniform1i(TextureID, 0);

            glBindVertexArray(projectile.VAO);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)projectile.vertexCount);

            ++ballIt;
        nextBall:;
        }

        // ========== MOSTRAR INFORMAÇÕES DO JOGO ==========
        std::cout << "\rHealth: " << playerHealth << " | Kills: " << kills << " | Active Balls: " << activeBalls.size() << "     " << std::flush;

        // Trocar buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    std::cout << "\n\nCleaning up..." << std::endl;
    cleanupDataFromGPU(playerShip);
    cleanupDataFromGPU(enemyShip);
    cleanupDataFromGPU(projectile);
    cleanupDataFromGPU(skybox);
    glDeleteProgram(ProgramID);
    glfwTerminate();

    std::cout << "Game Over! Final Score: " << kills << std::endl;
    return 0;
}