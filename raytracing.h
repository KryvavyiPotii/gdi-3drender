#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>

// Constants.
#define WINDOW_CLASS    L"CG Lab 3 Class"
#define WINDOW_TITLE    L"CG Lab 3"
#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480
#define BG_COLOR        0x00000000  // pixel outside spheres are black

// Base class that represents point in space.
class Primitive
{
public:
    // Coordinates of point.
    float x;
    float y;
    float z;

    Primitive()
    {
        x = y = z = 0;
    }
    Primitive(float x1, float y1, float z1)
    {
        x = x1;
        y = y1;
        z = z1;
    }
};

// Class that represents camera.
class Camera : public Primitive
{
public:
    int width;
    int height;

    Camera()
    {
        width = height = 0;
    }
    Camera(float x1, float y1, float z1, int screenWidth, int screenHeight)
    {
        x = x1;
        y = y1;
        z = z1;
        width = screenWidth;
        height = screenHeight;
    }
};

// Base class that represents vector.
class Vector : public Primitive
{
public:
    using Primitive::Primitive;

    // Calculate vector length.
    virtual float length()
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    // Calculate dot product of current and passed vectors.
    virtual float dot(Vector* vector)
    {
        return x * vector->x + y * vector->y + z * vector->z;
    }
};

// Base class that represents 3D object.
class Object : public Primitive
{
public:
    // Material parameters.
    COLORREF color;

    Object()
    {
        x = y = z = color = 0;
    }
    Object(float x1, float y1, float z1, COLORREF objectColor)
    {
        x = x1;
        y = y1;
        z = z1;
        color = objectColor;
    }

    virtual float intersect(Vector* vector)
    {
        // Calculate relation of Object's x and vector's x.
        float t = x / vector->x;

        // Check if relation is the same for every coordinate.
        // If so, vector intersects with object.
        if (t == y / vector->y && t == z / vector->z) return t;

        return -1;
    }
};

// Class to store light info.
class Light : public Vector
{
public:
    // Light parameters.
    COLORREF color;
    float power;

    Light()
    {
        x = y = z = color = power = 0;
    }
    Light(float x1, float y1, float z1, COLORREF lightColor, float lightPower)
    {
        x = x1;
        y = y1;
        z = z1;
        color = lightColor;
        power = lightPower;
    }

    // Calculate exposure of object's point to light.
    float countLight(Primitive* objectPoint, Primitive* objectCenter)
    {
        // Get vector 1 that points from light source to point.
        Vector lightToPoint;

        lightToPoint.x = x - objectPoint->x;
        lightToPoint.y = y - objectPoint->y;
        lightToPoint.z = z - objectPoint->z;

        // Get vector 2 that points from point to sphere's center.
        Vector pointToCenter;

        pointToCenter.x = objectPoint->x - objectCenter->x;
        pointToCenter.y = objectPoint->y - objectCenter->y;
        pointToCenter.z = objectPoint->z - objectCenter->z;

        // Calculate cosine of angle between vector 1 and 2.
        float cos = lightToPoint.dot(&pointToCenter) / (lightToPoint.length() * pointToCenter.length());

        if (cos > 0 && cos < 1) return cos;

        return 0;
    }

    // Calculate color of object in light.
    COLORREF lightColor(Object* object, float coefficient)
    {
        // COLORREF (typedef DWORD) format: 0x00BBGGRR
        COLORREF newObjectColor = 0;

        for (int i = 0; i <= 16; i += 8)
        {
            // Get color channel.
            char lightChannel = (color >> i) % 255;
            char objectChannel = (object->color >> i) % 255;

            // Create new color channel.
            char newObjectChannel = std::round((lightChannel + objectChannel) * power * coefficient);

            if (newObjectChannel > 0xFF) newObjectChannel = 0xFF;

            // Add new color channel to final color.
            newObjectColor |= newObjectChannel << i;
        }

        return newObjectColor;
    }
};

// Class to store sphere info.
class Sphere : public Object
{
public:
    // Sphere parameters.
    float radius;

    Sphere()
    {
        x = y = z = color = radius = 0;
    }
    Sphere(float x1, float y1, float z1, COLORREF sphereColor, float sphereRadius)
    {
        x = x1;
        y = y1;
        z = z1;
        color = sphereColor;
        radius = sphereRadius;
    }

    float intersect(Vector* vector) override
    {
        // Calculate coefficients of vector and sphere intersection equation.
        float a = vector->x * vector->x + vector->y * vector->y + vector->z * vector->z;
        float b = -2 * (vector->x * x + vector->y * y + vector->z * z);
        float c = x * x + y * y + z * z - radius * radius;

        // Calculate discriminant.
        float d = b * b - 4 * a * c;

        // Find closest intersection point.
        float t = -1;

        if (d >= 0)
        {
            if (-b + std::sqrt(d) > 0) t = (-b + std::sqrt(d)) / (2 * a);
            if (-b - std::sqrt(d) > 0) t = (-b - std::sqrt(d)) / (2 * a);
        }

        return t;
    }
};

// Class that contains info about whole scene (light sources and objects).
class Scene
{
public:
    Scene()
    {
        camera = NULL;
    }
    Scene(Camera* sceneCamera)
    {
        camera = sceneCamera;
    }

    void setCamera(Camera* newCamera)
    {
        // Free memory of current camera.
        if (camera) delete camera;

        camera = newCamera;
    }

    void addLight(Light* light)
    {
        lightSources.push_back(light);
    }

    void addObject(Object* object)
    {
        objects.push_back(object);
    }

    // Free all memory.
    void clear()
    {
        for (Light* l : lightSources)
        {
            if (l) delete l;
        }

        for (Object* o : objects)
        {
            if (o) delete o;
        }
    }

private:
    Camera* camera;
    std::vector<Light*> lightSources;
    std::vector<Object*> objects;
};

// Function prototypes.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
Scene createScene(int screenWidth, int screenHeight);
int renderScene(HWND hwnd, Scene* scene);
void showError(const std::wstring& wstrError);
void showValue(const std::wstring& wstrName, auto aValue);
