#include "main_window.h"

#include <Windows.h>

#include <memory>

#include "comptr_typedef.h"
#include "edit_window.h"

namespace wiese {

MainWindow::MainWindow(HINSTANCE hinstance, ID2D1FactoryPtr d2d,
                       IDWriteFactoryPtr dwrite)
    : WindowBase(hinstance, L"WieseMainWindowClass", WS_EX_APPWINDOW, L"Wiese",
                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                 CW_USEDEFAULT, CW_USEDEFAULT, nullptr) {
  RECT rect;
  GetClientRect(hwnd(), &rect);
  edit_.emplace(hinstance, d2d, dwrite, hwnd(), rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top);
}

void MainWindow::OnSize(int width, int height) {
  MoveWindow(edit_->hwnd(), 0, 0, width, height, TRUE);
}

void MainWindow::OnSetFocus() { SetFocus(edit_->hwnd()); }

LRESULT MainWindow::WindowProcedure(HWND hwnd, UINT msg, WPARAM wp,
                                    LPARAM lp) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_SIZE:
      OnSize(LOWORD(lp), HIWORD(lp));
      return 0;
    case WM_SETFOCUS:
      OnSetFocus();
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace wiese
