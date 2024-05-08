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
    Camera* camera = new Camera{
        (float)screenWidth / 2,  // x
        (float)-screenHeight,    // y
        (float)screenHeight / 2, // z
        screenWidth,             // screenWidth
        screenHeight             // screenHeight
    };

    scene.setCamera(camera);

    // Create light sources.
    Light* light1 = new Light{ -30, -30, -50, 0x00000077, 1 };
    Light* light2 = new Light{ 30, 30, 50, 0x0000FF00, 0.5 };

    scene.addLight(light1);
    scene.addLight(light2);

    // Create objects.
    Sphere* sphere1 = new Sphere{ 0, 13, -1, 0x000000FF, 4 };
    Sphere* sphere2 = new Sphere{ -12, 30, 5, 0x00FF3300, 4 };
    Sphere* sphere3 = new Sphere{ 1, 5, -1, 0x0022FF55, 1 };

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
    std::vector<Light*> lightSources = scene->getLightSources();
    std::vector<Object*> objects = scene->getObjects();

    // Loop though every pixel.
    for (int x = 0; x < camera->width; x++)
    {
        for (int y = 0; y < camera->height; y++)
        {
            COLORREF pixelColor = BG_COLOR;
            COLORREF lightColor = 0;

            // Create ray that goes through point {x, y}.
            Vector ray;

            ray.x = x - camera->x;
            ray.y = -camera->y;
            ray.z = y - camera->z;

            for (Light* light : lightSources)
            {
                float tMin = -1;

                for (Object* object : objects)
                {
                    float t;

                    // Typecast object.
                    switch (object->id)
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

                    if ((t > 0) && (tMin < 0) || t <= tMin) 
                    {
                        tMin = t;

                        // Get intersection point.
                        Primitive point = { ray.x * tMin, ray.y * tMin, ray.z * tMin };

                        // Calculate light coefficient in intersection point.
                        float coefficient = light->countLight(&point, object);

                        // Add light color to current pixel color.
                        if (t == tMin) lightColor += light->lightColor(object, coefficient);
                        else lightColor = light->lightColor(object, coefficient);
                    }
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

void showValue(const std::wstring& wstrName, auto aValue)
{
    std::wstringstream wssValue;

    wssValue << aValue << std::endl;

    std::wstring wsValue = wssValue.str();

    MessageBox(NULL, wsValue.c_str(), wstrName.c_str(), 0);
}
