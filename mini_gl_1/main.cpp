#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "shader.h"
#include "camera.h"

#include <iostream>

const glm::vec3 BACKGROUND_COLOR(6.f / 256.f, 46.f / 256.f, 3.f / 256.f);
const glm::vec3 CIRCLE_COLOR_1(255.f / 256.f, 7.f / 256.f, 137.f / 256.f); 
const glm::vec3 CIRCLE_COLOR_2(219.f / 256.f, 180.f / 256.f, 12.f / 256.f);
const glm::vec3 RECT_COLOR(114.f / 256.f, 134.f / 256.f, 57.f / 256.f);

glm::vec3 currentCircleColor1;
glm::vec3 currentCircleColor2;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 500;
const unsigned int SCR_HEIGHT = 800;

// balls
const int TRAIL_SIZE = 40;
const int CIRCLE_FIDELITY = 50;
const float CIRCLE_ORBIT_RADIUS = 130.f;
const float CIRCLE_RADIUS = 20.f;
const float CIRCLE_SPEED = 210.f;

// fps
const float FPS = 200.f;
const float frameTime = 1.f / FPS;
bool shouldRender = true;
float timePassed = 0.f;

const int DELTAS_COUNT = 5;
float deltas[DELTAS_COUNT];
int currentDeltaIdx = 0;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float realDeltaTime = 0.0f;
float realLastFrame = 0.0f;

// mouse
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float mouseX, mouseY;

// game
const float SPEED_SLOW = 300.f;
const float SPEED_MEDIUM = 340.f;
float gameTempo = 2.f;

bool restart = false;

glm::mat4 projection = glm::ortho(0.f, (float)SCR_WIDTH, 0.f, (float)SCR_HEIGHT);
// glm::mat4 projection = glm::ortho(-1000.f, 1000.f, -1000.f * 8.f/5.f, 1000.f * 8.f/5.f);

struct GlDrawer {
    void bindGlData(unsigned int _vbo, unsigned int _vao, Shader* _shader) {
        vbo = _vbo;
        vao = _vao;
        shader = _shader;
    }

    virtual void update() = 0;

    virtual void draw() = 0;

    unsigned int vbo, vao;
    Shader* shader;
};

struct Balls: public GlDrawer {
    void update() {
        handleReset();
    }

    void updateBalls() {
        ball1 = glm::translate(glm::mat4(1.f), pos);
        ball1 = glm::rotate(ball1, glm::radians(angle), glm::vec3(0.f, 0.f, 1.f));
        ball1 = glm::translate(ball1, glm::vec3(orbitRadius, 0.f, 0.f));
        ball1 = glm::scale(ball1, glm::vec3(radius, radius, 1.f));

        ball2 = glm::translate(glm::mat4(1.f), pos);
        ball2 = glm::rotate(ball2, glm::radians(angle + 180.f), glm::vec3(0.f, 0.f, 1.f));
        ball2 = glm::translate(ball2, glm::vec3(orbitRadius, 0.f, 0.f));
        ball2 = glm::scale(ball2, glm::vec3(radius, radius, 1.f));
    }

    void draw() {
        shader->use();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindVertexArray(vbo);

        drawTrail();
        // shader->setVec3("color", CIRCLE_COLOR_1);
        // drawBalls();
    }

    void drawBalls() {
        shader->setMat4("model", glm::value_ptr(ball1));
        glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_FIDELITY + 2);

        shader->setMat4("model", glm::value_ptr(ball2));
        glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_FIDELITY + 2);
    }

    void handleReset() {
        if (!active) {
            if (justActive) {
                if (angle < 0) {
                    angle = (int)angle % 360;
                    angle += 360.f;
                }

                justActive = false;
                int int_angle = (int)angle;
                float fraction = angle - (float)int_angle;

                angle = (int)angle % 180;
                angle += fraction;
            }

            angle -= realDeltaTime * CIRCLE_SPEED;
            if (angle <= 0.f) {
                angle = 0.f;
                active = true;
                justActive = true;
            }
        }
    }

    void drawTrail() {
        if (lastAngles[currentAngleIndex] != angle) {
            lastAngles[currentAngleIndex] = angle;
            currentAngleIndex = (currentAngleIndex + 1) % TRAIL_SIZE;
        }

#if 0
        for (int i = 0; i < 10; i++) {
            std::cout << lastAngles[i] << ' ';
        }
        std::cout << '\n';
#endif

        float initial_angle = angle;
        float initial_radius = radius;

        int indices[TRAIL_SIZE];
        int k = 0;
        for (int i = currentAngleIndex - 1; i >= 0; i--) {
            indices[k++] = i;
        }
        for (int i = TRAIL_SIZE - 1; i >= currentAngleIndex; i--) {
            indices[k++] = i;
        }

        glm::vec3 current_color = currentCircleColor1;
        glm::vec3 target_color = currentCircleColor2;
        glm::vec3 increment_color = (target_color - current_color) / (float)TRAIL_SIZE;

        float radius_decrement = initial_radius / (float)TRAIL_SIZE;
        // for (int i = TRAIL_SIZE - 1; i >= 0; i--) {
        for (int i = 0; i < TRAIL_SIZE; i++) {
            current_color += increment_color;
            shader->setVec3("color", current_color);

            angle = lastAngles[indices[i]];
            radius -= radius_decrement;

            updateBalls();
            drawBalls();
        }


        angle = initial_angle;
        radius = initial_radius;
        updateBalls();
    }

    void reset() {
        active = false;
    }

    void rotate(int dir) {
        if (!active) return;
        angle += (float)dir * speed * realDeltaTime;
    }

    glm::mat4 ball1 = glm::mat4(1.f);
    glm::mat4 ball2 = glm::mat4(1.f);
    float speed = CIRCLE_SPEED;
    float angle = 0.f;
    float radius = CIRCLE_RADIUS;
    float orbitRadius = CIRCLE_ORBIT_RADIUS;
    glm::vec3 pos = glm::vec3((float)SCR_WIDTH / 2.f, (float)SCR_HEIGHT * 1.f / 5.f, .0f);

    float lastAngles[TRAIL_SIZE];
    int currentAngleIndex = 0;

    bool active = true;
    bool justActive = true;
};

enum class ObstacleType {
    NORMAL,
    BOX,
    N_MAX
};

struct Obstacle {
    Obstacle(float _width, float _height, float _speed, const glm::vec2& _pos)
        :width(_width), height(_height), speed(_speed), pos(_pos), timeOffset(glfwGetTime())
    {
        
    }

    void update() {
        pos -= glm::vec2(0.f, realDeltaTime * speed);

        if (pos.y + height / 2.f <= 0)
            isAlive = false;
    }

    glm::mat4 getModelMatrix() {
        glm::mat4 model(1.f);
        model = glm::translate(model, glm::vec3(pos, 0.f));
        model = glm::scale(model, glm::vec3(width, height, 1.f));
        return model;
    }

    float width;
    float height;
    float speed;
    glm::vec2 pos;
    bool isAlive = true;
    float timeOffset;

    ObstacleType type;
};

struct ObstacleFactory: public GlDrawer {
    void update() {
        const static float wait = .9f;

        timer += realDeltaTime;
        if (timer >= wait) {
            timer -= wait;
            obstacles.push_back(getObstacle((ObstacleType)(rand() % (int)ObstacleType::N_MAX)));
        }
        
        for (auto& obs : obstacles) {
            obs.update();
        }

        obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(), [](const Obstacle& obs) {return !obs.isAlive; }), obstacles.end());
    }

    void checkCollision(Balls* balls) {
        glm::vec2 balls_pos[2];
        balls_pos[0] = { balls->ball1[3][0], balls->ball1[3][1] };
        balls_pos[1] = { balls->ball2[3][0], balls->ball2[3][1] };

        for (auto& obs : obstacles) {
            for (int i = 0; i < 2; i++) {
                if (balls_pos[i].x >= obs.pos.x - obs.width / 2.f - balls->radius &&
                    balls_pos[i].y >= obs.pos.y - obs.height / 2.f - balls->radius &&
                    balls_pos[i].x <= obs.pos.x + obs.width / 2.f + balls->radius &&
                    balls_pos[i].y <= obs.pos.y + obs.height / 2.f + balls->radius) {
                    restart = true;
                    return;
                }
            }
        }
    }

    void draw() {
        shader->use();
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindVertexArray(vao);

        const int N = 3;
        glm::vec3 colors[N];
        for (int i = 0; i < N; i++) {
            colors[i] = glm::vec3(RECT_COLOR - 10.f / 256.f * (i+1));
        }
        float scales[] = { .2f, .2f, .2f };
        float tempos[] = { gameTempo * 4.f, gameTempo * 2.f, gameTempo * 3.f };
        float base_scale = abs(sin(glfwGetTime() * gameTempo));

        /*for (int i = 0; i < 3; i++) {
            scales[i] = 1.f + abs(sin(glfwGetTime() * tempos[i])) * scales[i];
        }*/

        for (auto& obs : obstacles) {
            for (int i = 0; i < 3; i++) {
                shader->setVec3("color", colors[i]);
                glm::mat4 model(1.f);
                model = glm::scale(obs.getModelMatrix(), glm::vec3(1.f + abs(sin(obs.timeOffset + glfwGetTime() * tempos[i])) * scales[i]));
                shader->setMat4("model", glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 6);

                shader->setVec3("color", RECT_COLOR);
                shader->setMat4("model", glm::value_ptr(obs.getModelMatrix()));
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }

    void reset() {
        obstacles.clear();
    }

    Obstacle getObstacle(ObstacleType type) {
        glm::vec2 pos((float)SCR_WIDTH * (rand() % 2 == 0 ? (1.f / 4.f) : (3.f / 4.f)), (float)SCR_HEIGHT + 100.f);

        switch (type)
        {
        case ObstacleType::NORMAL:
            return Obstacle(250.f, 75.f, SPEED_SLOW, pos);
            break;
        case ObstacleType::BOX:
            return Obstacle(125.f, 125.f, SPEED_SLOW, pos);
            break;
        default:
            break;
        }
    }

    std::vector<Obstacle> obstacles;

    float timer = 0.f;
};

float randf();
void updateDelta();
float getDelta();

Balls balls;
ObstacleFactory obstacleFactory;

int main()
{
    srand(time(0));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Minigl1", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    // glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    for (int i = 0; i < DELTAS_COUNT; i++) {
        deltas[i] = (float)1 / FPS;
    }

    float rectVertices[] = {
        -.5f, -.5f, 0.f,
        .5f, -.5f, 0.f,
        .5f, .5f, 0.f,
        .5f, .5f, 0.f,
        -.5f, -.5f, 0.f,
        -.5f, .5f, 0.f
    };

    float circleVertices[(CIRCLE_FIDELITY +2) * 3];
    circleVertices[0] = circleVertices[1] = circleVertices[2] = 0.f;

    constexpr float step = 2.f * glm::pi<float>() / (float)CIRCLE_FIDELITY;
    float current_angle = .0f;
    float radius = 1.f;
    for (int i = 1; i <= CIRCLE_FIDELITY + 1; i++) {
        float x, y;
        x = cos(current_angle) * radius;
        y = sin(current_angle) * radius;
        
        circleVertices[i * 3] = x;
        circleVertices[i * 3 + 1] = y;
        circleVertices[i * 3 + 2] = .0f;

        current_angle += step;
    }
    
    //
    unsigned int circleVBO, circleVAO;
    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);
    glBindVertexArray(circleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circleVertices), circleVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 
    unsigned int rectVBO, rectVAO;
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);

    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // shaders
    Shader circleShader("shaders/vertex.vert", "shaders/color_fragment.frag");
    circleShader.use();
    circleShader.setMat4("projection", glm::value_ptr(projection));

    Shader rectShader("shaders/vertex.vert", "shaders/color_fragment.frag");
    rectShader.use();
    rectShader.setMat4("projection", glm::value_ptr(projection));

    obstacleFactory.bindGlData(rectVBO, rectVAO, &rectShader);
    balls.bindGlData(circleVBO, circleVAO, &circleShader);

    /*glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);*/

    glm::mat4 model(1.f);
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        realDeltaTime = currentFrame - realLastFrame;
        realLastFrame = currentFrame;

        processInput(window);
        // update
        obstacleFactory.update();
        balls.update();
        obstacleFactory.checkCollision(&balls);

        if (restart) {
            balls.reset();
            obstacleFactory.reset();
            restart = false;
            continue;
        }

        if (shouldRender) {
            float scale = 2.f;
            glm::vec3 color1_to_color2(CIRCLE_COLOR_2 - CIRCLE_COLOR_1);
            currentCircleColor1 = glm::vec3(CIRCLE_COLOR_1 + color1_to_color2 * (float)abs(sin(glfwGetTime() * gameTempo * scale)));
            currentCircleColor2 = glm::vec3(CIRCLE_COLOR_1 + color1_to_color2 * (1 - (float)abs(sin(glfwGetTime() * gameTempo * scale))));

            updateDelta();
            shouldRender = false;

            glClearColor(BACKGROUND_COLOR.x, BACKGROUND_COLOR.y, BACKGROUND_COLOR.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // draw
            glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
            glBindVertexArray(circleVAO);

            model = glm::translate(glm::mat4(1.f), balls.pos);
            model = glm::scale(model, glm::vec3(balls.orbitRadius));

            glLineWidth(1.f);
            circleShader.use();
            circleShader.setMat4("model", glm::value_ptr(model));
            circleShader.setVec3("color", glm::vec3(1.f));
            glDrawArrays(GL_LINE_LOOP, 1, CIRCLE_FIDELITY);

            obstacleFactory.draw();
            balls.draw();

            glfwSwapBuffers(window);
        }
        else {
            timePassed += realDeltaTime;
            if (timePassed >= frameTime) {
                shouldRender = true;
                timePassed -= frameTime;
            }
        }

        glfwPollEvents();
    }
    
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &circleVAO);

    glfwTerminate();
    return 0;
}

float randf() {
    return (float)rand() / (float)RAND_MAX;
}

void updateDelta() {
    float currentFrame = static_cast<float>(glfwGetTime());
    float lastDelta = currentFrame - lastFrame;
    lastFrame = currentFrame;
    deltas[currentDeltaIdx] = lastDelta;
    currentDeltaIdx = (currentDeltaIdx + 1) % DELTAS_COUNT;
    deltaTime = getDelta();
}

float getDelta() {
    float sum = .0f;
    for (int i = 0; i < DELTAS_COUNT; i++) {
        sum += deltas[i];
    }

    return sum / DELTAS_COUNT;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        balls.rotate(1);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        balls.rotate(-1);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        if (mouseX >= SCR_WIDTH / 2.f) {
            balls.rotate(-1);
        }
        else {
            balls.rotate(1);
        }
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    mouseX = xposIn;
    mouseY = yposIn;
}