#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include "render.h"
#include "matrix4x4.h"
#include "math3d.h"
#include "halfedge.h"

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "out vec4 vertexColor;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos.xyz, 1.0);\n"
    "   vertexColor = vec4(0.5f, 0.0f, 0.0f, 1.0f);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 ourColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = ourColor;\n"
    "}\n\0";

#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   600

struct {
    float zoom;
    point3d initialPosition;
    point3d position;
    float rotateY;
    float rotateX;
    matrix4x4 matrix;
} camera;

static void onWindowFramebufferSizeChanged(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

static void onMouseScroll(GLFWwindow *window, double xoffset, double yoffset) {
    camera.zoom -= yoffset;
    if (camera.zoom <= 1.0f) {
        camera.zoom = 1.0f;
    }
    if (camera.zoom >= 75.0f) {
        camera.zoom = 75.0f;
    }
    printf("zoom:%f\n", camera.zoom);
}

static void updateCameraMatrix(void) {
    point3d lookAt = {0.0f, 0.0f, 0.0f};
    point3d upPosition = {0.0f, 1.0f, 0.0f};
    vector3d up;
    matrix4x4 cameraRotateYMat = MATRIX4X4_ROTATION_Y(camera.rotateY);
    matrix4x4 cameraRotateXMat = MATRIX4X4_ROTATION_X(camera.rotateX);
    matrix4x4 cameraRotateMat = MATRIX4X4_IDENTITY;
    matrix4x4Multiply(&cameraRotateMat, &cameraRotateYMat);
    matrix4x4Multiply(&cameraRotateMat, &cameraRotateXMat);
    matrix4x4TransformPoint3d(&cameraRotateMat, &upPosition);
    camera.position = camera.initialPosition;
    matrix4x4TransformPoint3d(&cameraRotateMat, &camera.position);
    vector3dSegment(&camera.position, &upPosition, &up);
    matrix4x4Eye(&camera.matrix, &camera.position, &lookAt, &up);
}

static void onMouseMove(GLFWwindow *window, double xpos, double ypos) {
    static double lastXpos = 0;
    static double lastYpos = 0;
    static int firstTime = 1;
    if (firstTime) {
        firstTime = 0;
    } else {
        double xoffset = xpos - lastXpos;
        double yoffset = ypos - lastYpos;
        if (GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
            camera.rotateY -= xoffset * 0.01;
            camera.rotateX += yoffset * 0.01;
            if (camera.rotateY < 0) {
                camera.rotateY += 360;
            }
            if (camera.rotateY >= 360) {
                camera.rotateY = 0;
            }
            if (camera.rotateX < 0) {
                camera.rotateX += 360;
            }
            if (camera.rotateX >= 360) {
                camera.rotateX = 0;
            }
            updateCameraMatrix();
        }
    }
    lastXpos = xpos;
    lastYpos = ypos;
}

int main(int argc, char *argv[]) {
    GLFWwindow *window;
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "dust3d", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onWindowFramebufferSizeChanged);
    glfwSetScrollCallback(window, onMouseScroll);
    glfwSetCursorPosCallback(window, onMouseMove);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "gladLoadGLLoader failed\n");
        return -1;
    }
    
    int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    mesh *m = halfedgeCreateMesh();
    point3d center;
    halfedgeImportObj(m, "/Users/jeremy/Repositories/dust3d/sphere.obj");
    array *vertices = arrayCreate(sizeof(float));
    array *indices = arrayCreate(sizeof(unsigned int));
    halfedgeMeshCenter(m, &center);
    halfedgeExportVerticesAndIndices(m, vertices, indices);
    
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * arrayGetLength(vertices), arrayGetItem(vertices, 0), GL_STATIC_DRAW);
    
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * arrayGetLength(indices), arrayGetItem(indices, 0), GL_STATIC_DRAW);
    
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    camera.zoom = 45.0f;
    camera.initialPosition.x = 0.0f;
    camera.initialPosition.y = 0.0f;
    camera.initialPosition.z = -1.5f;
    camera.position = camera.initialPosition;
    camera.rotateX = 0.0f;
    camera.rotateY = 0.0f;
    updateCameraMatrix();

    while (!glfwWindowShouldClose(window)) {
        if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
            continue;
        }
        
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderProgram);
        
        float timeValue = glfwGetTime();
        float greenValue = sin(timeValue) / 2.0f + 0.5f;
        int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");
        glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);
        
        matrix4x4 perspectiveMat = MATRIX4X4_IDENTITY;
        matrix4x4PerspectiveProjection(&perspectiveMat, math3dDegreeToRadian(camera.zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
        unsigned int projection = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projection, 1, GL_TRUE, perspectiveMat.data);
        
        matrix4x4 modelMat = MATRIX4X4_IDENTITY;
        unsigned int model = glGetUniformLocation(shaderProgram, "model");
        matrix4x4Translation(&modelMat, -center.x, -center.y, -center.z);
        glUniformMatrix4fv(model, 1, GL_TRUE, modelMat.data);
        
        unsigned int view = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(view, 1, GL_TRUE, camera.matrix.data);
    
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, arrayGetLength(indices), GL_UNSIGNED_INT, 0);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


