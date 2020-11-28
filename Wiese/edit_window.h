#ifndef WIESE_EDITWINDOW_H_
#define WIESE_EDITWINDOW_H_

#include <Windows.h>
#include <comdef.h>
#include <d2d1.h>

#include <memory>
#include <string_view>

#include "comptr_typedef.h"
#include "document.h"

namespace wiese {

class EditWindow {
 public:
  ~EditWindow();
  static std::unique_ptr<EditWindow> Create(ID2D1FactoryPtr d2d,
                                            IDWriteFactoryPtr dwrite,
                                            HWND parent, int x, int y,
                                            int width, int height);

  HWND Handle() const { return hwnd_; }

 private:
  EditWindow(ID2D1FactoryPtr d2d, IDWriteFactoryPtr dwrite,
             ITfThreadMgrPtr tf_thread_manager,
             ITfDocumentMgrPtr tf_document_manager, HWND hwnd);

  void CreateDirect2DResources();
  void DrawLines();
  void DrawString(std::wstring_view text, int y_offset);
  void UpdateCaretPosition();
  float DesignUnitsToWindowCoordinates(UINT32 design_unit);

  void OnSetFocus();
  void OnKillFocus();
  void OnPaint();
  void OnKeyDown(char key);
  void OnChar(wchar_t character);

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                  LPARAM lparam);

  ID2D1FactoryPtr d2d_;
  IDWriteFactoryPtr dwrite_;
  HWND hwnd_;
  static ATOM class_atom_;

  ID2D1HwndRenderTargetPtr render_target_;
  IDWriteFontFacePtr font_face_;
  DWRITE_FONT_METRICS font_metrics_;
  ID2D1SolidColorBrushPtr brush_;

  ITfThreadMgrPtr tf_thread_manager_;
  TfClientId tf_client_id_;
  ITfDocumentMgrPtr tf_document_manager_;

  Document document_;

  class Selection {
   public:
    Selection() : start_(0), end_(0) {}
    int MoveForward() {
      assert(IsSinglePoint());
      return start_ = ++end_;
    }
    int MoveBack() {
      assert(IsSinglePoint());
      assert(end_ - 1 >= 0);
      return start_ = --end_;
    }
    int Position() const {
      assert(IsSinglePoint());
      return start_;
    }
    bool IsSinglePoint() const { return start_ == end_; }
    bool IsRange() const { return start_ != end_; }

   private:
    int start_;
    int end_;
  };
  Selection selection_;

  static constexpr int kFontEmSize = 32;
};

}  // namespace wiese

#endif
