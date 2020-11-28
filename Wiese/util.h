#ifndef WIESE_UTIL_H_
#define WIESE_UTIL_H_

#include <Windows.h>

namespace wiese {

class DPIScaler {
 public:
  DPIScaler(HWND hwnd) { factor_ = GetDpiForWindow(hwnd) / 96.0f; }
  BOOL CreateCaret(HWND hWnd, HBITMAP hBitmap, int nWidth, int nHeight) {
    return ::CreateCaret(hWnd, hBitmap, Scale(nWidth), Scale(nHeight));
  }
  BOOL SetCaretPos(int X, int Y) { return ::SetCaretPos(Scale(X), Scale(Y)); }

 private:
  int Scale(int value) { return static_cast<int>(value * factor_); }
  float factor_;
};

}  // namespace wiese

#endif
