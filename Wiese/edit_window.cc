#include "edit_window.h"

#include <Windows.h>
#include <winrt/base.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <string_view>
#include <vector>

#include "text_store.h"
#include "util.h"

namespace wiese {

EditWindow::EditWindow(HINSTANCE hinstance, ID2D1FactoryPtr d2d,
                       IDWriteFactoryPtr dwrite, HWND parent, int x, int y,
                       int width, int height)
    : WindowBase(hinstance, L"WieseEditWindowClass", 0, L"WieseEditWindow",
                 WS_CHILD | WS_VISIBLE, x, y, width, height, parent),
      d2d_(d2d),
      dwrite_(dwrite),
      document_(L"0123456789") {
  scaled_api_.SetDPI(GetDpiForWindow(hwnd()));

  winrt::check_hresult(tf_thread_manager_.CreateInstance(
      CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER));
  winrt::check_hresult(
      tf_thread_manager_->CreateDocumentMgr(&tf_document_manager_));
  tf_thread_manager_->Activate(&tf_client_id_);

  IDWriteFontCollectionPtr font_collection;
  dwrite->GetSystemFontCollection(&font_collection);

  UINT32 index;
  BOOL exists;
  font_collection->FindFamilyName(L"Meiryo", &index, &exists);
  winrt::check_bool(exists);

  IDWriteFontFamilyPtr font_family;
  font_collection->GetFontFamily(index, &font_family);
  winrt::check_pointer(font_family.GetInterfacePtr());

  IDWriteFontPtr font;
  font_family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL,
                                    DWRITE_FONT_STRETCH_NORMAL,
                                    DWRITE_FONT_STYLE_NORMAL, &font);
  winrt::check_pointer(font.GetInterfacePtr());

  font->CreateFontFace(&font_face_);
  winrt::check_pointer(font_face_.GetInterfacePtr());
  font->GetMetrics(&font_metrics_);
}

EditWindow::~EditWindow() { tf_thread_manager_->Deactivate(); }

void EditWindow::CreateDeviceResources() {
  RECT rect;
  GetClientRect(hwnd(), &rect);
  D2D1_SIZE_U size =
      D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);
  ID2D1HwndRenderTargetPtr render_target;
  winrt::check_hresult(d2d_->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(hwnd(), size), &render_target));
  ID2D1SolidColorBrushPtr text_brush;
  winrt::check_hresult(render_target->CreateSolidColorBrush(
      D2D1::ColorF(D2D1::ColorF::Black), &text_brush));
  ID2D1SolidColorBrushPtr selection_background_brush;
  winrt::check_hresult(render_target->CreateSolidColorBrush(
      D2D1::ColorF(D2D1::ColorF::LightCyan), &selection_background_brush));

  render_target_ = std::move(render_target);
  text_brush_ = std::move(text_brush);
  selection_background_brush_ = std::move(selection_background_brush);
}

void EditWindow::DiscardDeviceResources() {
  text_brush_ = nullptr;
  selection_background_brush_ = nullptr;
  render_target_ = nullptr;
}

namespace {

float CalculateLineHeight(const DWRITE_FONT_METRICS& metrics, int em) {
  const int line_spacing = metrics.ascent + metrics.descent + metrics.lineGap;
  return static_cast<float>(line_spacing) / metrics.designUnitsPerEm * em;
}

std::unique_ptr<std::uint16_t[]> StringToGlyphIndices(
    std::wstring_view text, IDWriteFontFacePtr font_face) {
  std::vector<std::uint32_t> codepoints(text.length());
  std::copy(text.begin(), text.end(), codepoints.begin());

  auto glyph_indices = std::make_unique<std::uint16_t[]>(codepoints.size());
  font_face->GetGlyphIndicesW(codepoints.data(), codepoints.size(),
                              glyph_indices.get());
  return glyph_indices;
}

bool IsKeyPressed(int key) { return GetKeyState(key) < 0; }

struct DrawStringOperation {
  std::wstring_view text;
  bool selected;
  DrawStringOperation(std::wstring_view text, bool selected)
      : text(text), selected(selected) {}
};

}  // namespace

void EditWindow::DrawLines() {
  const float line_height = CalculateLineHeight(font_metrics_, kFontEmSize);

  SelectionPoint selection_start;
  SelectionPoint selection_end;
  if (selection_.caret_pos < selection_.anchor) {
    selection_start = selection_.caret_pos;
    selection_end = selection_.anchor;
  } else {
    selection_start = selection_.anchor;
    selection_end = selection_.caret_pos;
  }

  std::queue<DrawStringOperation> queue;
  int line = 0;
  int column = 0;
  float x_offset = 0;
  bool in_selection = false;
  for (auto it = document_.PieceIteratorBegin();
       it != document_.PieceIteratorEnd(); ++it) {
    int char_count = it->GetCharCount();
    if (line == selection_start.line && column <= selection_start.column &&
        selection_start.column < column + char_count) {
      int start_point = selection_start.column - column;
      std::wstring_view string = document_.GetVisualCharsInPiece(*it);
      queue.push({string.substr(0, start_point), false});
      if (line == selection_end.line && column <= selection_end.column &&
          selection_end.column < column + char_count) {
        // I“_‚ª“¯ˆêPiece‚É‚ ‚Á‚½ê‡
        int end_point = selection_end.column - column;
        queue.push({string.substr(start_point, end_point - start_point), true});
        queue.push({string.substr(end_point), false});
      } else {
        in_selection = true;
        queue.push({
          string.substr(start_point), true});
      }
    } else if (in_selection) {
      if (line == selection_end.line && column <= selection_end.column &&
          selection_end.column < column + char_count) {
        int end_point = selection_end.column - column;
        std::wstring_view string = document_.GetCharsInPiece(*it);
        queue.push({string.substr(0, end_point), true});
        queue.push({string.substr(end_point), false});
        in_selection = false;
      }
    }

    if (queue.empty()) {
      queue.push({document_.GetVisualCharsInPiece(*it), in_selection});
    }

    while (!queue.empty()) {
      DrawStringOperation& op = queue.front();
      if (!op.text.empty()) {
        x_offset +=
            DrawString(op.text, x_offset, line_height * line,
                       op.selected ? selection_background_brush_ : nullptr);
      }
      queue.pop();
    }

    if (it->IsLineBreak()) {
      ++line;
      column = 0;
      x_offset = 0.0f;
    } else {
      column += char_count;
    }
  }
}

float EditWindow::DrawString(std::wstring_view text, float x, float y,
                             ID2D1BrushPtr background_brush) {
  std::unique_ptr<std::uint16_t[]> glyph_indices =
      StringToGlyphIndices(text, font_face_);
  float width = MeasureGlyphIndicesWidth(glyph_indices.get(), text.size());

  if (background_brush) {
    float height =
        static_cast<float>(font_metrics_.ascent + font_metrics_.descent) /
        font_metrics_.designUnitsPerEm * kFontEmSize;
    render_target_->FillRectangle(D2D1::RectF(x, y, x + width, y + height),
                                  background_brush);
  }

  DWRITE_GLYPH_RUN glyph_run;
  glyph_run.fontFace = font_face_;
  glyph_run.fontEmSize = kFontEmSize;
  glyph_run.glyphCount = text.size();
  glyph_run.glyphIndices = glyph_indices.get();
  glyph_run.glyphAdvances = nullptr;
  glyph_run.glyphOffsets = nullptr;
  glyph_run.isSideways = FALSE;
  glyph_run.bidiLevel = 0;
  float y_offset = y + static_cast<float>(font_metrics_.ascent) /
                           font_metrics_.designUnitsPerEm * kFontEmSize;
  render_target_->DrawGlyphRun(D2D1::Point2F(x, y_offset), &glyph_run,
                               text_brush_);
  return width;
}

float EditWindow::MeasureGlyphIndicesWidth(const std::uint16_t* indices,
                                           int count) {
  auto glyph_metrics = std::make_unique<DWRITE_GLYPH_METRICS[]>(count);
  font_face_->GetDesignGlyphMetrics(indices, count, glyph_metrics.get());
  float width = 0.0f;
  for (int i = 0; i < count; ++i) {
    width += DesignUnitsToWindowCoordinates(glyph_metrics[i].advanceWidth);
  }
  return width;
}

float EditWindow::MeasureStringWidth(std::wstring_view string) {
  std::unique_ptr<std::uint16_t[]> glyph_indices =
      StringToGlyphIndices(string, font_face_);
  return MeasureGlyphIndicesWidth(glyph_indices.get(), string.size());
}

void EditWindow::UpdateCaretPosition() {
  const float line_height = DesignUnitsToWindowCoordinates(
      font_metrics_.ascent + font_metrics_.descent + font_metrics_.lineGap);
  if (selection_.caret_pos.column == 0) {
    scaled_api_.SetCaretPos(
        0, static_cast<int>(line_height * selection_.caret_pos.line));
    return;
  }

  auto it = document_.PieceIteratorBegin();
  AdvanceByLine(it, selection_.caret_pos.line, document_.PieceIteratorEnd());
  int offset = 0;
  std::optional<Piece> piece_before_caret;
  for (; it != document_.PieceIteratorEnd(); ++it) {
    int end_of_current_piece = offset + it->GetCharCount();
    if (selection_.caret_pos.column == end_of_current_piece) {
      piece_before_caret = *it;
      break;
    }
    if (selection_.caret_pos.column < end_of_current_piece) {
      piece_before_caret = it->Slice(0, selection_.caret_pos.column - offset);
      break;
    }
    offset = end_of_current_piece;
  }
  float x = MeasureStringWidth(document_.GetCharsInPiece(*piece_before_caret));
  while (it != document_.PieceIteratorBegin() && !(--it)->IsLineBreak()) {
    x += MeasureStringWidth(document_.GetCharsInPiece(*it));
  }
  scaled_api_.SetCaretPos(
      static_cast<int>(x),
      static_cast<int>(line_height * selection_.caret_pos.line));
}

float EditWindow::DesignUnitsToWindowCoordinates(UINT32 design_unit) {
  return static_cast<float>(design_unit) / font_metrics_.designUnitsPerEm *
         kFontEmSize;
}

void EditWindow::MoveSelectionPointBack(SelectionPoint& point) {
  if (point.column == 0) {
    if (point.line > 0) {
      --point.line;

      auto it = document_.PieceIteratorBegin();
      auto end = document_.PieceIteratorEnd();
      AdvanceByLine(it, point.line, end);
      point.column = GetCharCountOfLine(it, end);
    }
  } else {
    --point.column;
  }
}

void EditWindow::MoveSelectionPointForward(SelectionPoint& point) {
  auto it = document_.PieceIteratorBegin();
  auto end = document_.PieceIteratorEnd();
  AdvanceByLine(it, point.line, end);
  if (point.column == GetCharCountOfLine(it, end)) {
    AdvanceByLine(it, 1, end);
    if (it != end) {
      ++point.line;
      point.column = 0;
    }
  } else {
    ++point.column;
  }
}

void EditWindow::DeleteSelectedText() {}

void EditWindow::OnSetFocus() {
  int kCaretWidth = 1;
  scaled_api_.CreateCaret(
      hwnd(), nullptr, kCaretWidth,
      static_cast<int>(CalculateLineHeight(font_metrics_, kFontEmSize)));
  scaled_api_.SetCaretPos(0, 0);
  ShowCaret(hwnd());
}

void EditWindow::OnKillFocus() { DestroyCaret(); }

void EditWindow::OnPaint() {
  HideCaret(hwnd());
  ValidateRect(hwnd(), nullptr);
  if (!render_target_) {
    CreateDeviceResources();
  }
  render_target_->BeginDraw();
  render_target_->Clear(D2D1::ColorF(D2D1::ColorF::FloralWhite, 1.0f));

  DrawLines();

  HRESULT hr = render_target_->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) {
    DiscardDeviceResources();
    InvalidateRect(hwnd(), nullptr, FALSE);
  }

  ShowCaret(hwnd());
}

void EditWindow::OnKeyDown(char key) {
  switch (key) {
    case VK_BACK: {
      if (selection_.HasRange()) {
        DeleteSelectedText();
      } else {
        if (selection_.caret_pos == SelectionPoint(0, 0)) return;
        MoveSelectionPointBack(selection_.caret_pos);
        selection_.anchor = selection_.caret_pos;
        document_.EraseCharAt(selection_.caret_pos.line,
                              selection_.caret_pos.column);
        InvalidateRect(hwnd(), nullptr, FALSE);
        UpdateCaretPosition();
      }
      return;
    }
    case VK_RETURN: {
      if (selection_.HasRange()) {
      } else {
        document_.InsertLineBreakBefore(selection_.caret_pos.line,
                                        selection_.caret_pos.column);
        selection_.SetCaretAndAnchorLine(selection_.caret_pos.line + 1);
        selection_.SetCaretAndAnchorColumn(0);
        InvalidateRect(hwnd(), nullptr, FALSE);
        UpdateCaretPosition();
      }
      return;
    }
    case VK_LEFT: {
      if (IsKeyPressed(VK_SHIFT)) {
        MoveSelectionPointBack(selection_.anchor);
        InvalidateRect(hwnd(), nullptr, FALSE);
      } else {
        MoveSelectionPointBack(selection_.caret_pos);
        selection_.anchor = selection_.caret_pos;
        UpdateCaretPosition();
      }
      return;
    }
    case VK_RIGHT: {
      if (IsKeyPressed(VK_SHIFT)) {
        MoveSelectionPointForward(selection_.anchor);
        InvalidateRect(hwnd(), nullptr, FALSE);
      } else {
        MoveSelectionPointForward(selection_.caret_pos);
        selection_.anchor = selection_.caret_pos;
        UpdateCaretPosition();
      }
      return;
    }
    case VK_DELETE: {
      if (selection_.HasRange()) {
      } else {
        if (selection_.caret_pos.line == document_.GetLineCount() - 1) {
          auto it = document_.PieceIteratorBegin();
          auto end = document_.PieceIteratorEnd();
          AdvanceByLine(it, selection_.caret_pos.line, end);
          if (selection_.caret_pos.column == GetCharCountOfLine(it, end))
            return;
        }
        document_.EraseCharAt(selection_.caret_pos.line,
                              selection_.caret_pos.column);
        InvalidateRect(hwnd(), nullptr, FALSE);
        UpdateCaretPosition();
      }
      return;
    }
  }
}

void EditWindow::OnChar(wchar_t ch) {
  if (ch == 0x08 || ch == 0x0d) return;
  document_.InsertCharBefore(ch, selection_.caret_pos.line,
                             selection_.caret_pos.column);
  selection_.SetCaretAndAnchorColumn(selection_.caret_pos.column + 1);
  InvalidateRect(hwnd(), nullptr, FALSE);
  UpdateCaretPosition();
}

LRESULT EditWindow::WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
    case WM_SETFOCUS:
      OnSetFocus();
      return 0;
    case WM_KILLFOCUS:
      OnKillFocus();
      return 0;
    case WM_PAINT:
      OnPaint();
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_KEYDOWN:
      OnKeyDown(static_cast<char>(wp));
      return 0;
    case WM_CHAR:
      OnChar(static_cast<wchar_t>(wp));
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace wiese
