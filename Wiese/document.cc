#include "document.h"

#include <cassert>
#include <cstring>
#include <string_view>
#include <vector>

namespace wiese {

Piece Piece::MakeOriginal(int start, int end) {
  assert(start <= end);
  Piece piece(Kind::kOriginal);
  piece.start_ = start;
  piece.end_ = end;
  return piece;
}

Piece Piece::MakePlain(int start, int end) {
  assert(start <= end);
  Piece piece(Kind::kPlain);
  piece.start_ = start;
  piece.end_ = end;
  return piece;
}

Piece Piece::MakeLineBreak() { return Piece(Kind::kLineBreak); }

Document::Document(const wchar_t* original_text) {
  std::size_t length = std::wcslen(original_text);
  buffer_.assign(original_text, original_text + length);
  /*
  if (length == 0) return;
  original_.assign(original_text, original_text + length);
  */
}

void Document::InsertCharacterBefore(wchar_t character, int position) {
  assert(0 <= position);
  assert(position <= (int)buffer_.size());
  buffer_.insert(buffer_.begin() + position, character);
}

void Document::InsertStringBefore(const wchar_t* string, int position) {
  assert(0 <= position);
  assert(position <= (int)buffer_.size());
  buffer_.insert(buffer_.begin() + position, string,
                 string + std::wcslen(string));
}

wchar_t Document::EraseCharacterAt(int position) {
  assert(0 <= position);
  assert(position < (int)buffer_.size());
  wchar_t ch = buffer_[position];
  buffer_.erase(buffer_.begin() + position);
  return ch;
}

std::wstring_view Document::GetText() const {
  return {buffer_.data(), buffer_.size()};
}

int Document::GetCharacterCount() const {
  return static_cast<int>(buffer_.size());
}

wchar_t Document::GetCharacterAt(int position) const {
  assert(0 <= position);
  assert(position < (int)buffer_.size());
  return buffer_[position];
}

}  // namespace wiese