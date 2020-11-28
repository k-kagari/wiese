#ifndef WIESE_COMPTR_TYPEDEF_H_
#define WIESE_COMPTR_TYPEDEF_H_

#include <comdef.h>
#include <d2d1.h>
#include <dwrite.h>
#include <msctf.h>

#define COMPTR(i) _COM_SMARTPTR_TYPEDEF(i, __uuidof(i))
COMPTR(ID2D1Brush);
COMPTR(ID2D1Factory);
COMPTR(ID2D1HwndRenderTarget);
COMPTR(ID2D1SolidColorBrush);
COMPTR(IDWriteFactory);
COMPTR(IDWriteFont);
COMPTR(IDWriteFontCollection);
COMPTR(IDWriteFontFace);
COMPTR(IDWriteFontFamily);
COMPTR(ITfDocumentMgr);
COMPTR(ITfThreadMgr);
#undef COMPTR

#endif