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

class Matrix4 {
public:
    float m[16];
    Matrix4() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    static Matrix4 Identity() { return Matrix4(); }
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
        r.m[0] = x * x * (1 - c) + c;     r.m[1] = y * x * (1 - c) + z * s; r.m[2] = z * x * (1 - c) - y * s;
        r.m[4] = x * y * (1 - c) - z * s; r.m[5] = y * y * (1 - c) + c;     r.m[6] = z * y * (1 - c) + x * s;
        r.m[8] = x * z * (1 - c) + y * s; r.m[9] = y * z * (1 - c) - x * s; r.m[10] = z * z * (1 - c) + c;
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
};

const char* colorVertexSource =
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

const char* colorFragmentSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 ourColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(ourColor, 1.0f);\n"
"}\n";

const char* cubeVertexSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"out vec3 LocalPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    TexCoord = aTexCoord;\n"
"    LocalPos = aPos;\n"
"}\n";

const char* cubeFragmentSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 TexCoord;\n"
"in vec3 LocalPos;\n"
"uniform sampler2D imageTexture;\n"
"uniform float mixVal;\n"
"uniform vec3 colorVal;\n"
"uniform int mode;\n" 
"void main()\n"
"{\n"
"    vec4 gradientColor = vec4(LocalPos + 0.5, 1.0);\n"
"    \n"
"    if (mode == 3) {\n"
"        vec4 imgColor = texture(imageTexture, TexCoord);\n"
"        FragColor = mix(gradientColor, imgColor, mixVal);\n"
"    } else {\n"
"        FragColor = gradientColor * vec4(colorVal, 1.0);\n"
"    }\n"
"}\n";

class Shader {
public:
    unsigned int ID;
    Shader(const char* vCode, const char* fCode) {
        unsigned int v, f;
        int success; char log[512];
        v = glCreateShader(GL_VERTEX_SHADER); glShaderSource(v, 1, &vCode, NULL); glCompileShader(v);
        glGetShaderiv(v, GL_COMPILE_STATUS, &success);
        if (!success) { glGetShaderInfoLog(v, 512, NULL, log); std::cout << "VERT ERR: " << log << std::endl; }

        f = glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(f, 1, &fCode, NULL); glCompileShader(f);
        glGetShaderiv(f, GL_COMPILE_STATUS, &success);
        if (!success) { glGetShaderInfoLog(f, 512, NULL, log); std::cout << "FRAG ERR: " << log << std::endl; }

        ID = glCreateProgram(); glAttachShader(ID, v); glAttachShader(ID, f); glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) { glGetProgramInfoLog(ID, 512, NULL, log); std::cout << "LINK ERR: " << log << std::endl; }
        glDeleteShader(v); glDeleteShader(f);
    }
    void use() { glUseProgram(ID); }
    void setMat4(const std::string& name, const Matrix4& mat) const { glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, mat.m); }
    void setVec3(const std::string& name, float x, float y, float z) const { glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); }
    void setFloat(const std::string& name, float value) const { glUniform1f(glGetUniformLocation(ID, name.c_str()), value); }
    void setInt(const std::string& name, int value) const { glUniform1i(glGetUniformLocation(ID, name.c_str()), value); }
};

unsigned int loadTexture(const char* filename) {
    unsigned int tID;
    glGenTextures(1, &tID);
    glBindTexture(GL_TEXTURE_2D, tID);

    sf::Image img;
    if (img.loadFromFile(filename)) {
        img.flipVertically();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4); 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.getSize().x, img.getSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getPixelsPtr());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        std::cerr << "Ошибка, изображения нету" << std::endl;
    }
    return tID;
}

void setupTetra(unsigned int& VAO, unsigned int& count) {
    const float SQRT_3 = 1.73205f; const float SQRT_6 = 2.44949f;
    float v[] = {
        0.0f, 0.0f, 1.0f, 1,0,0,  0.0f, 2.0f * SQRT_6 / 3.0f, -1.0f / 3.0f, 0,1,0,
        SQRT_3, -1.0f * SQRT_6 / 3.0f, -1.0f / 3.0f, 0,0,1, -SQRT_3, -1.0f * SQRT_6 / 3.0f, -1.0f / 3.0f, 1,1,0
    };
    unsigned int idx[] = { 0,1,2, 0,2,3, 0,3,1, 1,3,2 };
    count = 12;
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void setupCube(unsigned int& VAO) {
    float v[] = {
        -0.5f, -0.5f, -0.5f, 0,0,  0.5f, -0.5f, -0.5f, 1,0,  0.5f,  0.5f, -0.5f, 1,1,
         0.5f,  0.5f, -0.5f, 1,1, -0.5f,  0.5f, -0.5f, 0,1, -0.5f, -0.5f, -0.5f, 0,0,
         -0.5f, -0.5f,  0.5f, 0,0,  0.5f, -0.5f,  0.5f, 1,0,  0.5f,  0.5f,  0.5f, 1,1,
          0.5f,  0.5f,  0.5f, 1,1, -0.5f,  0.5f,  0.5f, 0,1, -0.5f, -0.5f,  0.5f, 0,0,
          -0.5f,  0.5f,  0.5f, 1,0, -0.5f,  0.5f, -0.5f, 1,1, -0.5f, -0.5f, -0.5f, 0,1,
          -0.5f, -0.5f, -0.5f, 0,1, -0.5f, -0.5f,  0.5f, 0,0, -0.5f,  0.5f,  0.5f, 1,0,
           0.5f,  0.5f,  0.5f, 1,0,  0.5f,  0.5f, -0.5f, 1,1,  0.5f, -0.5f, -0.5f, 0,1,
           0.5f, -0.5f, -0.5f, 0,1,  0.5f, -0.5f,  0.5f, 0,0,  0.5f,  0.5f,  0.5f, 1,0,
           -0.5f, -0.5f, -0.5f, 0,1,  0.5f, -0.5f, -0.5f, 1,1,  0.5f, -0.5f,  0.5f, 1,0,
            0.5f, -0.5f,  0.5f, 1,0, -0.5f, -0.5f,  0.5f, 0,0, -0.5f, -0.5f, -0.5f, 0,1,
            -0.5f,  0.5f, -0.5f, 0,1,  0.5f,  0.5f, -0.5f, 1,1,  0.5f,  0.5f,  0.5f, 1,0,
             0.5f,  0.5f,  0.5f, 1,0, -0.5f,  0.5f,  0.5f, 0,0, -0.5f,  0.5f, -0.5f, 0,1
    };
    unsigned int VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

int main() {
    sf::ContextSettings s; s.depthBits = 24; s.majorVersion = 3; s.minorVersion = 3;
    sf::RenderWindow win(sf::VideoMode(800, 600), "OpenGL Final", sf::Style::Default, s);
    win.setFramerateLimit(60); win.setActive(true);
    glewExperimental = GL_TRUE; glewInit(); glEnable(GL_DEPTH_TEST);

    Shader tetraShader(colorVertexSource, colorFragmentSource);
    Shader cubeShader(cubeVertexSource, cubeFragmentSource);

    unsigned int tetraVAO, tetraCount, cubeVAO;
    setupTetra(tetraVAO, tetraCount);
    setupCube(cubeVAO);

    unsigned int texID = loadTexture("image.png");

    cubeShader.use();
    cubeShader.setInt("imageTexture", 0);

    int task = 1;
    float tx = 0, ty = 0, tz = -5;
    float cr = 1, cg = 1, cb = 1;
    float mixVal = 0.5f;

    sf::Clock cl, rotCl;

    while (win.isOpen()) {
        float dt = cl.restart().asSeconds();
        sf::Event e;
        while (win.pollEvent(e)) {
            if (e.type == sf::Event::Closed) win.close();
            if (e.type == sf::Event::Resized) glViewport(0, 0, e.size.width, e.size.height);
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Num1) task = 1;
                if (e.key.code == sf::Keyboard::Num2) task = 2;
                if (e.key.code == sf::Keyboard::Num3) task = 3;

                if (task == 2) {
                    if (e.key.code == sf::Keyboard::R) cr += 0.1f; if (e.key.code == sf::Keyboard::F) cr -= 0.1f;
                    if (e.key.code == sf::Keyboard::G) cg += 0.1f; if (e.key.code == sf::Keyboard::H) cg -= 0.1f;
                    if (e.key.code == sf::Keyboard::B) cb += 0.1f; if (e.key.code == sf::Keyboard::N) cb -= 0.1f;
                }
                if (task == 3) {
                    if (e.key.code == sf::Keyboard::Up) mixVal += 0.1f;
                    if (e.key.code == sf::Keyboard::Down) mixVal -= 0.1f;
                }

                if (cr < 0) cr = 0; if (cr > 1) cr = 1; if (cg < 0) cg = 0; if (cg > 1) cg = 1; if (cb < 0) cb = 0; if (cb > 1) cb = 1;
                if (mixVal < 0) mixVal = 0; if (mixVal > 1) mixVal = 1;
            }
        }

        if (task == 1) {
            float sp = 3.0f * dt;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) ty += sp;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) ty -= sp;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) tx -= sp;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) tx += sp;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) tz += sp;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) tz -= sp;
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Matrix4 proj = Matrix4::Perspective(45.0f, 800.0f / 600.0f, 0.1f, 100.0f);
        Matrix4 view = Matrix4::Identity();
        float ang = rotCl.getElapsedTime().asSeconds() * 30.0f;

        if (task == 1) {
            tetraShader.use();
            Matrix4 m = Matrix4::Identity();
            Matrix4 tr = Matrix4::Identity(); tr.m[12] = tx; tr.m[13] = ty; tr.m[14] = tz;
            m = tr * m;
            m = Matrix4::Rotate(m, 30.0f, 1, 0, 0);
            m = Matrix4::Rotate(m, ang, 0, 1, 0);
            tetraShader.setMat4("projection", proj); tetraShader.setMat4("view", view); tetraShader.setMat4("model", m);
            glBindVertexArray(tetraVAO); glDrawElements(GL_TRIANGLES, tetraCount, GL_UNSIGNED_INT, 0);
        }
        else {
            cubeShader.use();
            Matrix4 m = Matrix4::Identity();
            Matrix4 tr = Matrix4::Identity(); tr.m[14] = -4.0f;
            m = tr * m;
            m = Matrix4::Rotate(m, ang, 0.5f, 1.0f, 0.0f);
            cubeShader.setMat4("projection", proj); cubeShader.setMat4("view", view); cubeShader.setMat4("model", m);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texID);

            cubeShader.setInt("mode", task);
            cubeShader.setVec3("colorVal", cr, cg, cb);
            cubeShader.setFloat("mixVal", mixVal);

            glBindVertexArray(cubeVAO); glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        win.display();
    }
    return 0;
}