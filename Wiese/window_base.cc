#include "window_base.h"

#include <Windows.h>
#include <winrt/base.h>

#include "exception.h"

namespace wiese {

void WindowBase::RegisterWindowClass(HINSTANCE hinstance,
                                     const wchar_t* class_name) {
  WNDCLASSEX wc;
  if (!GetClassInfoExW(hinstance, class_name, &wc)) {
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = WindowProcedureEntryPoint;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinstance;
    wc.hIcon = nullptr;
    wc.hCursor = static_cast<HCURSOR>(LoadImageW(
        nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = class_name;
    wc.hIconSm = nullptr;
    winrt::check_bool(RegisterClassExW(&wc));
  }
}

WindowBase::WindowBase(HINSTANCE hinstance, const wchar_t* class_name,
                       DWORD ex_style, const wchar_t* title, DWORD style, int x,
                       int y, int width, int height, HWND parent) {
  RegisterWindowClass(hinstance, class_name);
  hwnd_ = CreateWindowExW(ex_style, class_name, title, style, x, y, width,
                          height, parent, nullptr, hinstance, nullptr);
  winrt::check_pointer(hwnd_);
  SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

WindowBase::~WindowBase() {}

LRESULT CALLBACK WindowBase::WindowProcedureEntryPoint(HWND hwnd, UINT msg,
                                                       WPARAM wp, LPARAM lp) {
  try {
    WindowBase* thisp =
        reinterpret_cast<WindowBase*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (thisp) return thisp->WindowProcedure(hwnd, msg, wp, lp);
  } catch (...) {
    SaveExceptionForRethrow();
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace wiese
