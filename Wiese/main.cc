#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include <memory>

#include "comptr_typedef.h"
#include "main_window.h"

namespace {}  // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR /*lpCmdLine*/,
                    int nCmdShow) {
  CoInitialize(nullptr);

  ID2D1FactoryPtr d2d;
  HRESULT hr =
      D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_ID2D1Factory,
                        nullptr, reinterpret_cast<void**>(&d2d));
  if (FAILED(hr)) return 1;

  IDWriteFactoryPtr dwrite;
  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                           reinterpret_cast<IUnknown**>(&dwrite));
  if (FAILED(hr)) return 1;

  std::unique_ptr<wiese::MainWindow> window =
      wiese::MainWindow::Create(hInstance, d2d, dwrite);
  if (!window) return 1;
  ShowWindow(window->Handle(), nCmdShow);
  UpdateWindow(window->Handle());

  MSG msg;
  while (BOOL ret = GetMessageW(&msg, nullptr, 0, 0)) {
    if (ret == -1) break;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}

#ifdef _DEBUG

#include <gtest/gtest.h>

int wmain(int argc, wchar_t** argv) {
  testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();
  return wWinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOWNORMAL);
}

#endif
