#ifndef WIESE_TEXT_STORE_H_
#define WIESE_TEXT_STORE_H_

#include <msctf.h>

#include <string>
#include <string_view>

namespace wiese {

class TextStore : public ITextStoreACP {
 public:
  TextStore(std::wstring_view text)
      : refcount_(1),
        text_(text),
    selection_start_(0),
    selection_end_(0),
        advise_sink_(nullptr),
        advise_sink_mask_(0),
        active_lock_(0),
        pending_lock_(0) {}
  ~TextStore() {
    if (advise_sink_) advise_sink_->Release();
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                           LPVOID* ppvObject) override;
  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;

  HRESULT STDMETHODCALLTYPE AdviseSink(REFIID riid, IUnknown* punk,
                                       DWORD dwMask) override;
  HRESULT STDMETHODCALLTYPE UnadviseSink(IUnknown* punk) override;
  HRESULT STDMETHODCALLTYPE RequestLock(DWORD dwLockFlags,
                                        HRESULT* phrSession) override;
  HRESULT STDMETHODCALLTYPE GetStatus(TS_STATUS* pdcs) override;
  HRESULT STDMETHODCALLTYPE QueryInsert(LONG acpTestStart, LONG acpTestEnd,
                                        ULONG cch, LONG* pacpResultStart,
                                        LONG* pacpResultEnd) override;
  HRESULT STDMETHODCALLTYPE GetSelection(ULONG ulIndex, ULONG ulCount,
                                         TS_SELECTION_ACP* pSelection,
                                         ULONG* pcFetched) override;
  HRESULT STDMETHODCALLTYPE
  SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection) override;
  HRESULT STDMETHODCALLTYPE GetText(LONG acpStart, LONG acpEnd, WCHAR* pchPlain,
                                    ULONG cchPlainReq, ULONG* pcchPlainRet,
                                    TS_RUNINFO* prgRunInfo, ULONG cRunInfoReq,
                                    ULONG* pcRunInfoRet,
                                    LONG* pacpNext) override;
  HRESULT STDMETHODCALLTYPE SetText(DWORD dwFlags, LONG acpStart, LONG acpEnd,
                                    const WCHAR* pchText, ULONG cch,
                                    TS_TEXTCHANGE* pChange) override;
  HRESULT STDMETHODCALLTYPE GetFormattedText(
      LONG acpStart, LONG acpEnd, IDataObject** ppDataObject) override;
  HRESULT STDMETHODCALLTYPE GetEmbedded(LONG acpPos, REFGUID rguidService,
                                        REFIID riid, IUnknown** ppunk) override;
  HRESULT STDMETHODCALLTYPE QueryInsertEmbedded(const GUID* pguidService,
                                                const FORMATETC* pFormatEtc,
                                                BOOL* pfInsertable) override;
  HRESULT STDMETHODCALLTYPE InsertEmbedded(DWORD dwFlags, LONG acpStart,
                                           LONG acpEnd,
                                           IDataObject* pDataObject,
                                           TS_TEXTCHANGE* pChange) override;
  HRESULT STDMETHODCALLTYPE InsertTextAtSelection(
      DWORD dwFlags, const WCHAR* pchText, ULONG cch, LONG* pacpStart,
      LONG* pacpEnd, TS_TEXTCHANGE* pChange) override;
  HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(
      DWORD dwFlags, IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd,
      TS_TEXTCHANGE* pChange) override;
  HRESULT STDMETHODCALLTYPE
  RequestSupportedAttrs(DWORD dwFlags, ULONG cFilterAttrs,
                        const TS_ATTRID* paFilterAttrs) override;
  HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(
      LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs,
      DWORD dwFlags) override;
  HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(
      LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs,
      DWORD dwFlags) override;
  HRESULT STDMETHODCALLTYPE FindNextAttrTransition(
      LONG acpStart, LONG acpHalt, ULONG cFilterAttrs,
      const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext,
      BOOL* pfFound, LONG* plFoundOffset) override;
  HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(ULONG ulCount,
                                                   TS_ATTRVAL* paAttrVals,
                                                   ULONG* pcFetched) override;
  HRESULT STDMETHODCALLTYPE GetEndACP(LONG* pacp) override;
  HRESULT STDMETHODCALLTYPE GetActiveView(TsViewCookie* pcvView) override;
  HRESULT STDMETHODCALLTYPE GetACPFromPoint(TsViewCookie vcView,
                                            const POINT* ptScreen,
                                            DWORD dwFlags, LONG* pacp) override;
  HRESULT STDMETHODCALLTYPE GetTextExt(TsViewCookie vcView, LONG acpStart,
                                       LONG acpEnd, RECT* prc,
                                       BOOL* pfClipped) override;
  HRESULT STDMETHODCALLTYPE GetScreenExt(TsViewCookie vcView,
                                         RECT* prc) override;
  HRESULT STDMETHODCALLTYPE GetWnd(TsViewCookie vcView, HWND* phwnd) override;

 private:
  std::wstring text_;
  int selection_start_;
  int selection_end_;

  ULONG refcount_;

  DWORD active_lock_;
  DWORD pending_lock_;
  ITextStoreACPSink* advise_sink_;
  DWORD advise_sink_mask_;
};

}  // namespace wiese

#endif