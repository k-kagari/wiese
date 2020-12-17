#include "edit_window.h"

#include <Windows.h>
#include <winrt/base.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <optional>
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
  ID2D1SolidColorBrushPtr brush;
  winrt::check_hresult(render_target->CreateSolidColorBrush(
      D2D1::ColorF(D2D1::ColorF::Black), &brush));
  render_target_ = std::move(render_target);
  brush_ = std::move(brush);
}

void EditWindow::DiscardDeviceResources() {
  brush_ = nullptr;
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

}  // namespace

void EditWindow::DrawLines() {
  const float line_height = CalculateLineHeight(font_metrics_, kFontEmSize);
  float x_offset = 0;
  int line_number = 0;
  for (auto it = document_.PieceIteratorBegin();
       it != document_.PieceIteratorEnd(); ++it) {
    if (it->IsLineBreak()) {
      ++line_number;
      x_offset = 0.0f;
      continue;
    }
    x_offset += DrawString(document_.GetCharsInPiece(*it), x_offset,
                           line_height * line_number);
  }
}

float EditWindow::DrawString(std::wstring_view text, float x, float y) {
  std::unique_ptr<std::uint16_t[]> glyph_indices =
      StringToGlyphIndices(text, font_face_);

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
  render_target_->DrawGlyphRun(D2D1::Point2F(x, y_offset), &glyph_run, brush_);

  return MeasureGlyphIndicesWidth(glyph_indices.get(), text.size());
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
  if (selection_.IsSinglePoint() && selection_.Point() == SelectionPoint(0, 0)) {
    scaled_api_.SetCaretPos(0, 0);
    return;
  }

  //int pos = 0;
  int line_count = 0;
  int offset = 0;
  std::optional<Piece> piece_before_caret;
  Document::PieceListIterator it;
  for (it = document_.PieceIteratorBegin(); it != document_.PieceIteratorEnd();
       ++it) {
    offset += it->GetCharCount();
    if (it->IsLineBreak()) {
      ++line_count;
      offset = 0;
    }
    if (line_count == selection_.Point().line) {
      if (selection_.Point().offset == offset) {
        piece_before_caret = *it;
        break;
      }
      if (selection_.Point().offset < offset) {
        piece_before_caret = it->Slice(0, offset - selection_.Point().offset);
        break;
      }
    }
  }
  assert(piece_before_caret.has_value());

  float x;
  if (piece_before_caret->IsLineBreak()) {
    x = 0.0f;
  } else {
    x = MeasureStringWidth(document_.GetCharsInPiece(*piece_before_caret));
    while (it != document_.PieceIteratorBegin() && !(--it)->IsLineBreak()) {
      x += MeasureStringWidth(document_.GetCharsInPiece(*it));
    }
  }

  scaled_api_.SetCaretPos(static_cast<int>(x),
                          static_cast<int>(line_height * line_count));
}

float EditWindow::DesignUnitsToWindowCoordinates(UINT32 design_unit) {
  return static_cast<float>(design_unit) / font_metrics_.designUnitsPerEm *
         kFontEmSize;
}

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

  // DrawString(document_.GetText());
  DrawLines();

  HRESULT hr = render_target_->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) {
    DiscardDeviceResources();
    InvalidateRect(hwnd(), nullptr, FALSE);
  }

  ShowCaret(hwnd());
}

void EditWindow::OnKeyDown(char key) {
  key;
  /*
  switch (key) {
    case VK_BACK: {
      if (selection_.Point() == SelectionPoint(0, 0)) return;
      document_.EraseCharAt(selection_.MovePointBack());
      InvalidateRect(hwnd(), nullptr, FALSE);
      UpdateCaretPosition();
      return;
    }
    case VK_RETURN: {
      document_.InsertLineBreakBefore(selection_.Point());
      selection_.MovePointForward();
      InvalidateRect(hwnd(), nullptr, FALSE);
      UpdateCaretPosition();
      return;
    }
    case VK_LEFT: {
      if (selection_.Point() > 0) {
        if (!IsKeyPressed(VK_SHIFT)) {
          selection_.MovePointBack();
          UpdateCaretPosition();
        } else {
        }
      }
      return;
    }
    case VK_RIGHT: {
      if (selection_.Point() < document_.GetCharCount()) {
        selection_.MovePointForward();
        UpdateCaretPosition();
      }
      return;
    }
    case VK_DELETE: {
      if (document_.GetCharCount() == selection_.Point()) return;
      document_.EraseCharAt(selection_.Point());
      InvalidateRect(hwnd(), nullptr, FALSE);
      UpdateCaretPosition();
      return;
    }
  }
  */
}

void EditWindow::OnChar(wchar_t ch) {
  if (ch == 0x08 || ch == 0x0d) return;
//  document_.InsertCharBefore(ch, selection_.Point());
//  selection_.MovePointForward();
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
