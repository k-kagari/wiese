#include "document.h"

#include <cassert>
#include <cstring>
#include <string_view>
#include <vector>

namespace wiese {

Span Span::MakePlain(int start, int end) {
  assert(start <= end);
  Span span(Kind::kPlain);
  span.start_ = start;
  span.end_ = end;
  return span;
}

Span Span::MakeLineBreak() { return Span(Kind::kLineBreak); }

Document::Document(const wchar_t* original_text) {
  buffer_.assign(original_text, original_text + std::wcslen(original_text));
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