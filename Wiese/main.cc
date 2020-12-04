#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include <exception>
#include <memory>

#include "comptr_typedef.h"
#include "exception.h"
#include "main_window.h"

namespace {

std::exception_ptr saved_exception;

}  // namespace

void SaveExceptionForRethrow() noexcept {
  saved_exception = std::current_exception();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR /*lpCmdLine*/,
                    int nCmdShow) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr))
    return 1;

  ID2D1FactoryPtr d2d;
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_ID2D1Factory,
                        nullptr, reinterpret_cast<void**>(&d2d));
  if (FAILED(hr)) return 1;

  IDWriteFactoryPtr dwrite;
  hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                           reinterpret_cast<IUnknown**>(&dwrite));
  if (FAILED(hr)) return 1;

  wiese::MainWindow window(hInstance, d2d, dwrite);
  ShowWindow(window.hwnd(), nCmdShow);
  UpdateWindow(window.hwnd());

  MSG msg;
  while (BOOL ret = GetMessageW(&msg, nullptr, 0, 0)) {
    if (saved_exception)
      std::rethrow_exception(saved_exception);
    if (ret == -1) break;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return 0;
}

#if defined(_DEBUG) && !defined(UNITTEST)

#include <gtest/gtest.h>

int wmain(int argc, wchar_t** argv) {
  testing::InitGoogleTest(&argc, argv);
  RUN_ALL_TESTS();
  return wWinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOWNORMAL);
}

#endif
