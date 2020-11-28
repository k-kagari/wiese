#ifndef WIESE_MAIN_WINDOW_H_
#define WIESE_MAIN_WINDOW_H_

#include <Windows.h>
#include <d2d1.h>

#include <memory>

#include "comptr_typedef.h"
#include "edit_window.h"

namespace wiese {

class EditWindow;

class MainWindow {
 public:
  static std::unique_ptr<MainWindow> Create(HINSTANCE hinstance,
                                            ID2D1FactoryPtr d2d,
                                            IDWriteFactoryPtr dwrite);

  HWND Handle() const { return hwnd_; }

 private:
  MainWindow(HWND hwnd, std::unique_ptr<EditWindow> edit)
      : hwnd_(hwnd), edit_(std::move(edit)) {}

  void OnSize(int width, int height);
  void OnSetFocus();

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                  LPARAM lparam);

  HWND hwnd_;
  std::unique_ptr<EditWindow> edit_;

  static ATOM class_atom_;
};

}  // namespace wiese

#endif
