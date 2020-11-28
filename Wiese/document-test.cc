#include "document.h"

#include <gtest/gtest.h>

#include <cstring>

constexpr const wchar_t* kText = L"0123456789";

TEST(Document, Constructor) {
  wiese::Document doc(kText);
  EXPECT_EQ(kText, doc.GetText());
}

TEST(Document, GetCharCount) {
  wiese::Document doc(kText);
  EXPECT_EQ(static_cast<int>(std::wcslen(kText)), doc.GetCharCount());
  EXPECT_EQ(static_cast<int>(doc.GetText().size()), doc.GetCharCount());
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
