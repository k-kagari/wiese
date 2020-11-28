#ifndef WIESE_DOCUMENT_H_
#define WIESE_DOCUMENT_H_

#include <cassert>
#include <list>
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
  int GetCharCount() const {
    switch (kind_) {
      case Kind::kOriginal:
      case Kind::kPlain:
        return end_ - start_;
      case Kind::kLineBreak:
        return 1;
    }
    assert(false);
    return 0;
  }
  Piece SplitAt(int index) {
    Piece latter_half(*this);
    latter_half.end_ = end_;
    latter_half.start_ = end_ = index;
    return latter_half;
  }
  int start() const {
    assert(IsOriginal() || IsPlain());
    return start_;
  }
  void set_start(int value) {
    assert(IsOriginal() || IsPlain());
    assert(value <= end_);
    start_ = value;
  }
  int end() const {
    assert(IsOriginal() || IsPlain());
    return end_;
  }
  void set_end(int value) {
    assert(IsOriginal() || IsPlain());
    assert(start_ <= value);
    end_ = value;
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

  void InsertCharBefore(wchar_t ch, int position);
  void InsertStringBefore(const wchar_t* string, int position);
  wchar_t EraseCharAt(int position);

  std::wstring GetText() const;
  int GetCharCount() const;
  wchar_t GetCharAt(int position) const;

 private:
  Piece AddCharsToBuffer(const wchar_t* chars, int count);
  void InsertCharsBefore(const wchar_t* chars, int count,
                              int position);
  wchar_t GetCharInPiece(const Piece& piece, int index) const;
  std::wstring_view GetCharsInPiece(const Piece& piece) const;

  std::list<Piece> pieces_;
  std::vector<wchar_t> original_;
  std::vector<wchar_t> added_;
};

}  // namespace wiese

#endif
