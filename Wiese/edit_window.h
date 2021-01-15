#ifndef WIESE_EDITWINDOW_H_
#define WIESE_EDITWINDOW_H_

#include <Windows.h>
#include <comdef.h>
#include <d2d1.h>

#include <memory>
#include <string_view>

#include "comptr_typedef.h"
#include "document.h"
#include "util.h"
#include "window_base.h"

namespace wiese {

struct SelectionPoint {
  int line;
  int column;

  SelectionPoint() : line(0), column(0) {}
  SelectionPoint(int line, int column) : line(line), column(column) {}
  bool operator==(const SelectionPoint& rhs) const {
    return line == rhs.line && column == rhs.column;
  }
  bool operator!=(const SelectionPoint& rhs) const { return !operator==(rhs); }
  bool operator<(const SelectionPoint& rhs) const {
    if (line > rhs.line) return false;
    return line < rhs.line || column < rhs.column;
  }
  bool operator>(const SelectionPoint& rhs) const {
    if (line < rhs.line) return false;
    return line > rhs.line || column > rhs.column;
  }
  bool operator<=(const SelectionPoint& rhs) const { return !operator>(rhs); }
};

struct Selection {
 public:
  SelectionPoint caret_pos;
  SelectionPoint anchor;

  Selection() : caret_pos(), anchor() {}
  Selection(const SelectionPoint& caret_pos, const SelectionPoint& anchor)
      : caret_pos(caret_pos), anchor(anchor) {}
  void SetCaretAndAnchorLine(int line) { caret_pos.line = anchor.line = line; }
  void SetCaretAndAnchorColumn(int column) {
    caret_pos.column = anchor.column = column;
  }
  bool HasRange() const { return caret_pos != anchor; }
};

class EditWindow : public WindowBase {
 public:
  EditWindow(HINSTANCE hinstance, ID2D1FactoryPtr d2d, IDWriteFactoryPtr dwrite,
             HWND parent, int x, int y, int width, int height);
  ~EditWindow();

 private:
  void CreateDeviceResources();
  void DiscardDeviceResources();
  void DrawLines();
  float DrawString(std::wstring_view text, float x, float y, ID2D1BrushPtr background_brush);
  float MeasureGlyphIndicesWidth(const std::uint16_t* indices, int count);
  float MeasureStringWidth(std::wstring_view string);
  void UpdateCaretPosition();
  float DesignUnitsToWindowCoordinates(UINT32 design_unit);

  void MoveSelectionPointBack(SelectionPoint& point);
  void MoveSelectionPointForward(SelectionPoint& point);
  void DeleteSelectedText();

  void OnSetFocus();
  void OnKillFocus();
  void OnPaint();
  void OnKeyDown(char key);
  void OnChar(wchar_t ch);

  LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) override;

  static constexpr int kFontEmSize = 16;

  ID2D1FactoryPtr d2d_;
  IDWriteFactoryPtr dwrite_;
  ScaledAPI scaled_api_;

  ID2D1HwndRenderTargetPtr render_target_;
  IDWriteFontFacePtr font_face_;
  DWRITE_FONT_METRICS font_metrics_;
  ID2D1SolidColorBrushPtr text_brush_;
  ID2D1SolidColorBrushPtr selection_background_brush_;

  ITfThreadMgrPtr tf_thread_manager_;
  TfClientId tf_client_id_;
  ITfDocumentMgrPtr tf_document_manager_;

  Document document_;
  Selection selection_;
};

}  // namespace wiese

#endif
