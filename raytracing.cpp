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
        drawScene(hwnd);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int drawScene(HWND hwnd)
{
    // Get window dimensions.
    RECT rect;

    if (!GetWindowRect(hwnd, &rect))
    {
        showError(L"drawScene::GetWindowRect");
        return -1;
    }

    int iWidth = rect.right - rect.left;
    int iHeight = rect.bottom - rect.top;

    // Prepare for drawing.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    // Get light sources and objects.
    std::vector<std::vector<Primitive*>*>* scene = createScene();
    //std::vector<Light*>* lightSources = dynamic_cast<std::vector<Light*>*>(scene->at(0));

    // Set camera.
    Primitive camera;

    camera.x = iWidth / 2;
    camera.y = -iHeight;
    camera.z = iHeight / 2;

    // Loop though every pixel.
    for (int x = 0; x < iWidth; x++)
    {
        for (int y = 0; y < iHeight; y++)
        {
            // Create ray that goes through point {x, y}.
            Vector ray;

            ray.x = x - camera.x;
            ray.y = -camera.y;
            ray.z = y - camera.z;

            for (Primitive* l : *(scene->at(0)))
            {
                Light* light = dynamic_cast<Light*>(l);

                for (Primitive* o : *(scene->at(1)))
                {
                    Object* object = dynamic_cast<Object*>(o);

                    // Check if ray intersects with sphere.
                    float t = sphere.intersect(&ray);

                    if (t != -1)
                    {
                        // Get intersection point.
                        ray.x *= t;
                        ray.y *= t;
                        ray.z *= t;

                        // Calculate light coefficient in intersection point.
                        float l = light.countLight(&ray, &sphere);

                        // Calculate color in intersection point based on light.
                        COLORREF color = lightColor(&sphere, l);

                        // Draw color.
                        SetPixel(hdc, x, y, color);
                    }
                    else
                    {
                        // If ray doesn't intersect with sphere, draw background color.
                        SetPixel(hdc, x, y, BG_COLOR);
                    }
                }
            }
        }
    }

    // Cleanup.
    deleteScene(scene);
    EndPaint(hwnd, &ps);

    return 0;
}

std::vector<std::vector<Primitive*>*>* createScene()
{
    // Create light sources.
    Light* light = new Light;

    light->x = -3;
    light->y = 1;
    light->z = 5;

    std::vector<Primitive*>* lightSources = new std::vector<Primitive*>;

    lightSources->push_back(light);

    // Create objects.
    Sphere* sphere = new Sphere;

    sphere->x = 10;
    sphere->y = 13;
    sphere->z = -1;
    sphere->radius = 4;
    sphere->color = 0x000000FF;

    std::vector<Primitive*>* objects = new std::vector<Primitive*>;

    objects->push_back(sphere);

    // Store all primitives in one array (vector).
    std::vector<std::vector<Primitive*>*>* scene = new std::vector<std::vector<Primitive*>*>;

    // Format: scene[0] - array of lightsources, scene[i > 0] - anything.
    scene->push_back(lightSources);
    scene->push_back(objects);

    return scene;
}

void deleteScene(std::vector<std::vector<Primitive*>*>* scene)
{
    if (scene)
    {
        // Free memory of all subarrays.
        for (auto arr : *scene)
        {
            // Free memory of all primitives.
            if (arr)
            {
                for (auto p : *arr)
                {
                    if (p) delete p;
                }
            }

            // Free memory of subarray.
            delete arr;
        }

        // Free memory of array.
        delete scene;
    }
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