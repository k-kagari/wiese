#include "document.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>
#include <vector>

namespace {

template <typename T>
void Dump(T t) {
  std::cout << t << std::endl;
}

template <typename T, typename... Args>
void Dump(T t, Args... args)
{
  std::cout << t << ",";
  Dump(args...);
}

}  // namespace

#if 0
#define TRACE(...) Dump(__FUNCSIG__, __VA_ARGS__)
#else
#define TRACE(...)
#endif

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
  if (length == 0) return;
  original_.assign(original_text, original_text + length);
  pieces_.push_front(Piece::MakeOriginal(0, length));
}

Piece Document::AddCharsToBuffer(const wchar_t* chars, int count) {
  int start = added_.size();
  std::copy(chars, chars + count, std::back_inserter(added_));
  return Piece::MakePlain(start, start + count);
}

wchar_t Document::GetCharInPiece(const Piece& piece, int index) const {
  assert(index < piece.GetCharCount());
  if (piece.IsOriginal()) {
    return original_[piece.start() + index];
  } else if (piece.IsPlain()) {
    return added_[piece.start() + index];
  } else if (piece.IsLineBreak()) {
    return L'\n';
  }
  assert(false);
  return 0;
}

std::wstring_view Document::GetCharsInPiece(const Piece& piece) const {
  if (piece.IsOriginal()) {
    return {original_.data() + piece.start(),
            static_cast<std::size_t>(piece.GetCharCount())};
  } else if (piece.IsPlain()) {
    return {added_.data() + piece.start(),
            static_cast<std::size_t>(piece.GetCharCount())};
  } else if (piece.IsLineBreak()) {
    static const wchar_t kLF = L'\n';
    return {&kLF, 1};
  }
}

void Document::InsertCharsBefore(const wchar_t* chars, int count,
                                 int position) {
  assert(0 <= position);
  assert(position <= GetCharCount());
  if (position == 0) {
    pieces_.push_front(AddCharsToBuffer(chars, count));
    return;
  }
  int offset = 0;
  for (auto it = pieces_.begin(); it != pieces_.end(); ++it) {
    int piece_size = it->GetCharCount();
    if (offset + piece_size == position) {
      // Specified position is in between of two pieces.
      // Now consider whether we can just elongate the previous piece.
      if (it->IsPlain() && it->end() == static_cast<int>(added_.size())) {
        std::copy(chars, chars + count, std::back_inserter(added_));
        it->set_end(it->end() + count);
        return;
      }
      // Insert a new piece at current position.
      pieces_.insert(++it, AddCharsToBuffer(chars, count));
      return;
    }
    if (position < offset + piece_size) {
      // Specified position is in the middle of a piece.
      // Split it and insert a new piece.
      Piece latter_half = it->SplitAt(position - offset);
      pieces_.insert(++it, AddCharsToBuffer(chars, count));
      pieces_.insert(it, latter_half);
      return;
    }
    offset += piece_size;
  }
  assert(false);  // Unreachable.
}

void Document::InsertCharBefore(wchar_t ch, int position) {
  TRACE(ch, position);
  InsertCharsBefore(&ch, 1, position);
}

void Document::InsertStringBefore(const wchar_t* string, int position) {
  TRACE(string, position);
  InsertCharsBefore(string, std::wcslen(string), position);
}

wchar_t Document::EraseCharAt(int position) {
  TRACE(position);
  assert(0 <= position);
  assert(position < GetCharCount());
  if (position == 0) {
    Piece& piece = pieces_.front();
    if (piece.GetCharCount() == 1) {
      wchar_t ch = GetCharInPiece(piece, 0);
      pieces_.pop_front();
      return ch;
    }
    wchar_t ch = GetCharInPiece(piece, 0);
    piece.set_start(piece.start() + 1);
    return ch;
  }

  int offset = 0;
  for (auto it = pieces_.begin(); it != pieces_.end(); ++it) {
    int piece_size = it->GetCharCount();
    if (position == offset + piece_size) {
      ++it;
      if (it->GetCharCount() == 1) {
        wchar_t ch = GetCharInPiece(*it, 0);
        pieces_.erase(it);
        return ch;
      }
      wchar_t ch = GetCharInPiece(*it, 0);
      it->set_start(it->start() + 1);
      return ch;
    }
    if (position == offset + piece_size - 1) {
      assert(it->GetCharCount() > 1);
      wchar_t ch = GetCharInPiece(*it, it->GetCharCount() - 1);
      it->set_end(it->end() - 1);
      return ch;
    }
    if (position < offset + piece_size - 1) {
      Piece latter_half = it->SplitAt(position - offset);
      assert(latter_half.GetCharCount() > 1);
      wchar_t ch = GetCharInPiece(latter_half, 0);
      latter_half.set_start(latter_half.start() + 1);
      pieces_.insert(++it, latter_half);
      return ch;
    }
    offset += piece_size;
  }
  assert(false);  // Unreachable.
  return 0;
}

std::wstring Document::GetText() const {
  std::wstring text;
  for (const auto& piece : pieces_) {
    text += GetCharsInPiece(piece);
  }
  return text;
}

int Document::GetCharCount() const {
  int count = 0;
  for (const auto& piece : pieces_) {
    count += piece.GetCharCount();
  }
  return static_cast<int>(count);
}

wchar_t Document::GetCharAt(int position) const {
  assert(position >= 0);
  assert(position < GetCharCount());
  int offset = 0;
  for (const auto& piece : pieces_) {
    if (position < offset + piece.GetCharCount()) {
      return GetCharInPiece(piece, position - offset);
    }
    offset += piece.GetCharCount();
  }
  assert(false);  // Unreachable.
  return L'\0';
}

}  // namespace wiese