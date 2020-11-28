#include "text_store.h"

#include <Windows.h>
#include <msctf.h>
#include <olectl.h>

#include <algorithm>
#include <cassert>
#include <string>

namespace wiese {

HRESULT STDMETHODCALLTYPE TextStore::QueryInterface(REFIID riid,
                                                    LPVOID* ppvObject) {
  if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITextStoreACP)) {
    *ppvObject = static_cast<ITextStoreACP*>(this);
  } else {
    *ppvObject = nullptr;
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

ULONG STDMETHODCALLTYPE TextStore::AddRef() { return ++refcount_; }

ULONG STDMETHODCALLTYPE TextStore::Release() {
  if (--refcount_ == 0) delete this;
  return refcount_;
}

HRESULT STDMETHODCALLTYPE TextStore::AdviseSink(REFIID riid, IUnknown* punk,
                                                DWORD dwMask) {
  if (!IsEqualIID(riid, IID_ITextStoreACPSink)) return E_INVALIDARG;

  if (advise_sink_) {
    IUnknown* sink_id;
    advise_sink_->QueryInterface(&sink_id);
    sink_id->Release();
    if (sink_id == punk) {
      advise_sink_mask_ = dwMask;
      return S_OK;
    }
    return CONNECT_E_ADVISELIMIT;
  }

  ITextStoreACPSink* sink;
  HRESULT hr = punk->QueryInterface(&sink);
  if (FAILED(hr)) return E_UNEXPECTED;

  advise_sink_ = sink;
  advise_sink_mask_ = dwMask;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE TextStore::UnadviseSink(IUnknown* punk) {
  if (!advise_sink_) return CONNECT_E_NOCONNECTION;

  IUnknown* sink_id;
  advise_sink_->QueryInterface(&sink_id);
  sink_id->Release();
  if (sink_id == punk) {
    advise_sink_->Release();
    advise_sink_ = nullptr;
    advise_sink_mask_ = 0;
  }
  return S_OK;
}

HRESULT STDMETHODCALLTYPE TextStore::RequestLock(DWORD dwLockFlags,
                                                 HRESULT* phrSession) {
  if (active_lock_) {
    if (dwLockFlags & TS_LF_SYNC) {
      // Locked document can't grant a synchronous request another lock.
      *phrSession = TS_E_SYNCHRONOUS;
      return S_OK;
    }
    if (pending_lock_) {
      return E_FAIL;
    }
    pending_lock_ = dwLockFlags;
    *phrSession = TS_S_ASYNC;
    return S_OK;
  } else {
    if (dwLockFlags & TS_LF_SYNC) {
      // A synchrnous request has precedence over a pending asynchronous
      // request.
      active_lock_ = dwLockFlags & TS_LF_READWRITE;  // Removing TS_LF_SYNC.
      *phrSession = advise_sink_->OnLockGranted(active_lock_);
      active_lock_ = 0;
      dwLockFlags = 0;  // Mark as processed.
    }
    if (pending_lock_) {
      advise_sink_->OnLockGranted(pending_lock_);
      pending_lock_ = 0;
    }
    if (dwLockFlags) {
      active_lock_ = dwLockFlags;
      *phrSession = advise_sink_->OnLockGranted(active_lock_);
      active_lock_ = 0;
    }
    return S_OK;
  }
}

HRESULT STDMETHODCALLTYPE TextStore::GetStatus(TS_STATUS* pdcs) {
  if (!pdcs) return E_INVALIDARG;
  pdcs->dwDynamicFlags = 0;
  pdcs->dwStaticFlags = TS_SS_NOHIDDENTEXT;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE TextStore::QueryInsert(LONG acpTestStart,
                                                 LONG acpTestEnd, ULONG cch,
                                                 LONG* pacpResultStart,
                                                 LONG* pacpResultEnd) {
  if (acpTestStart < 0 || text_.size() < acpTestStart || acpTestEnd < 0 ||
      text_.size() < acpTestEnd)
    return E_INVALIDARG;
  *pacpResultStart = acpTestStart;
  *pacpResultEnd = acpTestStart;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE TextStore::GetSelection(ULONG ulIndex, ULONG ulCount,
                                                  TS_SELECTION_ACP* pSelection,
                                                  ULONG* pcFetched) {
  if (!(active_lock_ & TS_LF_READ)) return TF_E_NOLOCK;
  if (selection_start_ == selection_end_) return TF_E_NOSELECTION;
  if (ulCount == 0) return S_OK;
  pSelection->acpStart = selection_start_;
  pSelection->acpEnd = selection_end_;
  pSelection->style.ase = TS_AE_NONE;
  pSelection->style.fInterimChar = FALSE;
  *pcFetched = 1;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
TextStore::SetSelection(ULONG ulCount, const TS_SELECTION_ACP* pSelection) {
  if (!(active_lock_ & TS_LF_READWRITE)) return TF_E_NOLOCK;
  assert(ulCount < 2);
  selection_start_ = pSelection[0].acpStart;
  selection_end_ = pSelection[0].acpEnd;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE TextStore::GetText(
    LONG acpStart, LONG acpEnd, WCHAR* pchPlain, ULONG cchPlainReq,
    ULONG* pcchPlainRet, TS_RUNINFO* prgRunInfo, ULONG cRunInfoReq,
    ULONG* pcRunInfoRet, LONG* pacpNext) {
  if (!(active_lock_ & TS_LF_READ)) return TF_E_NOLOCK;
  if (acpStart < 0 || text_.size() < acpStart || acpEnd < -1 ||
      text_.size() < acpEnd)
    return TF_E_INVALIDPOS;

  auto begin = text_.begin() + acpStart;
  auto end = acpEnd == -1 ? text_.end() : text_.begin() + acpEnd + 1;
  ULONG num_chars_in_range = (end - begin) / sizeof(wchar_t);

  if (cchPlainReq) {
    ULONG num_chars_to_copy = std::min(num_chars_in_range, cchPlainReq);
    std::copy_n(begin, num_chars_to_copy, pchPlain);
    *pcchPlainRet = num_chars_to_copy;
    *pacpNext = acpStart + num_chars_to_copy;
  }

  if (cRunInfoReq) {
    prgRunInfo[0].uCount = num_chars_in_range;
    prgRunInfo[0].type = TS_RT_PLAIN;
    *pcRunInfoRet = 1;
  }
  return S_OK;
}

HRESULT STDMETHODCALLTYPE TextStore::SetText(DWORD dwFlags, LONG acpStart,
                                             LONG acpEnd, const WCHAR* pchText,
                                             ULONG cch,
                                             TS_TEXTCHANGE* pChange) {
  if (!(active_lock_ & TS_LF_READWRITE)) return TF_E_NOLOCK;
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::GetFormattedText(
    LONG acpStart, LONG acpEnd, IDataObject** ppDataObject) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::GetEmbedded(LONG acpPos,
                                                 REFGUID rguidService,
                                                 REFIID riid,
                                                 IUnknown** ppunk) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::QueryInsertEmbedded(
    const GUID* pguidService, const FORMATETC* pFormatEtc, BOOL* pfInsertable) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::InsertEmbedded(DWORD dwFlags,
                                                    LONG acpStart, LONG acpEnd,
                                                    IDataObject* pDataObject,
                                                    TS_TEXTCHANGE* pChange) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::InsertTextAtSelection(
    DWORD dwFlags, const WCHAR* pchText, ULONG cch, LONG* pacpStart,
    LONG* pacpEnd, TS_TEXTCHANGE* pChange) {
  if (!(active_lock_ & TS_LF_READWRITE)) return TF_E_NOLOCK;

  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::InsertEmbeddedAtSelection(
    DWORD dwFlags, IDataObject* pDataObject, LONG* pacpStart, LONG* pacpEnd,
    TS_TEXTCHANGE* pChange) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::RequestSupportedAttrs(
    DWORD dwFlags, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::RequestAttrsAtPosition(
    LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs,
    DWORD dwFlags) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::RequestAttrsTransitioningAtPosition(
    LONG acpPos, ULONG cFilterAttrs, const TS_ATTRID* paFilterAttrs,
    DWORD dwFlags) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::FindNextAttrTransition(
    LONG acpStart, LONG acpHalt, ULONG cFilterAttrs,
    const TS_ATTRID* paFilterAttrs, DWORD dwFlags, LONG* pacpNext,
    BOOL* pfFound, LONG* plFoundOffset) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::RetrieveRequestedAttrs(
    ULONG ulCount, TS_ATTRVAL* paAttrVals, ULONG* pcFetched) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::GetEndACP(LONG* pacp) { return E_NOTIMPL; }

HRESULT STDMETHODCALLTYPE TextStore::GetActiveView(TsViewCookie* pcvView) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::GetACPFromPoint(TsViewCookie vcView,
                                                     const POINT* ptScreen,
                                                     DWORD dwFlags,
                                                     LONG* pacp) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::GetTextExt(TsViewCookie vcView,
                                                LONG acpStart, LONG acpEnd,
                                                RECT* prc, BOOL* pfClipped) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::GetScreenExt(TsViewCookie vcView,
                                                  RECT* prc) {
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE TextStore::GetWnd(TsViewCookie vcView, HWND* phwnd) {
  return E_NOTIMPL;
}

}  // namespace wiese
