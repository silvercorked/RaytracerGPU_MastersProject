module;

// WIN32_LEAN_AND_MEAN // may want this later
#include <Windows.h>
#include <string>

export module Bitmap;

import PrimitiveTypes;

export class Bitmap {
    HBITMAP bmp;
    HDC hdc;
    HPEN pen;
    HBRUSH brush;
    DWORD clr;
    void* pBits;
    i32 width, height, wid;

public:
    Bitmap();
    Bitmap(u32, u32, u8 = 0);
    ~Bitmap();
    HDC getDC() const;
    auto getWidth() const -> u32;
    auto getHeight() const -> u32;
    auto create(u32, u32) -> bool;
    auto clear(BYTE) -> void;
    auto setBrushColor(DWORD) -> void;
    auto setPenColor(DWORD) -> void;
    auto setPenWidth(u32) -> void;

    auto setPixel(u32, u32, COLORREF = BLACK) -> void;
    auto saveBitmap(std::string) const -> void;

    static constexpr COLORREF BLACK = RGB(0,0,0);
    static constexpr COLORREF RED = RGB(255, 0, 0);
    static constexpr COLORREF GREEN = RGB(0, 255, 0);
    static constexpr COLORREF BLUE = RGB(0, 0, 255);
private:
    void createPen();
};

Bitmap::Bitmap() : pen(NULL), brush(NULL), clr(0), wid(1) {}
Bitmap::Bitmap(u32 w, u32 h, u8 clearVal) :
    pen(NULL),
    brush(NULL),
    clr(0),
    wid(1)
{
    this->create(w, h);
    this->clear(clearVal);
}
Bitmap::~Bitmap() {
    DeleteObject(pen); DeleteObject(brush);
    DeleteDC(hdc); DeleteObject(bmp);
}

HDC Bitmap::getDC() const {
    return hdc;
}
auto Bitmap::getWidth() const -> u32 {
    return width;
}
auto Bitmap::getHeight() const -> u32 {
    return height;
}
auto Bitmap::create(u32 w, u32 h) -> bool {
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biBitCount = sizeof(DWORD) * 8;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -static_cast<i32>(h);

    HDC dc = GetDC(GetConsoleWindow());
    bmp = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!bmp) return false;

    hdc = CreateCompatibleDC(dc);
    SelectObject(hdc, bmp);
    ReleaseDC(GetConsoleWindow(), dc);

    width = w; height = h;
    return true;
}
auto Bitmap::createPen() -> void {
    if (pen) DeleteObject(pen);
    pen = CreatePen(PS_SOLID, wid, clr);
    SelectObject(hdc, pen);
}
auto Bitmap::setBrushColor(DWORD bClr) -> void {
    if (brush) DeleteObject(brush);
    brush = CreateSolidBrush(bClr);
    SelectObject(hdc, brush);
}
auto Bitmap::setPenColor(DWORD c) -> void {
    clr = c;
    createPen();
}
auto Bitmap::setPenWidth(u32 w) -> void {
    wid = w;
    createPen();
}
auto Bitmap::clear(BYTE clr = 0) -> void {
    memset(pBits, clr, width * height * sizeof(DWORD));
}
auto Bitmap::setPixel(u32 x, u32 y, COLORREF color) -> void {
    SetPixel(
        this->getDC(),
        x,
        y,
        color
    );
}
auto Bitmap::saveBitmap(std::string path) const -> void {
    BITMAPFILEHEADER fileheader;
    BITMAPINFO       infoheader;
    BITMAP           bitmap;
    DWORD            wb;

    GetObject(bmp, sizeof(bitmap), &bitmap);
    DWORD* dwpBits = new DWORD[bitmap.bmWidth * bitmap.bmHeight];

    ZeroMemory(dwpBits, bitmap.bmWidth * bitmap.bmHeight * sizeof(DWORD));
    ZeroMemory(&infoheader, sizeof(BITMAPINFO));
    ZeroMemory(&fileheader, sizeof(BITMAPFILEHEADER));

    infoheader.bmiHeader.biBitCount = sizeof(DWORD) * 8;
    infoheader.bmiHeader.biCompression = BI_RGB;
    infoheader.bmiHeader.biPlanes = 1;
    infoheader.bmiHeader.biSize = sizeof(infoheader.bmiHeader);
    infoheader.bmiHeader.biHeight = bitmap.bmHeight;
    infoheader.bmiHeader.biWidth = bitmap.bmWidth;
    infoheader.bmiHeader.biSizeImage = bitmap.bmWidth * bitmap.bmHeight * sizeof(DWORD);

    fileheader.bfType = 0x4D42;
    fileheader.bfOffBits = sizeof(infoheader.bmiHeader) + sizeof(BITMAPFILEHEADER);
    fileheader.bfSize = fileheader.bfOffBits + infoheader.bmiHeader.biSizeImage;

    GetDIBits(hdc, bmp, 0, height, (LPVOID)dwpBits, &infoheader, DIB_RGB_COLORS);

    std::wstring wStrPath = std::wstring(path.begin(), path.end()); // convert string to wstring
    LPCWSTR filePath = wStrPath.c_str();

    HANDLE file = CreateFile(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(file, &fileheader, sizeof(BITMAPFILEHEADER), &wb, NULL);
    WriteFile(file, &infoheader.bmiHeader, sizeof(infoheader.bmiHeader), &wb, NULL);
    WriteFile(file, dwpBits, bitmap.bmWidth * bitmap.bmHeight * 4, &wb, NULL);
    CloseHandle(file);

    delete[] dwpBits;
};