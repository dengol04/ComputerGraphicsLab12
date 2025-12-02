#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <iostream>
#include <cmath>
#include <cstring>
#include <string>

#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif

const float PI = 3.14159265359f;

class Matrix4
{
public:
    float m[16];

    Matrix4() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Matrix4 Identity() {
        return Matrix4();
    }

    Matrix4 operator*(const Matrix4& o) const {
        Matrix4 r;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k)
                    sum += m[k * 4 + row] * o.m[col * 4 + k];
                r.m[col * 4 + row] = sum;
            }
        return r;
    }

    static Matrix4 Rotate(const Matrix4& mat, float angle, float x, float y, float z) {
        float rad = angle * PI / 180.0f;
        float c = cosf(rad);
        float s = sinf(rad);

        float len = sqrtf(x * x + y * y + z * z);
        if (len < 1e-6f) return mat;

        x /= len; y /= len; z /= len;

        Matrix4 r;
        r.m[0] = x * x * (1 - c) + c;
        r.m[1] = y * x * (1 - c) + z * s;
        r.m[2] = z * x * (1 - c) - y * s;

        r.m[4] = x * y * (1 - c) - z * s;
        r.m[5] = y * y * (1 - c) + c;
        r.m[6] = z * y * (1 - c) + x * s;

        r.m[8] = x * z * (1 - c) + y * s;
        r.m[9] = y * z * (1 - c) - x * s;
        r.m[10] = z * z * (1 - c) + c;

        return mat * r;
    }

    static Matrix4 Perspective(float fov, float aspect, float zNear, float zFar) {
        Matrix4 r;
        float tanHalf = tanf(fov * PI / 360.0f);
        float range = zNear - zFar;

        r.m[0] = 1.0f / (aspect * tanHalf);
        r.m[5] = 1.0f / tanHalf;
        r.m[10] = (zFar + zNear) / range;
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * zFar * zNear) / range;
        r.m[15] = 0.0f;

        return r;
    }

    static Matrix4 LookAt(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ) {
        return Matrix4::Identity();
    }
};

const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    ourColor = aColor;\n"
"}\n";

const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 ourColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(ourColor, 1.0f);\n"
"}\n";

class Shader
{
public:
    unsigned int ID;

    Shader(const char* vShaderCode, const char* fShaderCode)
    {
        unsigned int vertex, fragment;
        int success;
        char infoLog[512];

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success) { glGetShaderInfoLog(vertex, 512, NULL, infoLog); std::cout << "ERROR::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl; }

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success) { glGetShaderInfoLog(fragment, 512, NULL, infoLog); std::cout << "ERROR::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl; }

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);

        glBindAttribLocation(ID, 0, "aPos");
        glBindAttribLocation(ID, 1, "aColor");

        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) { glGetProgramInfoLog(ID, 512, NULL, infoLog); std::cout << "ERROR::SHADER::LINKING_FAILED\n" << infoLog << std::endl; }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() { glUseProgram(ID); }
    void setMat4(const std::string& name, const Matrix4& mat) const { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, mat.m); }
    void setVec3(const std::string& name, float x, float y, float z) const { glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); }
};

void initOpenGL(int width, int height);
void processInput(sf::RenderWindow& window, float& offsetX, float& offsetY, float& offsetZ, float dt);

int main()
{
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.majorVersion = 3;
    settings.minorVersion = 3;
    settings.attributeFlags = sf::ContextSettings::Core;

    sf::RenderWindow window(sf::VideoMode(SCR_WIDTH, SCR_HEIGHT, 32),
        "Gradient Tetrahedron",
        sf::Style::Default,
        settings);

    window.setFramerateLimit(60);
    window.setActive(true);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    initOpenGL(SCR_WIDTH, SCR_HEIGHT);

    Shader ourShader(vertexShaderSource, fragmentShaderSource);

    const float SQRT_3 = 1.73205f;
    const float SQRT_6 = 2.44949f;

    float vertices[] = {
        0.0f,  0.0f,  1.0f,                 1.0f, 0.0f, 0.0f,
        0.0f,  2.0f * SQRT_6 / 3.0f, -1.0f / 3.0f, 0.0f, 1.0f, 0.0f,
        SQRT_3, -1.0f * SQRT_6 / 3.0f, -1.0f / 3.0f, 0.0f, 0.0f, 1.0f,
        -SQRT_3, -1.0f * SQRT_6 / 3.0f, -1.0f / 3.0f, 1.0f, 1.0f, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 3, 2
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = -10.0f;

    sf::Clock clock;
    sf::Clock rotationClock;

    while (window.isOpen())
    {
        float dt = clock.restart().asSeconds();
        processInput(window, offsetX, offsetY, offsetZ, dt);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        Matrix4 projection = Matrix4::Perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        Matrix4 view = Matrix4::Identity();

        Matrix4 model = Matrix4::Identity();

        Matrix4 translationMatrix = Matrix4::Identity();
        translationMatrix.m[12] = offsetX;
        translationMatrix.m[13] = offsetY;
        translationMatrix.m[14] = offsetZ;

        model = translationMatrix * model;

        model = Matrix4::Rotate(model, 30.0f, 1.0f, 0.0f, 0.0f);

        float rotationSpeed = 20.0f;
        float angle = rotationClock.getElapsedTime().asSeconds() * rotationSpeed;
        model = Matrix4::Rotate(model, angle, 0.0f, 1.0f, 0.0f);

        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);

        window.display();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    return 0;
}

void initOpenGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
}

void processInput(sf::RenderWindow& window, float& offsetX, float& offsetY, float& offsetZ, float dt)
{
    sf::Event event;
    while (window.pollEvent(event))
    {
        if (event.type == sf::Event::Closed)
            window.close();
        if (event.type == sf::Event::Resized)
            glViewport(0, 0, event.size.width, event.size.height);
    }

    const float moveSpeed = 3.0f * dt;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) offsetY += moveSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) offsetY -= moveSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) offsetX -= moveSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) offsetX += moveSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) offsetZ += moveSpeed;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) offsetZ -= moveSpeed;
}