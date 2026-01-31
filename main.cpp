#include <windows.h>
#include <iostream>
#include <time.h>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>


/*
    tileHeight = screenHeight / MatrixHeight (ex. 1080 / 45 -> 24)          default 27
    tileWidth = screenWidth / MatrixWidth (ex. 1920 / 80 -> 24)             default 48
    Red Channel Intensivity (0...255)                                       default 255
    Green Channel Intensivity (0...255)                                     default 116
    Blue Channel Intensivity (0...255)                                      default 0
    Latency (16ms -> ~60FPS, 32ms -> ~30FPS)                                defalut 32
    Update frame skip   (1 turn off frame skip)                             default 1
*/

//      To jest projekt jedynie na moje w³asne widzimisie
//      Nie jest on zoptymalizowany :P

const unsigned int MatrixHeight = 54;
const unsigned int MatrixWidth = 96;
unsigned int maxR = 255;
unsigned int maxG = 133;
unsigned int maxB = 142;
static bool lerp = false;
const unsigned int latency = 32;
unsigned int color_switch = 0;

struct kolor {
    // example. if this is color 0 then lerp is from 0 -> 1.
    // last index lerps into first one
    unsigned int R;
    unsigned int G;
    unsigned int B;
};
class Paleta {
private:
    const unsigned int cR = 255, cG = 133, cB = 142;
    unsigned int paletes[8];
    kolor rgb[8];
public:
    Paleta(unsigned int p, unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned int p5, unsigned int p6, unsigned int p7) {
        paletes[0] = p; paletes[1] = p1; paletes[2] = p2; paletes[3] = p3; paletes[4] = p4; paletes[5] = p5; paletes[6] = p6; paletes[7] = p7;

        for (int i = 0; i < 8; i++) {
            rgb[i].R = (paletes[i] >> 16);
            rgb[i].G = (paletes[i] >> 8 & 0xFF);
            rgb[i].B = (paletes[i] & 0xFF);
        }
    }

    void lerpHex(int s, int t) const {
        int e = (s < 7) ? (s + 1) : 0;
        maxR = static_cast<int>(static_cast<float>(rgb[s].R) + (static_cast<float>(rgb[e].R) - static_cast<float>(rgb[s].R)) * static_cast<float>(t / 50.f));
        maxG = static_cast<int>(static_cast<float>(rgb[s].G) + (static_cast<float>(rgb[e].G) - static_cast<float>(rgb[s].G)) * static_cast<float>(t / 50.f));
        maxB = static_cast<int>(static_cast<float>(rgb[s].B) + (static_cast<float>(rgb[e].B) - static_cast<float>(rgb[s].B)) * static_cast<float>(t / 50.f));
    }

    void restoreColor() const {
        maxR = cR; maxG = cG; maxB = cB;
    }
    void switchColor() const {
        maxR = rgb[0].R; maxG = rgb[0].G; maxB = rgb[0].B;
    }
};
//  (0xffadad, 0xffd6a5, 0xfdffb6, 0xcaffbf, 0x9bf6ff, 0xa0c4ff, 0xbdb2ff, 0xffc6ff);
//  (0x2c4875, 0x58508d, 0x8a508f, 0xbc5090, 0xde5a79, 0xff6361, 0xff8531, 0xffa600);
//  (0x000000, 0x12003d, 0x210063, 0x00285c, 0x00406b, 0xffffff, 0x3d0d3b, 0xd10065);
//  (0x000000, 0xe5383b, 0x010f28, 0xfca311, 0xf7f7f7, 0x22cad6, 0x721cb3, 0x8822d6);
//  (0x000e96, 0x7a2aa8, 0xff00b2, 0x009629, 0x24cb15, 0x48ff00, 0x3770e0, 0x1441f4);
//  (0xdd4f9d, 0xeda3de, 0xffccff, 0xf28ac5, 0xffbce1, 0xef1087, 0xffc6f4, 0xd836a7);
//  (0x000000, 0xd61111, 0x0ce3ff, 0x6aff0c, 0x3211d6, 0xffffff, 0xffc60e, 0xd67e11);


Paleta current(0x2c4875, 0x58508d, 0x8a508f, 0xbc5090, 0xde5a79, 0xff6361, 0xff8531, 0xffa600);
int T = 0, S = 0;

HDC hdc_main = NULL;
HDC hdc_comp = NULL;

DWORD* window_bmp_p = NULL;
HBITMAP whole_screen = NULL;
BITMAPINFO bmi = {};
PAINTSTRUCT ps;

int screenw = 0;
int screenh = 0;

std::mt19937_64 mt1(26173679);
std::mt19937_64 mt3(911420);
std::mt19937_64 mt4(58008);

static bool showFPS = false;
static int fps = 0;
static int frameCount = 0;
static std::chrono::steady_clock::time_point fpsLastTime;

class Matrix {
private:
    int Fire[MatrixHeight][MatrixWidth];
public:
    Matrix() {
        for (int y = 0; y < MatrixHeight; ++y) {
            for (int x = 0; x < MatrixWidth; ++x) {
                Fire[y][x] = 0;
            }
        }
        for (int x = 0; x < MatrixWidth; ++x) {
            Fire[MatrixHeight - 1][x] = 255;
        }
    }

    void Update() {
        int heatLost = 0;
        for (int y = 0; y < MatrixHeight-1; ++y) {
            for (int x = 0; x < MatrixWidth; ++x) {
                heatLost = 0;
                int val = Fire[y+1][x];
                if (y <= (MatrixHeight * 55 / 100)) {

                    heatLost += 1 + (static_cast<int>(135 / MatrixHeight));
                    if (x < ((MatrixWidth/8)+ mt4() % (MatrixWidth / 5)) || x > MatrixWidth - ((MatrixWidth / 8) + mt4() % (MatrixWidth / 5))) { heatLost += mt3() % 3; }
                }
                if (mt1() % 100 < 40) { heatLost += 4 + (4) * (static_cast<int>(135 / MatrixHeight)); }
                Fire[y][x] = (val <= heatLost) ? 0 : (val - heatLost);
            }
        }
    }

    int GetValueR(int x, int y) {
        if (x < 0 || x >= MatrixWidth || y < 0 || y >= MatrixHeight) return 0;
        if (Fire[y][x] > 255) { Fire[y][x] = 255; }
        if (Fire[y][x] < 0) { Fire[y][x] = 0; }
        return ((100*Fire[y][x])/255);
    }

    int GetValueG(int x, int y) {
        if (x < 0 || x >= MatrixWidth || y < 0 || y >= MatrixHeight) return 0;
        int value3 = abs((static_cast<int>(MatrixWidth/2)) - x + 3) + (y <= (MatrixHeight * 6 / 10) ? 2 : 0);
        if (value3 < 0) { value3 = 0; }
        return value3;
    }
};
static Matrix fireMatrix;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MakeBitMap(HWND hwnd, int Width, int Height)
{
    HDC hdc = GetDC(hwnd);

    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = Width;
    bmi.bmiHeader.biHeight = -Height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = GetDeviceCaps(hdc, BITSPIXEL);
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    if (whole_screen)
    {
        SelectObject(hdc_comp, GetStockObject(NULL_BRUSH)); // odpi¹æ jeœli wybrany
        DeleteObject(whole_screen);
        whole_screen = NULL;
        window_bmp_p = NULL;
    }

    whole_screen = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&window_bmp_p, NULL, 0);

    ReleaseDC(hwnd, hdc);
}

void Prepare_Screen(HWND hwnd)
{
    screenw = GetSystemMetrics(SM_CXSCREEN);
    screenh = GetSystemMetrics(SM_CYSCREEN);

    MakeBitMap(hwnd, screenw, screenh);

    if (hdc_main) ReleaseDC(hwnd, hdc_main);
    hdc_main = GetDC(hwnd);

    if (hdc_comp) DeleteDC(hdc_comp);
    hdc_comp = CreateCompatibleDC(hdc_main);

    SelectObject(hdc_comp, whole_screen);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"TheWindowClass";

    WNDCLASSW wc = { 0 };
    //wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

   /* WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;*/

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, //WS_EX_TRANSPARENT,
        CLASS_NAME,
        L"Fake Fire Simulation",
        WS_POPUP | WS_VISIBLE,
        //WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        //CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) return 0;

    // przygotuj ekran i timer animacji
    Prepare_Screen(hwnd);
    ShowWindow(hwnd, nCmdShow);
    fpsLastTime = std::chrono::steady_clock::now();



    // timer co oko³o 30 FPS (~13ms)
    SetTimer(hwnd, 1, latency, NULL);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg)
    {
    case WM_SIZE:
    {
        int newW = LOWORD(lParam);
        int newH = HIWORD(lParam);
        if (newW > 0 && newH > 0)
        {
            screenw = newW;
            screenh = newH;
            MakeBitMap(hwnd, screenw, screenh);
            SelectObject(hdc_comp, whole_screen);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_TIMER:
    {
        frameCount++;

        auto now = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsLastTime).count();

        if (diff >= 1000) {
            fps = frameCount;
            frameCount = 0;
            fpsLastTime = now;
        }

        if (window_bmp_p) {
            fireMatrix.Update();
            
            if (lerp) {
                if (T > 50) { T = 0; S++; }
                if (S > 7) { S = 0; }
                current.lerpHex( S, T );
                T++;
            }

            const int tileW = screenw / MatrixWidth;
            const int tileH = screenh / MatrixHeight;

            for (int ty = 0; ty < MatrixHeight; ++ty)
            {
                for (int tx = 0; tx < MatrixWidth; ++tx)
                {
                    unsigned value = fireMatrix.GetValueR(tx, ty);
                    unsigned value2 = fireMatrix.GetValueG(tx, ty);

                    int red = (maxR * value) / 100 - ((color_switch % 3 == 2) ? value2 : 0);
                    int green = (maxG * value) / 100 - ((color_switch % 3 == 0) ? value2 : 0);
                    int blue = (maxB * value) / 100 - ((color_switch % 3 == 1) ? value2 : 0);

                    if (red > 255) { red = 255; } if (red < 0) { red = 0; }
                    if (green > 255) { green = 255; } if (green < 0) { green = 0; }
                    if (blue > 255) { blue = 255; } if (blue < 0) { blue = 0; }

                    DWORD color = RGB(red, green, blue);

                    int startX = tx * tileW;
                    int startY = ty * tileH;

                    for (int py = 0; py < tileH; ++py)
                    {
                        DWORD* row = window_bmp_p + (startY + py) * screenw + startX;
                        for (int px = 0; px < tileW; ++px)
                        { row[px] = color; }
                    }
                }
            }

        }
        InvalidateRect(hwnd, NULL, FALSE); // wymuœ WM_PAINT
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps_local;
        HDC hdc = BeginPaint(hwnd, &ps_local);
        if (hdc_comp && whole_screen)
        {
            BitBlt(hdc, 0, 0, screenw, screenh, hdc_comp, 0, 0, SRCCOPY);

            if (showFPS) {
                SetBkMode(hdc, TRANSPARENT);

                std::wstring fpsText = L"FPS: " + std::to_wstring(fps);

                int x = 10;
                int y = 10;
                SetTextColor(hdc, RGB(0, 0, 0));
                TextOutW(hdc, x - 1, y, fpsText.c_str(), fpsText.length());
                TextOutW(hdc, x + 1, y, fpsText.c_str(), fpsText.length());
                TextOutW(hdc, x, y - 1, fpsText.c_str(), fpsText.length());
                TextOutW(hdc, x, y + 1, fpsText.c_str(), fpsText.length());
                TextOutW(hdc, x - 1, y - 1, fpsText.c_str(), fpsText.length());
                TextOutW(hdc, x + 1, y - 1, fpsText.c_str(), fpsText.length());
                TextOutW(hdc, x - 1, y + 1, fpsText.c_str(), fpsText.length());
                TextOutW(hdc, x + 1, y + 1, fpsText.c_str(), fpsText.length());
                SetTextColor(hdc, RGB(100, 255, 100));
                TextOutW(hdc, x, y, fpsText.c_str(), fpsText.length());
            }
        }
        EndPaint(hwnd, &ps_local);
        return 0;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_NUMPAD7) {
            showFPS = !showFPS;
        }
        if (wParam == VK_NUMPAD8 && !lerp) {
            ++color_switch;
            if (color_switch >= 3) { color_switch = 0; }
            unsigned temp = maxB;
            maxB = maxG;
            maxG = maxR;
            maxR = temp;
        }
        if (wParam == VK_NUMPAD9) {
            lerp = !lerp;
            if (!lerp) { T = 0; S = 0; current.restoreColor(); }
        }
        if (wParam == VK_ESCAPE || wParam == VK_BACK) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_DESTROY:
    {
        KillTimer(hwnd, 1);
        if (hdc_comp) { DeleteDC(hdc_comp); hdc_comp = NULL; }
        if (whole_screen) { DeleteObject(whole_screen); whole_screen = NULL; }
        if (hdc_main) { ReleaseDC(hwnd, hdc_main); hdc_main = NULL; }
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}