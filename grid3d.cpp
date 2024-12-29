#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#define M_PI 3.14159265358979323846

// Grid parameters
const int GRID_SIZE = 5;  // Number of cells in each dimension
const float CELL_SIZE = 0.2f;  // Size of each cell

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    uniform bool isRay;  // Used to distinguish between grid and ray
    uniform bool isSphere;  // Used to identify sphere
    out vec4 FragColor;
    void main() {
        if (isRay) {
            FragColor = vec4(1.0, 1.0, 0.0, 1.0); // Yellow for ray
        } else if (isSphere) {
            FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Blue for sphere
        } else {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White for grid
        }
    }
)";

// Add this new class after the Ray class
class Sphere {
public:
    float x, y, z;  // Position
    float radius;
    std::vector<float> vertices;
    
    Sphere(float xPos, float yPos, float zPos, float r) 
        : x(xPos), y(yPos), z(zPos), radius(r) {
        generateVertices();
    }
    
    void generateVertices() {
        const int segments = 16;
        const int rings = 16;
        
        for(int i = 0; i <= rings; i++) {
            float phi = M_PI * (float)i / (float)rings;
            for(int j = 0; j <= segments; j++) {
                float theta = 2.0f * M_PI * (float)j / (float)segments;
                
                float xPos = x + radius * sin(phi) * cos(theta);
                float yPos = y + radius * sin(phi) * sin(theta);
                float zPos = z + radius * cos(phi);
                
                vertices.push_back(xPos);
                vertices.push_back(yPos);
                vertices.push_back(zPos);
            }
        }
    }
};

class Ray {
public:
    // Spherical coordinates
    float zenith;    // angle from vertical (0 = up, 180 = down)
    float azimuth;   // angle in horizontal plane (0 = +x axis)
    float currentLength;
    float speed;
    float gridHalfSize;  // Half size of the grid for boundary checking
    
    // Current position
    float x, y, z;
    
    Ray() : 
        zenith(45.0f),
        azimuth(45.0f),
        currentLength(0.0f),
        speed(0.005f),
        gridHalfSize((GRID_SIZE * CELL_SIZE) / 2.0f),
        x(0.0f), y(0.0f), z(0.0f) {}
    
    bool isOutsideCube() {
        return (std::fabs(x) > gridHalfSize || 
                std::fabs(y) > gridHalfSize || 
                std::fabs(z) > gridHalfSize);
    }
    
    bool intersectsSphere(float sphereX, float sphereY, float sphereZ, float sphereRadius) {
        // Calculate distance from current ray point to sphere center
        float dx = x - sphereX;
        float dy = y - sphereY;
        float dz = z - sphereZ;
        float distanceSquared = dx*dx + dy*dy + dz*dz;
        
        // If distance is less than sphere radius, we've hit it
        return distanceSquared <= (sphereRadius * sphereRadius);
    }


    void update(const std::vector<Sphere>& spheres) {  // Modified to take spheres as parameter
        currentLength += speed;
        updatePosition();
        
        // Check for sphere collisions
        for(const Sphere& sphere : spheres) {
            if(intersectsSphere(sphere.x, sphere.y, sphere.z, sphere.radius)) {
                std::cout << "Hit sphere at (" << sphere.x << ", " << sphere.y << ", " << sphere.z << ")" << std::endl;
                currentLength = 0.0f;
                x = y = z = 0.0f;  // Reset position to center
                zenith = rand() % 180;  // Random angle 0-180
                azimuth = rand() % 360; // Random angle 0-360
                return;  // Exit early since we hit a sphere
            }
        }
        
        // Reset if ray hits cube boundary
        if (isOutsideCube()) {
            std::cout << "Position at boundary: (" << x << ", " << y << ", " << z << "), halfsize: " << gridHalfSize << std::endl;
            currentLength = 0.0f;
            x = y = z = 0.0f;  // Reset position to center
            zenith = rand() % 180;  // Random angle 0-180
            azimuth = rand() % 360; // Random angle 0-360
        }
    }
    
    void updatePosition() {
        float phi = glm::radians(zenith);
        float theta = glm::radians(azimuth);
        
        x = currentLength * sin(phi) * cos(theta);
        y = currentLength * cos(phi);
        z = currentLength * sin(phi) * sin(theta);
    }
    
    // For rendering
    void getEndPoint(float& outX, float& outY, float& outZ) {
        outX = x;
        outY = y;
        outZ = z;
    }
};

// Function to create grid lines for one face of the cube
void addGridLines(std::vector<float>& vertices, float x, float y, float z, char axis) {
    for (int i = 0; i <= GRID_SIZE; i++) {
        float offset = i * CELL_SIZE;
        if (axis == 'x') {
            vertices.push_back(x); vertices.push_back(y + offset); vertices.push_back(z);
            vertices.push_back(x + GRID_SIZE * CELL_SIZE); vertices.push_back(y + offset); vertices.push_back(z);
        }
        else if (axis == 'y') {
            vertices.push_back(x + offset); vertices.push_back(y); vertices.push_back(z);
            vertices.push_back(x + offset); vertices.push_back(y + GRID_SIZE * CELL_SIZE); vertices.push_back(z);
        }
    }
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(800, 800, "3D Grid", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Create and compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Create grid vertices
    std::vector<float> vertices;
    float halfSize = (GRID_SIZE * CELL_SIZE) / 2.0f;
    
    // Front face
    addGridLines(vertices, -halfSize, -halfSize, -halfSize, 'x');
    addGridLines(vertices, -halfSize, -halfSize, -halfSize, 'y');
    
    // Back face
    addGridLines(vertices, -halfSize, -halfSize, halfSize, 'x');
    addGridLines(vertices, -halfSize, -halfSize, halfSize, 'y');
    
    // Connect front to back
    for (int i = 0; i <= GRID_SIZE; i++) {
        for (int j = 0; j <= GRID_SIZE; j++) {
            float x = -halfSize + (i * CELL_SIZE);
            float y = -halfSize + (j * CELL_SIZE);
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(-halfSize);
            vertices.push_back(x); vertices.push_back(y); vertices.push_back(halfSize);
        }
    }

    // Create and bind VAO and VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Camera parameters
    float cameraRadius = 3.0f;
    float cameraTheta = 45.0f;  // horizontal angle
    float cameraPhi = 45.0f;    // vertical angle
    const float rotationSpeed = 2.0f;

    Ray ray;
    GLuint isRayLocation = glGetUniformLocation(shaderProgram, "isRay");

    // Create ray vertices buffer
    GLuint rayVAO, rayVBO;
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);
    
    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    // We'll update this buffer every frame
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


        // Create three spheres
    std::vector<Sphere> spheres = {




        Sphere(0.100, -0.1000f, 0.4, 0.02f),
        Sphere(0.100, -0.1000f, -0.4f, 0.02f),
        Sphere(0.100, -0.1000f, 0.35f, 0.02f),
        Sphere(0.100, -0.1000f, 0.30f, 0.02f),
        Sphere(0.100, -0.1000f, 0.25f, 0.02f),
        Sphere(0.100, -0.1000f, 0.20f, 0.02f),
        Sphere(0.100, -0.1000f, 0.15f, 0.02f),
        Sphere(0.100, -0.1000f, 0.10f, 0.02f),
        Sphere(0.100, -0.1000f, 0.05f, 0.02f),
        Sphere(0.100, -0.1000f, 0.00f, 0.02f),
        Sphere(0.100, -0.1000f, -0.05f, 0.02f),
        Sphere(0.100, -0.1000f, -0.10f, 0.02f),
        Sphere(0.100, -0.1000f, -0.15f, 0.02f),
        Sphere(0.100, -0.1000f, -0.2f, 0.02f),
        Sphere(0.100, -0.1000f, -0.25f, 0.02f),
        Sphere(0.100, -0.1000f, -0.30f, 0.02f),
        Sphere(0.100, -0.1000f, -0.35f, 0.02f),
        Sphere(0.100, -0.1000f, -0.40f, 0.02f),

        Sphere(0.2f, -0.1000f, 0.4, 0.02f),
        Sphere(0.2f, -0.1000f, -0.4f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.35f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.30f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.25f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.20f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.15f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.10f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.05f, 0.02f),
        Sphere(0.2f, -0.1000f, 0.00f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.05f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.10f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.15f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.2f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.25f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.30f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.35f, 0.02f),
        Sphere(0.2f, -0.1000f, -0.40f, 0.02f),

        Sphere(0.300f, -0.1000f, 0.4, 0.02f),
        Sphere(0.300f, -0.1000f, -0.4f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.35f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.30f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.25f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.20f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.15f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.10f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.05f, 0.02f),
        Sphere(0.300f, -0.1000f, 0.00f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.05f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.10f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.15f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.2f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.25f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.30f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.35f, 0.02f),
        Sphere(0.300f, -0.1000f, -0.40f, 0.02f),

        Sphere(0.400f, -0.1000f, 0.4, 0.02f),
        Sphere(0.400f, -0.1000f, -0.4f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.35f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.30f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.25f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.20f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.15f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.10f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.05f, 0.02f),
        Sphere(0.400f, -0.1000f, 0.00f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.05f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.10f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.15f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.2f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.25f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.30f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.35f, 0.02f),
        Sphere(0.400f, -0.1000f, -0.40f, 0.02f),
 
        







        Sphere(0.100, 0.0000, 0.4, 0.02f),
        Sphere(0.100, 0.0000, -0.4f, 0.02f),
        Sphere(0.100, 0.0000, 0.35f, 0.02f),
        Sphere(0.100, 0.0000, 0.30f, 0.02f),
        Sphere(0.100, 0.0000, 0.25f, 0.02f),
        Sphere(0.100, 0.0000, 0.20f, 0.02f),
        Sphere(0.100, 0.0000, 0.15f, 0.02f),
        Sphere(0.100, 0.0000, 0.10f, 0.02f),
        Sphere(0.100, 0.0000, 0.05f, 0.02f),
        Sphere(0.100, 0.0000, 0.00f, 0.02f),
        Sphere(0.100, 0.0000, -0.05f, 0.02f),
        Sphere(0.100, 0.0000, -0.10f, 0.02f),
        Sphere(0.100, 0.0000, -0.15f, 0.02f),
        Sphere(0.100, 0.0000, -0.2f, 0.02f),
        Sphere(0.100, 0.0000, -0.25f, 0.02f),
        Sphere(0.100, 0.0000, -0.30f, 0.02f),
        Sphere(0.100, 0.0000, -0.35f, 0.02f),
        Sphere(0.100, 0.0000, -0.40f, 0.02f),

        Sphere(0.2f, 0.0000, 0.4, 0.02f),
        Sphere(0.2f, 0.0000, -0.4f, 0.02f),
        Sphere(0.2f, 0.0000, 0.35f, 0.02f),
        Sphere(0.2f, 0.0000, 0.30f, 0.02f),
        Sphere(0.2f, 0.0000, 0.25f, 0.02f),
        Sphere(0.2f, 0.0000, 0.20f, 0.02f),
        Sphere(0.2f, 0.0000, 0.15f, 0.02f),
        Sphere(0.2f, 0.0000, 0.10f, 0.02f),
        Sphere(0.2f, 0.0000, 0.05f, 0.02f),
        Sphere(0.2f, 0.0000, 0.00f, 0.02f),
        Sphere(0.2f, 0.0000, -0.05f, 0.02f),
        Sphere(0.2f, 0.0000, -0.10f, 0.02f),
        Sphere(0.2f, 0.0000, -0.15f, 0.02f),
        Sphere(0.2f, 0.0000, -0.2f, 0.02f),
        Sphere(0.2f, 0.0000, -0.25f, 0.02f),
        Sphere(0.2f, 0.0000, -0.30f, 0.02f),
        Sphere(0.2f, 0.0000, -0.35f, 0.02f),
        Sphere(0.2f, 0.0000, -0.40f, 0.02f),

        Sphere(0.300f, 0.0000, 0.4, 0.02f),
        Sphere(0.300f, 0.0000, -0.4f, 0.02f),
        Sphere(0.300f, 0.0000, 0.35f, 0.02f),
        Sphere(0.300f, 0.0000, 0.30f, 0.02f),
        Sphere(0.300f, 0.0000, 0.25f, 0.02f),
        Sphere(0.300f, 0.0000, 0.20f, 0.02f),
        Sphere(0.300f, 0.0000, 0.15f, 0.02f),
        Sphere(0.300f, 0.0000, 0.10f, 0.02f),
        Sphere(0.300f, 0.0000, 0.05f, 0.02f),
        Sphere(0.300f, 0.0000, 0.00f, 0.02f),
        Sphere(0.300f, 0.0000, -0.05f, 0.02f),
        Sphere(0.300f, 0.0000, -0.10f, 0.02f),
        Sphere(0.300f, 0.0000, -0.15f, 0.02f),
        Sphere(0.300f, 0.0000, -0.2f, 0.02f),
        Sphere(0.300f, 0.0000, -0.25f, 0.02f),
        Sphere(0.300f, 0.0000, -0.30f, 0.02f),
        Sphere(0.300f, 0.0000, -0.35f, 0.02f),
        Sphere(0.300f, 0.0000, -0.40f, 0.02f),

        Sphere(0.400f, 0.0000, 0.4, 0.02f),
        Sphere(0.400f, 0.0000, -0.4f, 0.02f),
        Sphere(0.400f, 0.0000, 0.35f, 0.02f),
        Sphere(0.400f, 0.0000, 0.30f, 0.02f),
        Sphere(0.400f, 0.0000, 0.25f, 0.02f),
        Sphere(0.400f, 0.0000, 0.20f, 0.02f),
        Sphere(0.400f, 0.0000, 0.15f, 0.02f),
        Sphere(0.400f, 0.0000, 0.10f, 0.02f),
        Sphere(0.400f, 0.0000, 0.05f, 0.02f),
        Sphere(0.400f, 0.0000, 0.00f, 0.02f),
        Sphere(0.400f, 0.0000, -0.05f, 0.02f),
        Sphere(0.400f, 0.0000, -0.10f, 0.02f),
        Sphere(0.400f, 0.0000, -0.15f, 0.02f),
        Sphere(0.400f, 0.0000, -0.2f, 0.02f),
        Sphere(0.400f, 0.0000, -0.25f, 0.02f),
        Sphere(0.400f, 0.0000, -0.30f, 0.02f),
        Sphere(0.400f, 0.0000, -0.35f, 0.02f),
        Sphere(0.400f, 0.0000, -0.40f, 0.02f),
 
        



        





        Sphere(0.100, 0.1000f, 0.4, 0.02f),
        Sphere(0.100, 0.1000f, -0.4f, 0.02f),
        Sphere(0.100, 0.1000f, 0.35f, 0.02f),
        Sphere(0.100, 0.1000f, 0.30f, 0.02f),
        Sphere(0.100, 0.1000f, 0.25f, 0.02f),
        Sphere(0.100, 0.1000f, 0.20f, 0.02f),
        Sphere(0.100, 0.1000f, 0.15f, 0.02f),
        Sphere(0.100, 0.1000f, 0.10f, 0.02f),
        Sphere(0.100, 0.1000f, 0.05f, 0.02f),
        Sphere(0.100, 0.1000f, 0.00f, 0.02f),
        Sphere(0.100, 0.1000f, -0.05f, 0.02f),
        Sphere(0.100, 0.1000f, -0.10f, 0.02f),
        Sphere(0.100, 0.1000f, -0.15f, 0.02f),
        Sphere(0.100, 0.1000f, -0.2f, 0.02f),
        Sphere(0.100, 0.1000f, -0.25f, 0.02f),
        Sphere(0.100, 0.1000f, -0.30f, 0.02f),
        Sphere(0.100, 0.1000f, -0.35f, 0.02f),
        Sphere(0.100, 0.1000f, -0.40f, 0.02f),

        Sphere(0.2f, 0.1000f, 0.4, 0.02f),
        Sphere(0.2f, 0.1000f, -0.4f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.35f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.30f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.25f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.20f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.15f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.10f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.05f, 0.02f),
        Sphere(0.2f, 0.1000f, 0.00f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.05f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.10f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.15f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.2f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.25f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.30f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.35f, 0.02f),
        Sphere(0.2f, 0.1000f, -0.40f, 0.02f),

        Sphere(0.300f, 0.1000f, 0.4, 0.02f),
        Sphere(0.300f, 0.1000f, -0.4f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.35f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.30f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.25f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.20f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.15f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.10f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.05f, 0.02f),
        Sphere(0.300f, 0.1000f, 0.00f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.05f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.10f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.15f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.2f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.25f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.30f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.35f, 0.02f),
        Sphere(0.300f, 0.1000f, -0.40f, 0.02f),

        Sphere(0.400f, 0.1000f, 0.4, 0.02f),
        Sphere(0.400f, 0.1000f, -0.4f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.35f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.30f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.25f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.20f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.15f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.10f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.05f, 0.02f),
        Sphere(0.400f, 0.1000f, 0.00f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.05f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.10f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.15f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.2f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.25f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.30f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.35f, 0.02f),
        Sphere(0.400f, 0.1000f, -0.40f, 0.02f),
 
        





        Sphere(0.100, 0.2000f, 0.4, 0.02f),
        Sphere(0.100, 0.2000f, -0.4f, 0.02f),
        Sphere(0.100, 0.2000f, 0.35f, 0.02f),
        Sphere(0.100, 0.2000f, 0.30f, 0.02f),
        Sphere(0.100, 0.2000f, 0.25f, 0.02f),
        Sphere(0.100, 0.2000f, 0.20f, 0.02f),
        Sphere(0.100, 0.2000f, 0.15f, 0.02f),
        Sphere(0.100, 0.2000f, 0.10f, 0.02f),
        Sphere(0.100, 0.2000f, 0.05f, 0.02f),
        Sphere(0.100, 0.2000f, 0.00f, 0.02f),
        Sphere(0.100, 0.2000f, -0.05f, 0.02f),
        Sphere(0.100, 0.2000f, -0.10f, 0.02f),
        Sphere(0.100, 0.2000f, -0.15f, 0.02f),
        Sphere(0.100, 0.2000f, -0.2f, 0.02f),
        Sphere(0.100, 0.2000f, -0.25f, 0.02f),
        Sphere(0.100, 0.2000f, -0.30f, 0.02f),
        Sphere(0.100, 0.2000f, -0.35f, 0.02f),
        Sphere(0.100, 0.2000f, -0.40f, 0.02f),

        Sphere(0.2f, 0.2000f, 0.4, 0.02f),
        Sphere(0.2f, 0.2000f, -0.4f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.35f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.30f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.25f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.20f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.15f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.10f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.05f, 0.02f),
        Sphere(0.2f, 0.2000f, 0.00f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.05f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.10f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.15f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.2f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.25f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.30f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.35f, 0.02f),
        Sphere(0.2f, 0.2000f, -0.40f, 0.02f),

        Sphere(0.300f, 0.2000f, 0.4, 0.02f),
        Sphere(0.300f, 0.2000f, -0.4f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.35f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.30f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.25f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.20f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.15f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.10f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.05f, 0.02f),
        Sphere(0.300f, 0.2000f, 0.00f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.05f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.10f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.15f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.2f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.25f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.30f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.35f, 0.02f),
        Sphere(0.300f, 0.2000f, -0.40f, 0.02f),

        Sphere(0.400f, 0.2000f, 0.4, 0.02f),
        Sphere(0.400f, 0.2000f, -0.4f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.35f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.30f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.25f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.20f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.15f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.10f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.05f, 0.02f),
        Sphere(0.400f, 0.2000f, 0.00f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.05f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.10f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.15f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.2f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.25f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.30f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.35f, 0.02f),
        Sphere(0.400f, 0.2000f, -0.40f, 0.02f),












        Sphere(0.100, 0.30000, 0.4, 0.02f),
        Sphere(0.100, 0.30000, -0.4f, 0.02f),
        Sphere(0.100, 0.30000, 0.35f, 0.02f),
        Sphere(0.100, 0.30000, 0.30f, 0.02f),
        Sphere(0.100, 0.30000, 0.25f, 0.02f),
        Sphere(0.100, 0.30000, 0.20f, 0.02f),
        Sphere(0.100, 0.30000, 0.15f, 0.02f),
        Sphere(0.100, 0.30000, 0.10f, 0.02f),
        Sphere(0.100, 0.30000, 0.05f, 0.02f),
        Sphere(0.100, 0.30000, 0.00f, 0.02f),
        Sphere(0.100, 0.30000, -0.05f, 0.02f),
        Sphere(0.100, 0.30000, -0.10f, 0.02f),
        Sphere(0.100, 0.30000, -0.15f, 0.02f),
        Sphere(0.100, 0.30000, -0.2f, 0.02f),
        Sphere(0.100, 0.30000, -0.25f, 0.02f),
        Sphere(0.100, 0.30000, -0.30f, 0.02f),
        Sphere(0.100, 0.30000, -0.35f, 0.02f),
        Sphere(0.100, 0.30000, -0.40f, 0.02f),

        Sphere(0.2f, 0.30000, 0.4, 0.02f),
        Sphere(0.2f, 0.30000, -0.4f, 0.02f),
        Sphere(0.2f, 0.30000, 0.35f, 0.02f),
        Sphere(0.2f, 0.30000, 0.30f, 0.02f),
        Sphere(0.2f, 0.30000, 0.25f, 0.02f),
        Sphere(0.2f, 0.30000, 0.20f, 0.02f),
        Sphere(0.2f, 0.30000, 0.15f, 0.02f),
        Sphere(0.2f, 0.30000, 0.10f, 0.02f),
        Sphere(0.2f, 0.30000, 0.05f, 0.02f),
        Sphere(0.2f, 0.30000, 0.00f, 0.02f),
        Sphere(0.2f, 0.30000, -0.05f, 0.02f),
        Sphere(0.2f, 0.30000, -0.10f, 0.02f),
        Sphere(0.2f, 0.30000, -0.15f, 0.02f),
        Sphere(0.2f, 0.30000, -0.2f, 0.02f),
        Sphere(0.2f, 0.30000, -0.25f, 0.02f),
        Sphere(0.2f, 0.30000, -0.30f, 0.02f),
        Sphere(0.2f, 0.30000, -0.35f, 0.02f),
        Sphere(0.2f, 0.30000, -0.40f, 0.02f),

        Sphere(0.300f, 0.30000, 0.4, 0.02f),
        Sphere(0.300f, 0.30000, -0.4f, 0.02f),
        Sphere(0.300f, 0.30000, 0.35f, 0.02f),
        Sphere(0.300f, 0.30000, 0.30f, 0.02f),
        Sphere(0.300f, 0.30000, 0.25f, 0.02f),
        Sphere(0.300f, 0.30000, 0.20f, 0.02f),
        Sphere(0.300f, 0.30000, 0.15f, 0.02f),
        Sphere(0.300f, 0.30000, 0.10f, 0.02f),
        Sphere(0.300f, 0.30000, 0.05f, 0.02f),
        Sphere(0.300f, 0.30000, 0.00f, 0.02f),
        Sphere(0.300f, 0.30000, -0.05f, 0.02f),
        Sphere(0.300f, 0.30000, -0.10f, 0.02f),
        Sphere(0.300f, 0.30000, -0.15f, 0.02f),
        Sphere(0.300f, 0.30000, -0.2f, 0.02f),
        Sphere(0.300f, 0.30000, -0.25f, 0.02f),
        Sphere(0.300f, 0.30000, -0.30f, 0.02f),
        Sphere(0.300f, 0.30000, -0.35f, 0.02f),
        Sphere(0.300f, 0.30000, -0.40f, 0.02f),

        Sphere(0.400f, 0.30000, 0.4, 0.02f),
        Sphere(0.400f, 0.30000, -0.4f, 0.02f),
        Sphere(0.400f, 0.30000, 0.35f, 0.02f),
        Sphere(0.400f, 0.30000, 0.30f, 0.02f),
        Sphere(0.400f, 0.30000, 0.25f, 0.02f),
        Sphere(0.400f, 0.30000, 0.20f, 0.02f),
        Sphere(0.400f, 0.30000, 0.15f, 0.02f),
        Sphere(0.400f, 0.30000, 0.10f, 0.02f),
        Sphere(0.400f, 0.30000, 0.05f, 0.02f),
        Sphere(0.400f, 0.30000, 0.00f, 0.02f),
        Sphere(0.400f, 0.30000, -0.05f, 0.02f),
        Sphere(0.400f, 0.30000, -0.10f, 0.02f),
        Sphere(0.400f, 0.30000, -0.15f, 0.02f),
        Sphere(0.400f, 0.30000, -0.2f, 0.02f),
        Sphere(0.400f, 0.30000, -0.25f, 0.02f),
        Sphere(0.400f, 0.30000, -0.30f, 0.02f),
        Sphere(0.400f, 0.30000, -0.35f, 0.02f),
        Sphere(0.400f, 0.30000, -0.40f, 0.02f),






        Sphere(0.100, 0.4000f, 0.4, 0.02f),
        Sphere(0.100, 0.4000f, -0.4f, 0.02f),
        Sphere(0.100, 0.4000f, 0.35f, 0.02f),
        Sphere(0.100, 0.4000f, 0.30f, 0.02f),
        Sphere(0.100, 0.4000f, 0.25f, 0.02f),
        Sphere(0.100, 0.4000f, 0.20f, 0.02f),
        Sphere(0.100, 0.4000f, 0.15f, 0.02f),
        Sphere(0.100, 0.4000f, 0.10f, 0.02f),
        Sphere(0.100, 0.4000f, 0.05f, 0.02f),
        Sphere(0.100, 0.4000f, 0.00f, 0.02f),
        Sphere(0.100, 0.4000f, -0.05f, 0.02f),
        Sphere(0.100, 0.4000f, -0.10f, 0.02f),
        Sphere(0.100, 0.4000f, -0.15f, 0.02f),
        Sphere(0.100, 0.4000f, -0.2f, 0.02f),
        Sphere(0.100, 0.4000f, -0.25f, 0.02f),
        Sphere(0.100, 0.4000f, -0.30f, 0.02f),
        Sphere(0.100, 0.4000f, -0.35f, 0.02f),
        Sphere(0.100, 0.4000f, -0.40f, 0.02f),

        Sphere(0.2f, 0.4000f, 0.4, 0.02f),
        Sphere(0.2f, 0.4000f, -0.4f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.35f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.30f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.25f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.20f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.15f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.10f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.05f, 0.02f),
        Sphere(0.2f, 0.4000f, 0.00f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.05f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.10f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.15f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.2f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.25f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.30f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.35f, 0.02f),
        Sphere(0.2f, 0.4000f, -0.40f, 0.02f),

        Sphere(0.300f, 0.4000f, 0.4, 0.02f),
        Sphere(0.300f, 0.4000f, -0.4f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.35f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.30f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.25f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.20f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.15f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.10f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.05f, 0.02f),
        Sphere(0.300f, 0.4000f, 0.00f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.05f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.10f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.15f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.2f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.25f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.30f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.35f, 0.02f),
        Sphere(0.300f, 0.4000f, -0.40f, 0.02f),

        Sphere(0.400f, 0.4000f, 0.4, 0.02f),
        Sphere(0.400f, 0.4000f, -0.4f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.35f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.30f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.25f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.20f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.15f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.10f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.05f, 0.02f),
        Sphere(0.400f, 0.4000f, 0.00f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.05f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.10f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.15f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.2f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.25f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.30f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.35f, 0.02f),
        Sphere(0.400f, 0.4000f, -0.40f, 0.02f),







    };

    // Create sphere buffers
    std::vector<GLuint> sphereVAOs(spheres.size());
    std::vector<GLuint> sphereVBOs(spheres.size());
    glGenVertexArrays(spheres.size(), sphereVAOs.data());
    glGenBuffers(spheres.size(), sphereVBOs.data());

    for(size_t i = 0; i < spheres.size(); i++) {
        glBindVertexArray(sphereVAOs[i]);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBOs[i]);
        glBufferData(GL_ARRAY_BUFFER, spheres[i].vertices.size() * sizeof(float), 
                    spheres[i].vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    GLuint isSphereLocation = glGetUniformLocation(shaderProgram, "isSphere");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Handle keyboard input
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraTheta += rotationSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraTheta -= rotationSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPhi = std::max(1.0f, cameraPhi - rotationSpeed);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPhi = std::min(179.0f, cameraPhi + rotationSpeed);

        ray.update(spheres);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glUseProgram(shaderProgram);

        // Create transformation matrices
        glm::mat4 model = glm::mat4(1.0f);
        // No translation needed since grid is already centered

        // Calculate camera position using spherical coordinates
        float x = cameraRadius * sin(glm::radians(cameraPhi)) * cos(glm::radians(cameraTheta));
        float y = cameraRadius * cos(glm::radians(cameraPhi));
        float z = cameraRadius * sin(glm::radians(cameraPhi)) * sin(glm::radians(cameraTheta));

        glm::mat4 view = glm::lookAt(
            glm::vec3(x, y, z),  // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f),  // Look at center
            glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
        );

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

        // Set uniforms
        GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
        GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
        GLuint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw grid
        glUniform1i(isRayLocation, 0);  // This is grid, not ray
        glUniform1i(isSphereLocation, 0);
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINES, 0, vertices.size() / 3);

        // Update and draw ray
        glUniform1i(isRayLocation, 1);  // This is ray
        glUniform1i(isSphereLocation, 0);  // Not a sphere
        glBindVertexArray(rayVAO);
        
        float endX, endY, endZ;
        ray.getEndPoint(endX, endY, endZ);
        
        float rayVertices[] = {
            0.0f, 0.0f, 0.0f,     // Start at center
            endX, endY, endZ      // End at calculated point
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rayVertices), rayVertices);
        glDrawArrays(GL_LINES, 0, 2);

        // Draw spheres
        glUniform1i(isRayLocation, 0);  // Not a ray
        glUniform1i(isSphereLocation, 1);  // This is a sphere
        for(size_t i = 0; i < sphereVAOs.size(); i++) {
            glBindVertexArray(sphereVAOs[i]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, spheres[i].vertices.size() / 3);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &rayVAO);
    glDeleteBuffers(1, &rayVBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

        // Clean up sphere buffers
    glDeleteVertexArrays(spheres.size(), sphereVAOs.data());
    glDeleteBuffers(spheres.size(), sphereVBOs.data());
    glfwTerminate();
    return 0;
}