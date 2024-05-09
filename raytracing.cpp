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
        // Get window dimensions.
        RECT rect;

        if (!GetWindowRect(hwnd, &rect))
        {
            showError(L"renderScene::GetWindowRect");
            return -1;
        }

        int iWidth = rect.right - rect.left;
        int iHeight = rect.bottom - rect.top;
        
        // Create scene.
        Scene scene = createScene(iWidth, iHeight);

        // Render created scene
        renderScene(hwnd, &scene);

        // Cleanup.
        scene.clear();

        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

Scene createScene(int screenWidth, int screenHeight)
{
    Scene scene;

    // Set camera.
    Screen screen = { screenWidth, screenHeight };

    Camera* camera = new Camera{
        (float)screenWidth / 2,  // x
        (float)-screenHeight,    // y
        (float)screenHeight / 2, // z
        screen
    };

    scene.setCamera(camera);

    // Create light sources.
    Light* light1 = new Light{ -30, -30, -50, { 0x00000077 }, 1 };
    Light* light2 = new Light{ 30, 30, 50, { 0x0000FF00 }, 0.5 };

    scene.addLight(light1);
    scene.addLight(light2);

    // Create objects.
    Sphere* sphere1 = new Sphere{ 0, 13, -1, 4, { 0x000000FF } };
    Sphere* sphere2 = new Sphere{ -12, 30, 5, 4, { 0x00FF3300 } };
    Sphere* sphere3 = new Sphere{ 1, 5, -1, 1, { 0x0022FF55 } };

    scene.addObject(sphere1);
    scene.addObject(sphere2);
    scene.addObject(sphere3);

    return scene;
}

int renderScene(HWND hwnd, Scene* scene)
{
    // Prepare for drawing.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

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
            COLORREF pixelColor = BG_COLOR;
            COLORREF lightColor = 0;
            float tMin = -1;
            Object* closestObject = NULL;

            // Create ray that goes through point {x, y}.
            Primitive ray = Primitive(x, 0, y) - *camera;

            // Find the closest object.
            for (Object* object : objects)
            {
                float t;

                // Typecast object.
                switch (object->getID())
                {
                case ID_SPHERE:
                {
                    Sphere* sphere = dynamic_cast<Sphere*>(object);

                    // Check if ray intersects with sphere.
                    t = sphere->intersect(&ray);

                    break;
                }
                default:
                    t = -1;
                    break;
                }

                // Check if object is closer.
                if ((tMin > t || tMin < 0) && t > 0)
                {
                    tMin = t;
                    closestObject = object;
                }
            }

            // Light the closest object.
            for (Light* light : lightSources)
            {
                if (closestObject)
                {
                    // Get intersection point.
                    Primitive point = ray * tMin;

                    // Calculate light coefficient in intersection point.
                    float coefficient = light->countLight(&point, closestObject);

                    // Add light color to current pixel color.
                    lightColor += light->lightColor(closestObject, coefficient);
                }
            }

            // Dram pixel.
            SetPixel(hdc, x, y, pixelColor + lightColor);
        }
    }

    // Cleanup.
    EndPaint(hwnd, &ps);

    return 0;
}

void showError(const std::wstring& wstrError)
{
    std::wstringstream wsstr;

    wsstr << wstrError << L". Error: " << GetLastError()
        << L" (0x" << std::hex << GetLastError() << L")" << std::endl;

    std::wstring wstr = wsstr.str();

    MessageBox(NULL, wstr.c_str(), L"Error", MB_OK);
}
