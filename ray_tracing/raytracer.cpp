#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <limits>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

float camerax = 0.0f, cameray = 0.0f, cameraz = 2.0f;
float cameraSpeed = 0.3f;

float cameraFOV = 90.0f;
float FOVspeed = 10.0f;

float lightx = 1.0f, lighty = 1.0f, lightz = -1.0f;
float lightSpeed = 0.3f;

int texWidth, texHeight, texChannels;
unsigned char *loaded_texture;

// Vector3 class for representing 3D vectors
class Vector3 {
public:
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3 &v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3 &v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(float scalar) const { return Vector3(x * scalar, y * scalar, z * scalar); }
    Vector3 operator/(float scalar) const { return Vector3(x / scalar, y / scalar, z / scalar); }

    float dot(const Vector3 &v) const { return x * v.x + y * v.y + z * v.z; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }

    Vector3 normalize() const {
        float len = length();
        return *this / len;
    }

    static Vector3 cross(const Vector3 &v1, const Vector3 &v2) {
        return Vector3(
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x
        );
    }
};

// Function to handle key input
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_W) {
            cameraz -= cameraSpeed;
            std::cout << "W: move camera forward" << std::endl;
            // Handle movement or camera forward logic here
        }
        else if (key == GLFW_KEY_S) {
            cameraz += cameraSpeed;
            std::cout << "S: move camera backward" << std::endl;
            // Handle movement or camera backward logic here
        }
        else if (key == GLFW_KEY_A) {
            camerax -= cameraSpeed;
            std::cout << "A: move camera left" << std::endl;
            // Handle camera move left
        }
        else if (key == GLFW_KEY_D) {
            camerax += cameraSpeed;
            std::cout << "D: move camera right" << std::endl;
            // Handle camera move right
        }
        else if (key == GLFW_KEY_Q) {
            cameray += cameraSpeed;
            std::cout << "Q: move camera down" << std::endl;
        }
        else if( key == GLFW_KEY_E) {
            cameray -= cameraSpeed;
            std::cout << "E: move camera up" << std::endl;
        }
        else if (key == GLFW_KEY_I) {
            lightz -= lightSpeed;
            std::cout << "I: move light" << std::endl;
            // Handle movement or camera backward logic here
        }
        else if (key == GLFW_KEY_K) {
            lightz += lightSpeed;
            std::cout << "K: move light" << std::endl;
            // Handle camera move left
        }
        else if (key == GLFW_KEY_J) {
            lightx -= lightSpeed;
            std::cout << "J: move light" << std::endl;
            // Handle camera move right
        }
        else if (key == GLFW_KEY_L) {
          std::cout << "L: move light" << std::endl;
            lightx += lightSpeed;
        }
        else if( key == GLFW_KEY_U) {
            lighty -= lightSpeed;
            std::cout << "U: move light" << std::endl;
        }
        else if( key == GLFW_KEY_O) {
            lighty += lightSpeed;
            std::cout << "O: move light" << std::endl;
        }
        else if (key == GLFW_KEY_ESCAPE) {
            std::cout << "Escape key pressed: Exit program" << std::endl;
            glfwSetWindowShouldClose(window, GLFW_TRUE);  // Close the window on Escape
        }
        else if (key == GLFW_KEY_UP) {
            cameraFOV -= FOVspeed;
            std::cout << "Up arrow pressed: Zoom in (decrease FOV to " << cameraFOV << ")" << std::endl;
            // Handle camera zoom in
        }
        else if (key == GLFW_KEY_DOWN) {
            cameraFOV += FOVspeed;
            std::cout << "Down arrow pressed: Zoom out (increase FOV to " << cameraFOV << ")" << std::endl;
            // Handle camera zoom out
        }
    }
}

// Ray class for ray tracing
class Ray {
public:
    Vector3 origin, direction;
    int depth;

    Ray(const Vector3 &origin, const Vector3 &direction, int depth = 0)
        : origin(origin), direction(direction.normalize()), depth(depth) {}

    Vector3 at(float t) const {
        return origin + direction * t;
    }
};


// Sphere class for objects in the scene
class Sphere {
public:
    Vector3 center;
    float radius;

    Sphere(const Vector3 &center, float radius) : center(center), radius(radius) {}

    bool intersect(const Ray &ray, float &t) const {
        Vector3 oc = ray.origin - center;
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - radius * radius;
        float discriminant = b * b - 4.0f * a * c;

        if (discriminant > 0) {
            float sqrtDisc = std::sqrt(discriminant);
            float t1 = (-b - sqrtDisc) / (2.0f * a);
            float t2 = (-b + sqrtDisc) / (2.0f * a);
            t = (t1 < t2) ? t1 : t2;
            return true;
        }

        return false;
    }
};

// Planes to create a room
// planes in 3D space can be represented using
// a normal vector and a point on the plane.
class Plane {
public:
    Vector3 point;    // point on the plane
    Vector3 normal;   // normal vector to the plane

    Plane(const Vector3 &point, const Vector3 &normal)
        : point(point), normal(normal.normalize()) {}

    bool intersect(const Ray &ray, float &t) const {
        float denom = normal.dot(ray.direction);
        if (std::abs(denom) > 1e-6) {  // Avoid division by zero
            t = (point - ray.origin).dot(normal) / denom;
            return t >= 0;
        }
        return false;
    }
};

// Scene setup with spheres and walls
class Scene {
public:
  std::vector<Sphere> spheres;
  std::vector<Plane> planes;

    Scene() { // 3 spheres in the scene (center coord), radius)
        spheres.push_back(Sphere(Vector3(0, 0, -5), 1));
        //spheres.push_back(Sphere(Vector3(2, 0, -6), 1));
        //spheres.push_back(Sphere(Vector3(-2, 0, -6), 1));

        // Add walls (planes)
        planes.push_back(Plane(Vector3(0, -2, 0), Vector3(0, 1, 0))); // floor
        planes.push_back(Plane(Vector3(0, 1, 0), Vector3(0, -1, 0))); // ceiling
        //planes.push_back(Plane(Vector3(-3, 0, 0), Vector3(1, 0, 0))); // left wall
        //planes.push_back(Plane(Vector3(3, 0, 0), Vector3(-1, 0, 0))); // right wall
        //planes.push_back(Plane(Vector3(0, 0, -7), Vector3(0, 0, 1))); // back wall
    }

    bool intersect(const Ray &ray, float &t, Vector3 &normal, std::string &objectType, float &reflectivity) const {
        float closest_t = std::numeric_limits<float>::max();
        bool hit = false;

        for (const auto &sphere : spheres) {
            float temp_t;
            if (sphere.intersect(ray, temp_t)) {
                if (temp_t < closest_t) {
                    objectType = "sphere";
                    closest_t = temp_t;
                    hit = true;
                    normal = (ray.at(closest_t) - sphere.center).normalize();
                    reflectivity = 0.4f;  // Example reflectivity for spheres
                }
            }
        }

        for (const auto &plane : planes) {
            float temp_t;
            if (plane.intersect(ray, temp_t) && temp_t < closest_t) {
                objectType = "plane";
                closest_t = temp_t;
                hit = true;
                normal = plane.normal;
                reflectivity = 0.1f;  // Example reflectivity for planes
            }
        }

        t = closest_t;
        return hit;
    }
};

Vector3 trace(const Ray &ray, const Scene &scene, int maxDepth) {
    if (ray.depth > maxDepth) {
        return Vector3(0.1f, 0.1f, 0.1f);  // Background color
    }

    float t;
    Vector3 normal;
    std::string objectType;
    float reflectivity;

    if (scene.intersect(ray, t, normal, objectType, reflectivity)) {
        Vector3 hitPoint = ray.at(t);
        Vector3 viewDir = Vector3(-ray.direction.x, -ray.direction.y, -ray.direction.z);

        // Calculate base color
        Vector3 finalColor;

            if (objectType == "sphere") {
                   // texture mapping for spheres
                   float u = 1.0f - (0.5f + (atan2(normal.z, normal.x) / (2.0f * M_PI)));
                   float v = 1.0f - (0.5f - (asin(normal.y) / M_PI));

                   int texX = std::min((int)(u * texWidth), texWidth - 1);
                   int texY = std::min((int)(v * texHeight), texHeight - 1);
                   int index = (texY * texWidth + texX) * texChannels;

                   float r = loaded_texture[index] / 255.0f;
                   float g = loaded_texture[index + 1] / 255.0f;
                   float b = loaded_texture[index + 2] / 255.0f;

                   Vector3 textureColor(r, g, b);

                   Vector3 pixelPosition = ray.at(t);

                    // Calculate light direction per pixel
                    Vector3 lightDir = (Vector3(lightx, lighty, lightz) - pixelPosition).normalize();

                    // Diffuse lighting
                    float diffuseIntensity = std::max(0.0f, normal.dot(lightDir));

                    // Specular lighting
                    Vector3 viewDir = (ray.origin - pixelPosition).normalize();  // Camera direction
                    Vector3 reflection = (normal * 2.0f * normal.dot(lightDir)) - lightDir;
                    float specularIntensity = std::pow(std::max(0.0f, viewDir.dot(reflection)), 50.0f); // Shininess

                    // Combine ambient, diffuse, and specular components
                    Vector3 lightPosition = Vector3(lightx, lighty, lightz);
                    float ambientIntensity = 0.3f;
                    float lightingIntensity = ambientIntensity + diffuseIntensity + specularIntensity;
                    lightingIntensity = std::min(1.0f, lightingIntensity);

                    float distance = (lightPosition - pixelPosition).length();
                    float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * (distance * distance));
                    lightingIntensity *= attenuation;

                   finalColor = textureColor * lightingIntensity;
            } else if (objectType == "plane") {

                // Flat color for planes
                Vector3 surfaceColor = Vector3(1.0f, 0.1f, 0.1f);  // Base color of the plane

                // Calculate the pixel's position
                Vector3 pixelPosition = ray.at(t);

                // Calculate light direction per pixel
                Vector3 lightDir = (Vector3(lightx, lighty, lightz) - pixelPosition).normalize();

                // Diffuse lighting
                float diffuseIntensity = std::max(0.0f, normal.dot(lightDir));

                // Specular lighting
                Vector3 viewDir = (ray.origin - pixelPosition).normalize();  // Camera direction
                Vector3 reflection = (normal * 2.0f * normal.dot(lightDir)) - lightDir;
                float specularIntensity = std::pow(std::max(0.0f, viewDir.dot(reflection)), 32.0f); // Shininess = 32

                // Combine ambient, diffuse, and specular components
                Vector3 lightPosition = Vector3(lightx, lighty, lightz);
                float ambientIntensity = 0.3f;
                float lightingIntensity = ambientIntensity + diffuseIntensity + specularIntensity;
                lightingIntensity = std::min(1.0f, lightingIntensity);

                float distance = (lightPosition - pixelPosition).length();
                float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * (distance * distance));
                lightingIntensity *= attenuation;

                finalColor = surfaceColor * lightingIntensity;
            }

        // Calculate reflection
        Vector3 reflectedColor;
        Vector3 reflectionDir = ray.direction - normal * 2.0f * ray.direction.dot(normal);
        if (ray.depth > 0){
            Ray reflectedRay(hitPoint + reflectionDir * 1e-4f, reflectionDir, ray.depth - 1);
            reflectedColor = trace(reflectedRay, scene, maxDepth - 1);
        }
        else
        {
            reflectedColor = {0,0,0};
            return finalColor;
        }

        

        // Combine base color and reflected color
        return finalColor * (1.0f - reflectivity) + reflectedColor * reflectivity;
    }

    return Vector3(0.3f, 0.3f, 0.3f);  // Background color
}

// Render function using ray tracing
void render(GLFWwindow *window, int width, int height) {
    glClear(GL_COLOR_BUFFER_BIT);
    Scene scene;

    Vector3 cameraPosition(camerax, cameray, cameraz);  // Move the camera back a bit
    float aspect = (float)width / (float)height;
    float tanFov = std::tan(cameraFOV * 0.5f * M_PI / 180.0f);

    // Define a light source
    Vector3 lightDir(lightx, lighty, lightz);
    lightDir = lightDir.normalize();  // Normalize the light direction
    float ambientIntensity = 0.3f;    // Add ambient light for some brightness

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float px = (2 * (x + 0.5f) / (float)width - 1) * tanFov * aspect;
            float py = (1 - 2 * (y + 0.5f) / (float)height) * tanFov;

            Vector3 rayDir(px, py, -1);
            rayDir = rayDir.normalize();

            Ray ray(cameraPosition, rayDir, 2);//Max depth of 2 for the ray
            Vector3 color = trace(ray, scene, 2);  // 2 as max recursion depth

            glColor3f(color.x, color.y, color.z);
            glBegin(GL_POINTS);
            glVertex2f((2.0f * x) / width - 1, (2.0f * y) / height - 1);
            glEnd();
        }
    }
    glfwSwapBuffers(window);
}

// Main function with GLFW window setup
int main() {

	// load texture
    loaded_texture = stbi_load("metal.png", &texWidth, &texHeight, &texChannels, 0);
    if (!loaded_texture) {
        std::cerr << "Failed to load texture!" << std::endl;
        return -1;
    }
    else std::cout << "Loading texture" << std::endl;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    int width = 800, height = 600;
    GLFWwindow *window = glfwCreateWindow(width, height, "Basic Ray Tracer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);

    glPointSize(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);
        render(window, width, height);
        glfwPollEvents();
    }

    stbi_image_free(loaded_texture);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
