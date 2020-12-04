#ifndef WIESE_MAIN_WINDOW_H_
#define WIESE_MAIN_WINDOW_H_

#include <Windows.h>
#include <d2d1.h>

#include <memory>
#include <optional>

#include "comptr_typedef.h"
#include "edit_window.h"
#include "window_base.h"

namespace wiese {

class EditWindow;

class MainWindow : public WindowBase {
 public:
  MainWindow(HINSTANCE hinstance, ID2D1FactoryPtr d2d,
             IDWriteFactoryPtr dwrite);

 private:
  void OnSize(int width, int height);
  void OnSetFocus();

  LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wp,
                                  LPARAM lp) override;

  std::optional<EditWindow> edit_;
};

}  // namespace wiese

#endif
