#include "raytracing.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Class creation and registration.
    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = WINDOW_CLASS;

    if (!RegisterClass(&wc))
    {
        showError(L"wWinMain::RegisterClass");
        return -1;
    }

    // Create the window.
    HWND hwnd;

    hwnd = CreateWindow(
        WINDOW_CLASS, WINDOW_TITLE,
        WS_OVERLAPPED | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    if (hwnd == NULL)
    {
        showError(L"wWinMain::CreateWindow");
        return -1;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.
    MSG msg = { };

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {   
        // Initialize rendering.
        PAINTSTRUCT ps;
        HDC hdc;

        Screen screen = initRender(hwnd, &ps, &hdc);

        if (screen.height == 0 && screen.width == 0)
        {
            return -1;
        }

        // Create a scene.
        Scene scene = createScene(screen);

        // Render the scene.
        renderScene(&scene, hdc);

        // Cleanup.
        scene.clear();

        // Shutdown rendering.
        shutRender(hwnd, &ps);

        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

Screen initRender(HWND hwnd, PAINTSTRUCT* ps, HDC* hdc)
{
    // Prepare for drawing.
    *hdc = BeginPaint(hwnd, ps);

    if (!(*hdc))
    {
        showError(L"initRender::BeginPaint");
        return { 0, 0 };
    }

    // Get window dimensions.
    RECT rect;

    if (!GetWindowRect(hwnd, &rect))
    {
        showError(L"initRender::GetWindowRect");
        return { 0, 0 };
    }

    Screen screen(rect.right - rect.left, rect.bottom - rect.top);

    return screen;
}

int shutRender(HWND hwnd, PAINTSTRUCT* ps)
{
    EndPaint(hwnd, ps);

    return 0;
}

Scene createScene(Screen screen)
{
    Scene scene;

    // Set a camera.
    Camera* camera = new Camera{
        (float)screen.width / 2,  // x
        (float)-screen.height,    // y
        (float)screen.height / 2, // z
        screen
    };

    scene.setCamera(camera);

    // Create light sources.
    Light* light1 = new Light{ -30, -30, -50, { 0x00000077 }, 1 };
    Light* light2 = new Light{ 30, 30, 50, { 0x0000FF00 }, 0.5 };

    scene.addLight(light1);
    scene.addLight(light2);

    // Create objects.
    /*
    Sphere* sphere1 = new Sphere{ 0, 13, -1, 4, { 0x000000FF } };
    Sphere* sphere2 = new Sphere{ -12, 30, 5, 4, { 0x00FF3300 } };
    Sphere* sphere3 = new Sphere{ 1, 5, -1, 1, { 0x0022FF55 } };

    scene.addObject(sphere1);
    scene.addObject(sphere2);
    scene.addObject(sphere3);
    */
    Sphere* sphere1 = new Sphere{ 4, 13, 0, 2, { 0x000000FF } };
    Sphere* sphere2 = new Sphere{ 3, 11, 3, 0.5, { 0x00FF0000 } };
    Mirror* mirror = new Mirror{ -7, 20, 0, {-12, 1, 0}, 150 };

    scene.addObject(sphere1);
    scene.addObject(sphere2);
    scene.addObject(mirror);

    return scene;
}

int renderScene(Scene* scene, HDC hdc)
{
    // Get scene parameters and parts.
    Camera* camera = scene->getCamera();
    Screen screen = camera->getScreen();
    std::vector<Light*> lightSources = scene->getLightSources();
    std::vector<Object*> objects = scene->getObjects();

    // Loop though every pixel.
    for (int x = 0; x < screen.width; x++)
    {
        for (int y = 0; y < screen.height; y++)
        {
            // Create a ray that goes through the point {x, y}.
            Primitive ray = Primitive(x, 0, y) - *camera;

            // Find the closest object.
            float tMin = -1;
            Object* closestObject = findClosest(&tMin, objects, &ray);
            
            // Check reflections and lighten the object.
            COLORREF lightColor = 0;

            if (closestObject)
            {
                if (closestObject->getID() == ID_MIRROR)
                {
                    Mirror* mirror = dynamic_cast<Mirror*>(closestObject);

                    // Get reflected ray.
                    ray = mirror->reflect(&ray);

                    // Find the closest object in reflection.
                    closestObject = findClosest(&tMin, objects, &ray);
                }

                // Lighten the closest object.
                lightColor = lighten(closestObject, lightSources, &ray, tMin);
            }

            // Dram the pixel on the screen.
            if (renderPixel(hdc, x, y, lightColor) < 0)
            {
                showError(L"renderScene::renderPixel");
                return -1;
            }
        }
    }

    return 0;
}

int renderPixel(HDC hdc, int x, int y, COLORREF lightColor)
{
    COLORREF color = lightColor;

    SetPixel(hdc, x, y, color);

    return 0;
}

Object* findClosest(float* tMin, std::vector<Object*> objects, Primitive* ray)
{
    Object* closestObject = NULL;

    for (Object* object : objects)
    {
        float t;

        // Typecast the object properly.
        switch (object->getID())
        {
        case ID_SPHERE:
        {
            Sphere* sphere = dynamic_cast<Sphere*>(object);

            t = sphere->intersect(ray);

            break;
        }
        case ID_MIRROR:
        {
            Mirror* mirror = dynamic_cast<Mirror*>(object);

            t = mirror->intersect(ray);

            break;
        }
        default:
            t = -1;
            break;
        }

        // Check if the object is closer than the current one.
        if ((*tMin > t || *tMin < 0) && t > 0)
        {
            *tMin = t;
            closestObject = object;
        }
    }

    return closestObject;
}

COLORREF lighten(Object* closestObject, std::vector<Light*> lightSources, Primitive* ray, float tMin)
{
    COLORREF lightColor = 0;

    if (closestObject)
    {
        for (Light* light : lightSources)
        {
            // Get the intersection point.
            Primitive point = *ray * tMin;

            // Calculate light coefficient in the intersection point.
            float coefficient = light->countLight(&point, closestObject);

            // Add light color to the current pixel color.
            lightColor += light->lightColor(closestObject, coefficient);
        }
    }

    return lightColor;
}

void showError(const std::wstring& wstrError)
{
    std::wstringstream wsstr;

    wsstr << wstrError << L". Error: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring wstr = wsstr.str();

    MessageBox(NULL, wstr.c_str(), L"Error", MB_OK);
}
