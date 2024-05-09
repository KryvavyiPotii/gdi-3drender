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

// Struct that contains coordinates in 3D space.
struct Coordinates3D
{
    float x = 0;
    float y = 0;
    float z = 0;
};

// Struct that contains data about object material.
struct Material
{
    COLORREF color = BG_COLOR;
};

// Struct that contains data about screen.
struct Screen
{
    int width = 0;
    int height = 0;
};

// Base class that represents point/vector in space.
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

    // Calculate vector length.
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

    // Multiply point/vector by scalar.
    Primitive operator*(float t)
    {
        return Primitive(
            coordinates.x * t,
            coordinates.y * t,
            coordinates.z * t
        );
    }

    // Calculate dot product of current and passed vectors.
    float operator*(Primitive vector)
    {
        Coordinates3D vCoordinates = vector.getCoordinates();
        return coordinates.x * vCoordinates.x
            + coordinates.y * vCoordinates.y
            + coordinates.z * vCoordinates.z;
    }

protected:
    // Coordinates of point.
    Coordinates3D coordinates;
};

// Class that represents camera.
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

// Base class that represents 3D object.
class Object : public Primitive
{
public:
    Object() {}
    Object(float x, float y, float z, Material objectMaterial)
    {
        coordinates = { x, y, z };
        material = objectMaterial;
    }

    int getID()
    {
        return id;
    }

    Material getMaterial()
    {
        return material;
    }

    // Find coefficient of intersection point between object and vector.
    virtual float intersect(Primitive* vector)
    {
        // Get vector's coordinates.
        Coordinates3D vCoordinates = vector->getCoordinates();

        // Calculate relation of Object's x and vector's x.
        float t = coordinates.x / vCoordinates.x;

        // Check if relation is the same for every coordinate.
        // If so, vector intersects with object.
        if (t == coordinates.y / vCoordinates.y && t == coordinates.z / vCoordinates.z)
            return t;

        return -1;
    }

protected:
    int id = ID_DEFAULT;
    // Material parameters.
    Material material;
};

// Class to store sphere info.
class Sphere : public Object
{
public:
    // Sphere parameters.
    float radius;

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
        // Get vector coordinates.
        Coordinates3D vCoordinates = vector->getCoordinates();

        // Calculate coefficients of vector and sphere intersection equation.
        float a = vCoordinates.x * vCoordinates.x
            + vCoordinates.y * vCoordinates.y
            + vCoordinates.z * vCoordinates.z;
        float b = -2 * (vCoordinates.x * coordinates.x
            + vCoordinates.y * coordinates.y
            + vCoordinates.z * coordinates.z);
        float c = coordinates.x * coordinates.x
            + coordinates.y * coordinates.y
            + coordinates.z * coordinates.z - radius * radius;

        // Calculate discriminant.
        float d = b * b - 4 * a * c;

        // Find closest intersection point.
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
};

// Class to store light info.
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
        // Get vector 1 that points from light source to point.
        Primitive lightToPoint = *this - *objectPoint;

        // Get vector 2 that points from point to sphere's center.
        Primitive pointToCenter = *objectPoint - *objectCenter;

        // Calculate cosine of angle between vector 1 and 2.
        float cos = lightToPoint * pointToCenter / (lightToPoint.length() * pointToCenter.length());

        if (cos > 0 && cos < 1) return cos;

        return 0;
    }

    // Calculate color of object in light.
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

            // Create new color channel.
            int newObjectChannel = (lightChannel + objectChannel) * power * coefficient;

            if (newObjectChannel > 0xFF) newObjectChannel = 0xFF;

            // Add new color channel to final color.
            newObjectColor |= newObjectChannel << i;
        }

        return newObjectColor;
    }

private:
    // Light parameters.
    COLORREF color;
    float power;
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

    Camera* getCamera()
    {
        return camera;
    }

    std::vector<Light*> getLightSources()
    {
        return lightSources;
    }

    std::vector<Object*> getObjects()
    {
        return objects;
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
// Rendering window procedure.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
// Create a scene to render.
// Arguments:
//     [in] screenWidth - rendering window width.
//     [in] screenHeight - rendering window height.
//         From camera's point of view a window it is a screen.
// Return value:
//     Scene object
Scene createScene(int screenWidth, int screenHeight);
// Render a scene.
// Arguments:
//     [in] hwnd - handle to rendering window.
//     [in] scene - Scene object that should be rendered.
// Return value:
//     0 - success
//     1 - failure
int renderScene(HWND hwnd, Scene* scene);
// Show error message box with error code.
// Arguments:
//     [in] wstrError - string to pring in message box.
void showError(const std::wstring& wstrError);
