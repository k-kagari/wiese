#include "edit_window.h"

#include <Windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

#include "text_store.h"
#include "util.h"

namespace {

ATOM RegisterWindowClass(const wchar_t* class_name, HINSTANCE hinstance,
                         WNDPROC wndproc) {
  WNDCLASSEX wc;
  wc.cbSize = sizeof(wc);
  wc.style = 0;
  wc.lpfnWndProc = wndproc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hinstance;
  wc.hIcon = nullptr;
  wc.hCursor = static_cast<HCURSOR>(LoadImageW(
      nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = class_name;
  wc.hIconSm = nullptr;
  return RegisterClassExW(&wc);
}

}  // namespace

namespace wiese {

EditWindow::EditWindow(ID2D1FactoryPtr d2d, IDWriteFactoryPtr dwrite,
                       ITfThreadMgrPtr tf_thread_manager,
                       ITfDocumentMgrPtr tf_document_manager, HWND hwnd)
    : d2d_(d2d),
      dwrite_(dwrite),
      tf_thread_manager_(tf_thread_manager),
      tf_document_manager_(tf_document_manager),
      tf_client_id_(),
      font_metrics_(),
      hwnd_(hwnd),
      document_(L"0123456789") {}

ATOM EditWindow::class_atom_ = 0;

std::unique_ptr<EditWindow> EditWindow::Create(ID2D1FactoryPtr d2d,
                                               IDWriteFactoryPtr dwrite,
                                               HWND parent, int x, int y,
                                               int width, int height) {
  static const wchar_t* const kClassName = L"WieseEditWindowClass";

  HINSTANCE hinstance =
      reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));

  if (!class_atom_) {
    class_atom_ = RegisterWindowClass(kClassName, hinstance, WndProc);
    if (!class_atom_) return nullptr;
  }

  HWND hwnd =
      CreateWindowExW(0, kClassName, L"WieseEditWindow", WS_CHILD | WS_VISIBLE,
                      x, y, width, height, parent, nullptr, hinstance, nullptr);
  if (!hwnd) return nullptr;

  ITfThreadMgrPtr thread_manager;
  HRESULT hr = CoCreateInstance(CLSID_TF_ThreadMgr, nullptr,
                                CLSCTX_INPROC_SERVER, IID_ITfThreadMgr,
                                reinterpret_cast<void**>(&thread_manager));
  if (FAILED(hr)) return nullptr;

  ITfDocumentMgrPtr document_manager;
  hr = thread_manager->CreateDocumentMgr(&document_manager);
  if (FAILED(hr)) return nullptr;

  std::unique_ptr<EditWindow> window(
      new EditWindow(d2d, dwrite, thread_manager, document_manager, hwnd));
  SetWindowLongPtr(hwnd, GWLP_USERDATA,
                   reinterpret_cast<LONG_PTR>(window.get()));
  thread_manager->Activate(&window->tf_client_id_);
  window->CreateDirect2DResources();

  IDWriteFontCollectionPtr font_collection;
  dwrite->GetSystemFontCollection(&font_collection);

  UINT32 index;
  BOOL exists;
  font_collection->FindFamilyName(L"Meiryo", &index, &exists);
  if (!exists) return nullptr;

  IDWriteFontFamilyPtr font_family;
  font_collection->GetFontFamily(index, &font_family);
  if (!font_family) return nullptr;

  IDWriteFontPtr font;
  font_family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL,
                                    DWRITE_FONT_STRETCH_NORMAL,
                                    DWRITE_FONT_STYLE_NORMAL, &font);
  if (!font) return nullptr;

  IDWriteFontFacePtr font_face;
  font->CreateFontFace(&font_face);
  if (!font_face) return nullptr;
  window->font_face_ = font_face;
  font->GetMetrics(&window->font_metrics_);

  return window;
}

EditWindow::~EditWindow() { tf_thread_manager_->Deactivate(); }

void EditWindow::CreateDirect2DResources() {
  RECT rect;
  GetClientRect(hwnd_, &rect);
  D2D1_SIZE_U size =
      D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);
  d2d_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                               D2D1::HwndRenderTargetProperties(hwnd_, size),
                               &render_target_);
  render_target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                        &brush_);
}

namespace {

int CalculateLineHeight(const DWRITE_FONT_METRICS& metrics, int em) {
  const int line_spacing = metrics.ascent + metrics.descent + metrics.lineGap;
  return static_cast<int>(static_cast<float>(line_spacing) /
                          metrics.designUnitsPerEm * em);
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

}  // namespace

void EditWindow::DrawLines() {
  std::wstring text = document_.GetText();
  std::array<wchar_t, 1> crlf = {L'\n'};

  auto it = text.begin();
  int y_offset = 0;
  for (;;) {
    auto line_end = std::search(it, text.end(), crlf.begin(), crlf.end());
    std::wstring_view line(&*it, std::distance(it, line_end));
    DrawString(line, y_offset);
    if (line_end == text.end()) break;
    it = line_end + 2;
    y_offset += CalculateLineHeight(font_metrics_, kFontEmSize);
  }
}

void EditWindow::DrawString(std::wstring_view text, int y_offset) {
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

  float y = static_cast<float>(font_metrics_.ascent) /
                font_metrics_.designUnitsPerEm * kFontEmSize +
            y_offset;
  render_target_->DrawGlyphRun(D2D1::Point2F(0.0f, y), &glyph_run, brush_);
}

void EditWindow::UpdateCaretPosition() {
  std::wstring text = document_.GetText();
  int start_pos_of_line = 0;
  int line_count = 0;
  for (int i = 0; i < selection_.Position(); ++i) {
    if (text[i] == L'\r') {
      if (i + 1 == selection_.Position()) break;
      if (text[i + 1] == L'\n') ++i;
      start_pos_of_line = i + 1;
      ++line_count;
    }
  }
  text = text.substr(start_pos_of_line, selection_.Position());

  std::unique_ptr<std::uint16_t[]> glyph_indices =
      StringToGlyphIndices(text, font_face_);
  auto glyph_metrics = std::make_unique<DWRITE_GLYPH_METRICS[]>(text.size());
  font_face_->GetDesignGlyphMetrics(glyph_indices.get(), text.size(),
                                    glyph_metrics.get());
  float x = 0.0f;
  for (int i = 0; i < static_cast<int>(text.size()); ++i) {
    x += DesignUnitsToWindowCoordinates(glyph_metrics[i].advanceWidth);
  }
  const float y = DesignUnitsToWindowCoordinates(
      font_metrics_.ascent + font_metrics_.descent + font_metrics_.lineGap);
  DPIScaler scaler(hwnd_);
  scaler.SetCaretPos(static_cast<int>(x), static_cast<int>(y * line_count));
}

float EditWindow::DesignUnitsToWindowCoordinates(UINT32 design_unit) {
  return static_cast<float>(design_unit) / font_metrics_.designUnitsPerEm *
         kFontEmSize;
}

void EditWindow::OnSetFocus() {
  int kCaretWidth = 1;
  DPIScaler scaler(hwnd_);
  scaler.CreateCaret(hwnd_, nullptr, kCaretWidth,
                     CalculateLineHeight(font_metrics_, kFontEmSize));
  scaler.SetCaretPos(0, 0);
  ShowCaret(hwnd_);
}

void EditWindow::OnKillFocus() { DestroyCaret(); }

void EditWindow::OnPaint() {
  HideCaret(hwnd_);

  ValidateRect(hwnd_, nullptr);

  render_target_->BeginDraw();
  render_target_->Clear(D2D1::ColorF(D2D1::ColorF::FloralWhite, 1.0f));

  // DrawString(document_.GetText());
  DrawLines();

  HRESULT hr = render_target_->EndDraw();
  if (hr == D2DERR_RECREATE_TARGET) CreateDirect2DResources();

  ShowCaret(hwnd_);
}

void EditWindow::OnKeyDown(char key) {
  switch (key) {
    case VK_BACK: {
      if (selection_.Position() == 0) return;
      document_.EraseCharAt(selection_.MoveBack());
      /*
      wchar_t ch = document_.EraseCharAt(selection_.MoveBack());
      // If the removed character was LF, look for leading CR.
      if (ch == L'\n' && selection_.Position() > 0) {
        ch = document_.GetCharAt(selection_.Position() - 1);
        if (ch == L'\r') {
          document_.EraseCharAt(selection_.MoveBack());
        }
      }
      */
      InvalidateRect(hwnd_, nullptr, FALSE);
      UpdateCaretPosition();
      return;
    }
    case VK_RETURN: {
      document_.InsertLineBreakBefore(selection_.Position());
      selection_.MoveForward();
      InvalidateRect(hwnd_, nullptr, FALSE);
      return;
    }
    case VK_LEFT: {
      if (selection_.Position() > 0) {
        int pos = selection_.MoveBack();
        /*
        if (pos > 0 && document_.GetCharAt(pos) == L'\n' &&
            document_.GetCharAt(pos - 1) == L'\r')
          selection_.MoveBack();
        */
        UpdateCaretPosition();
      }
      return;
    }
    case VK_RIGHT: {
      if (selection_.Position() < document_.GetCharCount()) {
        wchar_t ch = document_.GetCharAt(selection_.Position());
        selection_.MoveForward();
        /*
        if (ch == L'\r' && document_.GetCharAt(selection_.Position()) == L'\n')
          selection_.MoveForward();
        */
        UpdateCaretPosition();
      }
      return;
    }
    case VK_DELETE: {
      if (document_.GetCharCount() == selection_.Position()) return;
      document_.EraseCharAt(selection_.Position());
      InvalidateRect(hwnd_, nullptr, FALSE);
      UpdateCaretPosition();
      return;
    }
  }
}

void EditWindow::OnChar(wchar_t ch) {
  if (ch == 0x08 || ch == 0x0d) return;
  document_.InsertCharBefore(ch, selection_.Position());
  selection_.MoveForward();
  InvalidateRect(hwnd_, nullptr, FALSE);
  UpdateCaretPosition();
}

namespace {

EditWindow* GetThis(HWND hwnd) {
  return reinterpret_cast<EditWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

}  // namespace

LRESULT CALLBACK EditWindow::WndProc(HWND hwnd, UINT msg, WPARAM wparam,
                                     LPARAM lparam) {
  switch (msg) {
    case WM_SETFOCUS:
      GetThis(hwnd)->OnSetFocus();
      return 0;
    case WM_KILLFOCUS:
      GetThis(hwnd)->OnKillFocus();
      return 0;
    case WM_PAINT: {
      GetThis(hwnd)->OnPaint();
      return 0;
    }
    case WM_ERASEBKGND:
      return 1;
    case WM_KEYDOWN:
      GetThis(hwnd)->OnKeyDown(static_cast<char>(wparam));
      return 0;
    case WM_CHAR:
      GetThis(hwnd)->OnChar(static_cast<wchar_t>(wparam));
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

}  // namespace wiese
