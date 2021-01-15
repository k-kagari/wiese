#include "document.h"

#include <gtest/gtest.h>

#include <cstring>
#include <iterator>
#include <ostream>

constexpr const wchar_t* kText = L"0123456789";
constexpr const wchar_t* kMultiLineText = L"01234\n6789a";

namespace wiese {

std::ostream& operator<<(std::ostream& os, const Piece& piece) {
  if (piece.IsOriginal()) {
    return os << "Original(" << piece.start() << "," << piece.end() << ")";
  } else if (piece.IsPlain()) {
    return os << "Plain(" << piece.start() << "," << piece.end() << ")";
  } else if (piece.IsLineBreak()) {
    return os << "LineBreak";
  }
  assert(false);
  return os;
}

}  // namespace wiese

TEST(Document, Constructor) {
  wiese::Document doc(kText);
  EXPECT_EQ(kText, doc.GetText());
}

TEST(Document, Constructor_ConvertLFIntoLineBreak) {
  wiese::Document doc(kMultiLineText);
  auto it = doc.PieceIteratorBegin();
  EXPECT_EQ(wiese::Piece::MakeOriginal(0, 5), *it);
  EXPECT_EQ(wiese::Piece::MakeLineBreak(), *++it);
  EXPECT_EQ(wiese::Piece::MakeOriginal(6, 11), *++it);
}

TEST(Document, GetCharCount) {
  wiese::Document doc(kText);
  EXPECT_EQ(static_cast<int>(std::wcslen(kText)), doc.GetCharCount());
  EXPECT_EQ(static_cast<int>(doc.GetText().size()), doc.GetCharCount());
}

TEST(Document, GetLineCount) {
  wiese::Document doc(kMultiLineText);
  EXPECT_EQ(2, doc.GetLineCount());
}

TEST(Document, GetCharAt_Beginning) {
  wiese::Document doc(kText);
  EXPECT_EQ(L'0', doc.GetCharAt(0));
}

TEST(Document, GetCharAt_End) {
  wiese::Document doc(kText);
  EXPECT_EQ(L'9', doc.GetCharAt(9));
}

TEST(Document, InsertCharBefore_Beginning) {
  wiese::Document doc(kText);
  doc.InsertCharBefore(L's', 0);
  EXPECT_EQ(L's', doc.GetCharAt(0));
}

TEST(Document, InsertCharBefore_Middle) {
  wiese::Document doc(kText);
  doc.InsertCharBefore(L'm', 5);
  EXPECT_EQ(L'm', doc.GetCharAt(5));
}

TEST(Document, InsertCharBefore_End) {
  wiese::Document doc(kText);
  doc.InsertCharBefore(L'e', 10);
  EXPECT_EQ(L'e', doc.GetCharAt(10));
}

TEST(Document, InsertCharBefore_ByLineAndColumn1) {
  wiese::Document doc(kMultiLineText);
  doc.InsertCharBefore(L's', 0, 0);
  EXPECT_EQ(L's', doc.GetCharAt(0));
}

TEST(Document, InsertCharBefore_ByLineAndColumn2) {
  wiese::Document doc(kMultiLineText);
  doc.InsertCharBefore(L'm', 0, 5);
  EXPECT_EQ(L'm', doc.GetCharAt(5));
}

TEST(Document, InsertCharBefore_ByLineAndColumn3) {
  wiese::Document doc(kMultiLineText);
  doc.InsertCharBefore(L'm', 1, 0);
  EXPECT_EQ(L'm', doc.GetCharAt(6));
}

TEST(Document, InsertCharBefore_ByLineAndColumn4) {
  wiese::Document doc(kMultiLineText);
  doc.InsertCharBefore(L'e', 1, 5);
  EXPECT_EQ(L'e', doc.GetCharAt(11));
}

TEST(Document, InsertStringBefore_Beginning) {
  wiese::Document doc(kText);
  doc.InsertStringBefore(L"beginning", 0);
  EXPECT_EQ(L"beginning0123456789", doc.GetText());
}

TEST(Document, InsertStringBefore_Middle) {
  wiese::Document doc(kText);
  doc.InsertStringBefore(L"middle", 5);
  EXPECT_EQ(L"01234middle56789", doc.GetText());
}

TEST(Document, InsertStringBefore_End) {
  wiese::Document doc(kText);
  doc.InsertStringBefore(L"end", 10);
  EXPECT_EQ(L"0123456789end", doc.GetText());
}

TEST(Document, InsertLineBreakBefore_Beginning) {
  wiese::Document doc(kText);
  doc.InsertLineBreakBefore(0);
  EXPECT_EQ(L"\n0123456789", doc.GetText());
}

TEST(Document, InsertLineBreakBefore_Middle) {
  wiese::Document doc(kText);
  doc.InsertLineBreakBefore(5);
  EXPECT_EQ(L"01234\n56789", doc.GetText());
}

TEST(Document, InsertLineBreakBefore_End) {
  wiese::Document doc(kText);
  doc.InsertLineBreakBefore(10);
  EXPECT_EQ(L"0123456789\n", doc.GetText());
}

TEST(Document, InsertLineBreakBefore_ByLineAndOffset1) {
  wiese::Document doc(kMultiLineText);
  doc.InsertLineBreakBefore(0, 0);
  EXPECT_EQ(L"\n01234\n6789a", doc.GetText());
}

TEST(Document, InsertLineBreakBefore_ByLineAndOffset2) {
  wiese::Document doc(kMultiLineText);
  doc.InsertLineBreakBefore(0, 5);
  EXPECT_EQ(L"01234\n\n6789a", doc.GetText());
}

TEST(Document, InsertLineBreakBefore_ByLineAndOffset3) {
  wiese::Document doc(kMultiLineText);
  doc.InsertLineBreakBefore(1, 0);
  EXPECT_EQ(L"01234\n\n6789a", doc.GetText());
}

TEST(Document, InsertLineBreakBefore_ByLineAndOffset4) {
  wiese::Document doc(kMultiLineText);
  doc.InsertLineBreakBefore(1, 5);
  EXPECT_EQ(L"01234\n6789a\n", doc.GetText());
}

TEST(Document, EraseCharAt_Beginning) {
  wiese::Document doc(kText);
  EXPECT_EQ(L'0', doc.EraseCharAt(0));
  EXPECT_EQ(L"123456789", doc.GetText());
}

TEST(Document, EraseCharAt_Middle) {
  wiese::Document doc(kText);
  EXPECT_EQ(L'5', doc.EraseCharAt(5));
  EXPECT_EQ(L"012346789", doc.GetText());
}

TEST(Document, EraseCharAt_End) {
  wiese::Document doc(kText);
  EXPECT_EQ(L'9', doc.EraseCharAt(9));
  EXPECT_EQ(L"012345678", doc.GetText());
}

TEST(Document, EraseCharAt_ByLineAndOffset_1) {
  wiese::Document doc(kMultiLineText);
  EXPECT_EQ(L'4', doc.EraseCharAt(0, 4));
  EXPECT_EQ(L"0123\n6789a", doc.GetText());
}

TEST(Document, EraseCharAt_ByLineAndOffset_2) {
  wiese::Document doc(kMultiLineText);
  EXPECT_EQ(L'\n', doc.EraseCharAt(0, 5));
  EXPECT_EQ(L"012346789a", doc.GetText());
}

TEST(Document, EraseCharAt_ByLineAndOffset_3) {
  wiese::Document doc(kMultiLineText);
  EXPECT_EQ(L'6', doc.EraseCharAt(1, 0));
  EXPECT_EQ(L"01234\n789a", doc.GetText());
}

TEST(Document, EraseCharsInRange_SingleLine) {
  wiese::Document doc(kMultiLineText);
  doc.EraseCharsInRange(1, 1, 1, 4);
  EXPECT_EQ(L"01234\n6a", doc.GetText());
}

TEST(Document, EraseCharsInRange_MultiLine) {
  wiese::Document doc(kMultiLineText);
  doc.EraseCharsInRange(0, 3, 1, 2);
  EXPECT_EQ(L"01289a", doc.GetText());
}

TEST(Document, RegressionCase1) {
  wiese::Document doc(kText);
  doc.InsertCharBefore(L'a', 0);
  doc.EraseCharAt(0);
  EXPECT_EQ(kText, doc.GetText());
}

TEST(Document, RegressionCase2) {
  wiese::Document doc(kText);
  doc.InsertCharBefore(L'a', 1);
  doc.InsertCharBefore(L'b', 2);
  EXPECT_EQ(L"0ab123456789", doc.GetText());
}

TEST(Document, RegressionCase3) {
  wiese::Document doc(kText);
  doc.InsertCharBefore(L'a', 1);
  doc.InsertCharBefore(L'b', 2);
  doc.EraseCharAt(2);
  EXPECT_EQ(L"0a123456789", doc.GetText());
}

TEST(Document, RegressionCase4) {
  wiese::Document doc(kText);
  doc.InsertLineBreakBefore(0, 5);
  doc.EraseCharAt(1, 2);
  doc.EraseCharAt(1, 1);
  doc.EraseCharAt(1, 0);
  EXPECT_EQ(L"01234\n89", doc.GetText());
}

TEST(Document, RegressionCase5) {
  wiese::Document doc(kText);
  doc.InsertLineBreakBefore(0, 5);
  doc.InsertCharBefore(L'a', 1, 0);
  EXPECT_EQ(L"01234\na56789", doc.GetText());
}

TEST(Piece, MakeOriginal_returns_Piece_that_is_a_Original) {
  EXPECT_TRUE(wiese::Piece::MakeOriginal(1, 2).IsOriginal());
}

TEST(Piece, MakePlain_returns_Piece_that_is_a_Plain) {
  EXPECT_TRUE(wiese::Piece::MakePlain(1, 2).IsPlain());
}

TEST(Piece, MakeLineBreak_returns_Piece_that_is_a_LineBreak) {
  EXPECT_TRUE(wiese::Piece::MakeLineBreak().IsLineBreak());
}

TEST(Piece, IsOriginal_returns_false_when_it_is_not_a_Original) {
  EXPECT_FALSE(wiese::Piece::MakePlain(1, 2).IsOriginal());
}

TEST(Piece, IsPlain_returns_false_when_it_is_not_a_Plain) {
  EXPECT_FALSE(wiese::Piece::MakeLineBreak().IsPlain());
}

TEST(Piece, IsLineBreak_returns_false_when_it_is_not_a_LineBreak) {
  EXPECT_FALSE(wiese::Piece::MakeOriginal(1, 2).IsLineBreak());
}

TEST(Piece, start_returns_value_passed_on_MakeOriginal) {
  EXPECT_EQ(1, wiese::Piece::MakeOriginal(1, 2).start());
  EXPECT_EQ(2, wiese::Piece::MakeOriginal(2, 3).start());
}

TEST(Piece, end_returns_value_passed_on_MakeOriginal) {
  EXPECT_EQ(2, wiese::Piece::MakeOriginal(1, 2).end());
  EXPECT_EQ(3, wiese::Piece::MakeOriginal(2, 3).end());
}

TEST(Piece, start_returns_value_passed_on_MakePlain) {
  EXPECT_EQ(1, wiese::Piece::MakePlain(1, 2).start());
  EXPECT_EQ(2, wiese::Piece::MakePlain(2, 3).start());
}

TEST(Piece, end_returns_value_passed_on_MakePlain) {
  EXPECT_EQ(2, wiese::Piece::MakePlain(1, 2).end());
  EXPECT_EQ(3, wiese::Piece::MakePlain(2, 3).end());
}

TEST(Piece, SplitAt_shortens_itself_and_returns_the_rest) {
  wiese::Piece piece = wiese::Piece::MakeOriginal(1, 6);
  wiese::Piece rest = piece.SplitAt(2);
  EXPECT_EQ(1, piece.start());
  EXPECT_EQ(3, piece.end());
  EXPECT_EQ(3, rest.start());
  EXPECT_EQ(6, rest.end());
}

TEST(Piece, Slice_returns_subset_of_itself) {
  wiese::Piece piece = wiese::Piece::MakePlain(1, 5);
  wiese::Piece sub_piece = piece.Slice(1, 3);
  EXPECT_EQ(2, sub_piece.start());
  EXPECT_EQ(4, sub_piece.end());
}

TEST(AdvanceByLine, Test1) {
  wiese::Document doc(kMultiLineText);
  auto it1 = doc.PieceIteratorBegin();
  auto it2 = doc.PieceIteratorBegin();
  EXPECT_EQ(*it1, *it2);
  std::advance(it1, 2);
  AdvanceByLine(it2, 1, doc.PieceIteratorEnd());
  EXPECT_EQ(*it1, *it2);
}

TEST(GetCharCountOfLine, Test1) {
  wiese::Document doc(kText);
  EXPECT_EQ(10, GetCharCountOfLine(doc.PieceIteratorBegin(), doc.PieceIteratorEnd()));
}
