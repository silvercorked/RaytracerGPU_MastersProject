module;

// WIN32_LEAN_AND_MEAN // may want this later
#include <Windows.h>

export module Bitmap;

import <string>;

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
