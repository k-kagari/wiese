#ifndef WIESE_WINDOW_BASE_H_
#define WIESE_WINDOW_BASE_H_

#include <Windows.h>

namespace wiese {

class WindowBase {
 public:
  WindowBase(HINSTANCE hinstance, const wchar_t* class_name, DWORD ex_style,
             const wchar_t* title, DWORD style, int x, int y, int width,
             int height, HWND parent);
  virtual ~WindowBase();

  HWND hwnd() { return hwnd_; }

 private:
  virtual LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wp,
                                  LPARAM lp) = 0;
  void RegisterWindowClass(HINSTANCE hinstance, const wchar_t* class_name);
  static LRESULT CALLBACK WindowProcedureEntryPoint(HWND hwnd, UINT msg,
                                                    WPARAM wp, LPARAM lp);
  HWND hwnd_;
};

}  // namespace wiese

#endif
