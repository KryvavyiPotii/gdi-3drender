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

    // Find distance between current and passed points.
    virtual float distance(Primitive* point)
    {
        return std::sqrt(x * point->x + y * point->y + z * point->z);
    }
};

// Base class that represents vector.
class Vector : public Primitive
{
public:
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
private:
    float distance(Primitive* point) override
    {
        return length();
    }
};

// Class to store light info.
class Light : public Vector
{
public:
    // Light parameters.
    COLORREF color;
    float power;

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
    COLORREF lightColor(Object* object)
    {
        // COLORREF (typedef DWORD) format: 0x00BBGGRR
        COLORREF newObjectColor = 0;

        for (int i = 0; i <= 16; i += 8)
        {
            // Get color channel.
            char lightChannel = (color >> i) % 255;
            char objectChannel = (object->color >> i) % 255;

            // Create new color channel.
            char newObjectChannel = std::round((lightChannel + objectChannel) * power);

            if (newObjectChannel > 0xFF) newObjectChannel = 0xFF;

            // Add new color channel to final color.
            newObjectColor |= newObjectChannel << i;
        }

        return newObjectColor;
    }
};

// Base class that represents 3D object.
class Object : public Primitive
{
public:
    // Material parameters.
    COLORREF color;

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

// Class to store sphere info.
class Sphere : public Object
{
public:
    // Sphere parameters.
    float radius;

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

// Function prototypes.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int drawScene(HWND hwnd);
std::vector<std::vector<Primitive*>*>* createScene();
void deleteScene(std::vector<std::vector<Primitive*>*>* scene);
void showError(const std::wstring& wstrError);
void showValue(const std::wstring& wstrName, auto aValue);