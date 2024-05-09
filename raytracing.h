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

// Object identifiers.
#define ID_DEFAULT  1
#define ID_SPHERE   2
#define ID_MIRROR   3

// Struct that contains coordinates in 3D space.
struct Coordinates3D
{
    float x = 0;
    float y = 0;
    float z = 0;
};

// Struct that contains data about an object material.
struct Material
{
    COLORREF color = BG_COLOR;
};

// Struct that contains data about a screen.
struct Screen
{
    int width = 0;
    int height = 0;
};

// Base class that represents a point/vector in space.
class Primitive
{
public:
    Primitive() {}
    Primitive(float x, float y, float z) : coordinates({ x, y, z }) {}

    Coordinates3D getCoordinates()
    {
        return coordinates;
    }

    void moveTo(float x, float y, float z)
    {
        coordinates = { x, y, z };
    }

    // Calculate vector's length.
    float length()
    {
        return std::sqrt(coordinates.x * coordinates.x
            + coordinates.y * coordinates.y
            + coordinates.z * coordinates.z);
    }

    // Subtract points/vectors.
    Primitive operator-(Primitive point)
    {
        // Get vector coordinates.
        Coordinates3D pCoordinates = point.getCoordinates();

        return Primitive(
            coordinates.x - pCoordinates.x,
            coordinates.y - pCoordinates.y,
            coordinates.z - pCoordinates.z
        );
    }

    // Add points/vectors.
    Primitive operator+(Primitive point)
    {
        // Get vector coordinates.
        Coordinates3D pCoordinates = point.getCoordinates();

        return Primitive(
            coordinates.x + pCoordinates.x,
            coordinates.y + pCoordinates.y,
            coordinates.z + pCoordinates.z
        );
    }

    // Multiply the point/vector by a scalar.
    Primitive operator*(float t)
    {
        return Primitive(
            coordinates.x * t,
            coordinates.y * t,
            coordinates.z * t
        );
    }

    // Calculate a dot product of the current and passed vectors.
    float operator*(Primitive vector)
    {
        Coordinates3D vCoordinates = vector.getCoordinates();
        return coordinates.x * vCoordinates.x
            + coordinates.y * vCoordinates.y
            + coordinates.z * vCoordinates.z;
    }

protected:
    // Coordinates of the point/vector.
    Coordinates3D coordinates;
};

// Class that represents a camera.
class Camera : public Primitive
{
public:
    Camera() {}
    Camera(float x, float y, float z, Screen cameraScreen)
    {
        coordinates = { x, y, z };
        screen = cameraScreen;
    }

    Screen getScreen()
    {
        return screen;
    }

private:
    // Screen parameters.
    Screen screen;
};

// Base class that represents a 3D object.
class Object : public Primitive
{
public:
    Object() {}
    Object(float x, float y, float z, Material objectMaterial)
    {
        coordinates = { x, y, z };
        material = objectMaterial;
    }

    int getID() { return id; }

    Material getMaterial() { return material; }

    // Find the coefficient of intersection point between the object and a vector.
    virtual float intersect(Primitive* vector)
    {
        // Get vector's coordinates.
        Coordinates3D vCoordinates = vector->getCoordinates();

        // Calculate the relation of Object's x and vector's x.
        float t = coordinates.x / vCoordinates.x;

        // Check if the relation is the same for every coordinate.
        // If so, the vector intersects with the object.
        if (t == coordinates.y / vCoordinates.y && t == coordinates.z / vCoordinates.z)
            return t;

        return -1;
    }

protected:
    int id = ID_DEFAULT;
    // Material parameters.
    Material material;
};

// Class that represents a round reflective surface.
class Mirror : public Object
{
public:
    Mirror()
    {
        id = ID_MIRROR;
        radius = 0;
    }
    Mirror(float x, float y, float z, Primitive normalVector, float mirrorRadius)
    {
        id = ID_MIRROR;
        coordinates = { x, y, z };
        radius = mirrorRadius;

        // Normalize the vector.
        normal = normalVector * (1 / normalVector.length());
    }

    float intersect(Primitive* vector) override
    {
        // Find the closest intersection point from the intersection equation.
        // Mirror origin: { x0, y0, z0 }
        // Vector: { x, y, z }
        // Normal vector: { A, B, C }
        // Mirror surface: A(x0 - x*t) + B(y0 - y*t) + C(z0 - z*t) = 0
        // Intersection point: { x*t, y*t, z*t }

        // Get required coordinates.
        Coordinates3D nc = normal.getCoordinates();
        Coordinates3D vc = vector->getCoordinates();

        // Calculate the coefficient and find the intersection point.
        float t = (nc.x * coordinates.x + nc.y * coordinates.y + nc.z * coordinates.z)
            / (nc.x * vc.x + nc.y * vc.y + nc.z * vc.z);

        Primitive intersection = *vector * t;

        // Check if the intersection point lies on mirror.
        if (intersection.length() > radius) return -1;

        return t;
    }

    Primitive reflect(Primitive* vector)
    {
        return *vector - (normal * (*vector * normal)) * 2;
    }

private:
    // Normal vector that sets the direction.
    Primitive normal;
    float radius;
};

// Class that represents a sphere object.
class Sphere : public Object
{
public:
    Sphere()
    {
        id = ID_SPHERE;
        radius = 0;
    }
    Sphere(float x, float y, float z, float sphereRadius, Material objectMaterial)
    {
        id = ID_SPHERE;
        coordinates = { x, y, z };
        material = objectMaterial;
        radius = sphereRadius;
    }

    float intersect(Primitive* vector) override
    {
        // Get vector's coordinates.
        Coordinates3D vCoordinates = vector->getCoordinates();

        // Calculate the coefficients of the intersection equation.
        // Sphere: x^2 + y^2 + z^2 = R^2
        // Vector: { x, y, z }
        // Intersection point: { x*t, y*t, z*t }
        float a = vCoordinates.x * vCoordinates.x
            + vCoordinates.y * vCoordinates.y
            + vCoordinates.z * vCoordinates.z;
        float b = -2 * (vCoordinates.x * coordinates.x
            + vCoordinates.y * coordinates.y
            + vCoordinates.z * coordinates.z);
        float c = coordinates.x * coordinates.x
            + coordinates.y * coordinates.y
            + coordinates.z * coordinates.z - radius * radius;

        // Calculate the discriminant.
        float d = b * b - 4 * a * c;

        // Find the closest intersection point.
        float t = -1;

        if (d >= 0)
        {
            if (-b + std::sqrt(d) > 0)
                t = (-b + std::sqrt(d)) / (2 * a);
            if (-b - std::sqrt(d) > 0)
                t = (-b - std::sqrt(d)) / (2 * a);
        }

        return t;
    }

private:
    // Sphere parameters.
    float radius;
};

// Class that represents point light.
class Light : public Primitive
{
public:
    Light()
    {
        color = power = 0;
    }
    Light(float x, float y, float z, COLORREF lightColor, float lightPower)
    {
        coordinates = { x, y, z };
        color = lightColor;
        power = lightPower;
    }

    // Calculate exposure of object's point to light.
    float countLight(Primitive* objectPoint, Primitive* objectCenter)
    {
        // Create the vector that points from the light source to a point.
        Primitive lightToPoint = *this - *objectPoint;

        // Create the vector that points from the point to sphere's center.
        Primitive pointToCenter = *objectPoint - *objectCenter;

        // Calculate cosine of angle between created vectors.
        float cos = lightToPoint * pointToCenter / (lightToPoint.length() * pointToCenter.length());

        if (cos > 0 && cos < 1) return cos;

        return 0;
    }

    // Calculate the color of an object in light.
    COLORREF lightColor(Object* object, float coefficient)
    {
        Material objectMaterial = object->getMaterial();

        // COLORREF (typedef DWORD) format: 0x00BBGGRR
        COLORREF newObjectColor = 0;

        for (int i = 0; i <= 16; i += 8)
        {
            // Get light and object color channels.
            int lightChannel = (color >> i) % 256;
            int objectChannel = (objectMaterial.color >> i) % 256;

            // Create a new color channel.
            int newObjectChannel = (lightChannel + objectChannel) * power * coefficient;

            if (newObjectChannel > 0xFF) newObjectChannel = 0xFF;

            // Add the color channel to final color.
            newObjectColor |= newObjectChannel << i;
        }

        return newObjectColor;
    }

private:
    // Light parameters.
    COLORREF color;
    float power;
};

// Class that contains the whole scene (light sources and objects).
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

    Camera* getCamera() { return camera; }

    std::vector<Light*> getLightSources() { return lightSources; }

    std::vector<Object*> getObjects() { return objects; }

    void addLight(Light* light) { lightSources.push_back(light); }

    void addObject(Object* object) { objects.push_back(object); }

    // Free all memory.
    void clear()
    {
        for (Light* l : lightSources)
            if (l) delete l;

        for (Object* o : objects)
            if (o) delete o;
    }

private:
    // Scene parts.
    Camera* camera;
    std::vector<Light*> lightSources;
    std::vector<Object*> objects;
};

// Function prototypes.
// Rendering window procedure.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Initialize rendering.
// Return value:
//     Screen object.
Screen initRender(
    HWND hwnd,       // [in] handle to the rendering window.
    PAINTSTRUCT* ps, // [in, out] pointer to PAINTSTRUCT used for rendering.
    HDC* hdc         // [in, out] pointer to HDC used for rendering.
);
// Shutdown rendering.
// Return value:
//     0 - success.
//     1 - failure.
int shutRender(
    HWND hwnd,      // [in] handle to the rendering window.
    PAINTSTRUCT* ps // [in] pointer to PAINTSTRUCT used for rendering.
);
// Create a scene to render.
// Return value:
//     Scene object.
Scene createScene(
    Screen screen   // [in] screen parameters for a camera setup.
);
// Render a scene.
// Return value:
//     0 - success.
//     1 - failure.
int renderScene(
    Scene* scene,   // [in] scene that should be rendered.
    HDC hdc         // [in] initialized structure for rendering.
);
// Render pixel with provided color.
// Return value:
//     0 - success.
//     1 - failure.
int renderPixel(
    HDC hdc,            // [in] HDC used for rendering.
    int x,              // [in] horizontal position of the pixel.
    int y,              // [in] vertical position of the pixel.
    COLORREF lightColor // [in] color of the pixel.
);
// Find closest object to the camera.
// Return value:
//     Pointer to Object.
Object* findClosest(
    float* tMin,                    // [in, out] pointer to the coefficient of proximity.
    std::vector<Object*> objects,   // [in] array of objects in the scene.
    Primitive* ray                  // [in] ray whose interception points we are searching.
);
// Set color to the closest object according to lighting of the scene.
// Return value:
//     COLORREF color.
COLORREF lighten(
    Object* closestObject,            // [in] pointer to the closest object.
    std::vector<Light*> lightSources, // [in] array of light sources in the scene.
    Primitive* ray,                   // [in] ray whose interception points we are searching.
    float tMin                        // [in] coefficient of proximity.
);
// Show error message box with error code.
void showError(
    const std::wstring& wstrError   // [in] string to pring in message box.
);
