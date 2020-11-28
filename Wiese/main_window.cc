#include "main_window.h"

#include <Windows.h>

#include <memory>

#include "comptr_typedef.h"
#include "edit_window.h"

namespace wiese {

ATOM MainWindow::class_atom_ = 0;

std::unique_ptr<MainWindow> MainWindow::Create(HINSTANCE hinstance,
                                               ID2D1FactoryPtr d2d,
                                               IDWriteFactoryPtr dwrite) {
  static const wchar_t* const kClassName = L"WieseMainWindowClass";
  if (!class_atom_) {
    WNDCLASSEX wc;
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinstance;
    wc.hIcon = nullptr;
    wc.hCursor = static_cast<HCURSOR>(LoadImageW(
        nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = kClassName;
    wc.hIconSm = nullptr;
    class_atom_ = RegisterClassExW(&wc);
    if (!class_atom_) return nullptr;
  }

  HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, kClassName, L"Wiese",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr,
                              hinstance, nullptr);
  if (!hwnd) return nullptr;

  RECT rect;
  GetClientRect(hwnd, &rect);
  std::unique_ptr<EditWindow> edit =
      EditWindow::Create(d2d, dwrite, hwnd, rect.left, rect.top,
                         rect.right - rect.left, rect.bottom - rect.top);
  if (!edit) return nullptr;

  std::unique_ptr<MainWindow> window(new MainWindow(hwnd, std::move(edit)));
  SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                    reinterpret_cast<LONG_PTR>(window.get()));
  return window;
}

void MainWindow::OnSize(int width, int height) {
  MoveWindow(edit_->Handle(), 0, 0, width, height, TRUE);
}

void MainWindow::OnSetFocus() { SetFocus(edit_->Handle()); }

namespace {

MainWindow* GetThis(HWND hwnd) {
  return reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

}  // namespace

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_SIZE:
      GetThis(hwnd)->OnSize(LOWORD(lparam), HIWORD(lparam));
      return 0;
    case WM_SETFOCUS:
      GetThis(hwnd)->OnSetFocus();
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace wiese
