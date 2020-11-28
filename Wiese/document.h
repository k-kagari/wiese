#ifndef WIESE_DOCUMENT_H_
#define WIESE_DOCUMENT_H_

#include <cassert>
#include <string_view>
#include <vector>

namespace wiese {

class Piece {
 private:
  enum class Kind { kOriginal, kPlain, kLineBreak };

 public:
  static Piece MakeOriginal(int start, int end);
  static Piece MakePlain(int start, int end);
  static Piece MakeLineBreak();
  bool IsOriginal() const { return kind_ == Kind::kOriginal; }
  bool IsPlain() const { return kind_ == Kind::kPlain; }
  bool IsLineBreak() const { return kind_ == Kind::kLineBreak; }
  int start() const {
    assert(IsOriginal() || IsPlain());
    return start_;
  }
  int end() const {
    assert(IsOriginal() || IsPlain());
    return end_;
  }

 private:
  Piece(Kind kind) : kind_(kind) {}
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
  std::vector<Piece> pieces_;
  std::vector<wchar_t> original_;
  std::vector<wchar_t> added_;
  std::vector<wchar_t> buffer_;
};

}  // namespace wiese

#endif
