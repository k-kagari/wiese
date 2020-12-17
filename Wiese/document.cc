#include "document.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace {

template <typename T>
void Dump(T t) {
  std::cout << t << std::endl;
}

template <typename T, typename... Args>
void Dump(T t, Args... args) {
  std::cout << t << ",";
  Dump(args...);
}

}  // namespace

#if 0
#define TRACE(...) Dump(__FUNCSIG__, __VA_ARGS__)
#else
#define TRACE(...)
#endif
#define UNREACHABLE assert(false)

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

Document::Document(const wchar_t* original_text)
    : original_(original_text, original_text + std::wcslen(original_text)) {
  int start = 0;
  for (int i = 0; i < static_cast<int>(original_.size()); ++i) {
    if (original_[i] == L'\n') {
      pieces_.push_back(Piece::MakeOriginal(start, i));
      pieces_.push_back(Piece::MakeLineBreak());
      ++i;
      start = i;
    }
  }
  if (original_.size() - start > 0) {
    pieces_.push_back(Piece::MakeOriginal(start, original_.size()));
  }
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
  UNREACHABLE;
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
  UNREACHABLE;
  return {};
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
      Piece rest = it->SplitAt(position - offset);
      pieces_.insert(++it, AddCharsToBuffer(chars, count));
      pieces_.insert(it, rest);
      return;
    }
    offset += piece_size;
  }
  UNREACHABLE;
}

void Document::InsertCharsBefore(const wchar_t* chars, int count,
                                 int line, int column) {
  assert(line >= 0);
  assert(column >= 0);
  assert(line < GetLineCount());
  if (line == 0 && column == 0) {
    pieces_.push_front(AddCharsToBuffer(chars, count));
    return;
  }

  auto it = pieces_.begin();
  auto end = pieces_.end();
  AdvanceByLine(it, line, end);
  if (column == 0) {
    pieces_.insert(it, AddCharsToBuffer(chars, count));
  }

  int offset = 0;
  for (; it != end; ++it) {
    int piece_size = it->GetCharCount();
    if (offset + piece_size == column) {
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
    if (column < offset + piece_size) {
      // Specified position is in the middle of a piece.
      // Split it and insert a new piece.
      Piece rest = it->SplitAt(column - offset);
      pieces_.insert(++it, AddCharsToBuffer(chars, count));
      pieces_.insert(it, rest);
      return;
    }
    offset += piece_size;
  }
  UNREACHABLE;
}

void Document::InsertCharBefore(wchar_t ch, int position) {
  TRACE(ch, position);
  InsertCharsBefore(&ch, 1, position);
}

void Document::InsertCharBefore(wchar_t ch, int line, int column) {
  TRACE(ch, position);
  InsertCharsBefore(&ch, 1, line, column);
}

void Document::InsertStringBefore(const wchar_t* string, int position) {
  TRACE(string, position);
  InsertCharsBefore(string, std::wcslen(string), position);
}

void Document::InsertLineBreakBefore(int position) {
  TRACE(position);
  assert(position >= 0);
  assert(position <= GetCharCount());
  if (position == 0) {
    pieces_.push_front(Piece::MakeLineBreak());
    return;
  }
  int offset = 0;
  for (auto it = pieces_.begin(); it != pieces_.end(); ++it) {
    int piece_size = it->GetCharCount();
    if (offset + piece_size == position) {
      pieces_.insert(++it, Piece::MakeLineBreak());
      return;
    }
    if (position < offset + piece_size) {
      Piece rest = it->SplitAt(position - offset);
      pieces_.insert(++it, Piece::MakeLineBreak());
      pieces_.insert(it, rest);
      return;
    }
    offset += piece_size;
  }
  UNREACHABLE;
}

void Document::InsertLineBreakBefore(int line, int column) {
  TRACE(line, column);
  assert(line >= 0);
  assert(column >= 0);
  assert(line < GetLineCount());
  if (line == 0 && column == 0) {
    pieces_.push_front(Piece::MakeLineBreak());
    return;
  }

  auto it = pieces_.begin();
  auto end = pieces_.end();
  AdvanceByLine(it, line, end);

  int offset = 0;
  for (; it != end; ++it) {
    int piece_size = it->GetCharCount();
    if (offset + piece_size == column) {
      pieces_.insert(++it, Piece::MakeLineBreak());
      return;
    }
    if (column < offset + piece_size) {
      Piece rest = it->SplitAt(column - offset);
      pieces_.insert(++it, Piece::MakeLineBreak());
      pieces_.insert(it, rest);
      return;
    }
    offset += piece_size;
  }
  UNREACHABLE;
}

wchar_t Document::EraseCharInFrontOf(PieceList::iterator it) {
  if (it->GetCharCount() == 1) {
    wchar_t ch = GetCharInPiece(*it, 0);
    pieces_.erase(it);
    return ch;
  }
  wchar_t ch = GetCharInPiece(*it, 0);
  it->set_start(it->start() + 1);
  return ch;
}

wchar_t Document::EraseCharAt(int position) {
  TRACE(position);
  assert(0 <= position);
  assert(position < GetCharCount());
  if (position == 0) {
    return EraseCharInFrontOf(pieces_.begin());
  }

  int pos = 0;
  for (auto it = pieces_.begin(); it != pieces_.end(); ++it) {
    int piece_size = it->GetCharCount();
    if (position == pos + piece_size) {
      return EraseCharInFrontOf(++it);
    }
    if (position == pos + piece_size - 1) {
      assert(it->GetCharCount() > 1);
      wchar_t ch = GetCharInPiece(*it, it->GetCharCount() - 1);
      it->set_end(it->end() - 1);
      return ch;
    }
    if (position < pos + piece_size - 1) {
      Piece rest = it->SplitAt(position - pos);
      assert(rest.GetCharCount() > 1);
      wchar_t ch = GetCharInPiece(rest, 0);
      rest.set_start(rest.start() + 1);
      pieces_.insert(++it, rest);
      return ch;
    }
    pos += piece_size;
  }
  UNREACHABLE;
  return 0;
}

wchar_t Document::EraseCharAt(int line, int column) {
  TRACE(line, column);
  assert(0 <= line);
  assert(0 <= column);
  if (line == 0 && column == 0) {
    return EraseCharInFrontOf(pieces_.begin());
  }

  auto it = pieces_.begin();
  AdvanceByLine(it, line, pieces_.end());
  if (column == 0) {
    return EraseCharInFrontOf(it);
  }
  int offset = 0;
  for (; it != pieces_.end(); ++it) {
    offset += it->GetCharCount();
    if (offset == column) {
      return EraseCharInFrontOf(++it);
    }
    if (offset - 1 == column) {
      assert(it->GetCharCount() > 1);
      wchar_t ch = GetCharInPiece(*it, it->GetCharCount() - 1);
      it->set_end(it->end() - 1);
      return ch;
    }
    if (offset - 1 > column) {
      Piece rest = it->SplitAt(column);
      assert(rest.GetCharCount() > 1);
      wchar_t ch = GetCharInPiece(rest, 0);
      rest.set_start(rest.start() + 1);
      pieces_.insert(++it, rest);
      return ch;
    }
  }
  UNREACHABLE;
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

int Document::GetLineCount() const {
  int count = 1;
  for (const auto& piece : pieces_) {
    if (piece.IsLineBreak()) ++count;
  }
  return count;
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
  UNREACHABLE;
  return L'\0';
}

void AdvanceByLine(Document::PieceList::const_iterator& it, int count,
                   Document::PieceList::const_iterator end) {
  for (int i = 0; i < count && it != end; ++it) {
    if (it->IsLineBreak()) ++i;
  }
}

int GetCharCountOfLine(Document::PieceList::const_iterator it,
                        Document::PieceList::const_iterator end) {
  int count = 0;
  for (; it != end && !it->IsLineBreak(); ++it) {
    count += it->GetCharCount();
  }
  return count;
}

}  // namespace wiese