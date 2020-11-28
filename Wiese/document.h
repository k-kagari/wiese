#ifndef WIESE_DOCUMENT_H_
#define WIESE_DOCUMENT_H_

#include <cassert>
#include <string_view>
#include <vector>

namespace wiese {

class Span {
 private:
  enum class Kind { kPlain, kLineBreak };

 public:
  static Span MakePlain(int start, int end);
  static Span MakeLineBreak();
  bool IsPlain() const { return kind_ == Kind::kPlain; }
  bool IsLineBreak() const { return kind_ == Kind::kLineBreak; }
  int start() const {
    assert(IsPlain());
    return start_;
  }
  int end() const {
    assert(IsPlain());
    return end_;
  }

 private:
  Span(Kind kind) : kind_(kind) {}
  Kind kind_;
  int start_;
  int end_;
};

class Document {
 public:
  Document(const wchar_t* original_text);
  Document(const Document&) = delete;
  Document& operator=(const Document&) = delete;

  void InsertCharacterBefore(wchar_t character, int position);
  void InsertStringBefore(const wchar_t* string, int position);
  wchar_t EraseCharacterAt(int position);

  std::wstring_view GetText() const;
  int GetCharacterCount() const;
  wchar_t GetCharacterAt(int position) const;

 private:
  std::vector<wchar_t> buffer_;
};

}  // namespace wiese

#endif
