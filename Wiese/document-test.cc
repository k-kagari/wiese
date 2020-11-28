#include "document.h"

#include <gtest/gtest.h>

#include <cstring>

constexpr const wchar_t* kText = L"0123456789";

TEST(Document, Constructor) {
  wiese::Document doc(kText);
  EXPECT_EQ(kText, doc.GetText());
}

TEST(Document, GetCharacterCount) {
  wiese::Document doc(kText);
  EXPECT_EQ(static_cast<int>(std::wcslen(kText)), doc.GetCharacterCount());
  EXPECT_EQ(static_cast<int>(doc.GetText().size()), doc.GetCharacterCount());
}

TEST(Document, GetCharacterAt_Beginning) {
  wiese::Document doc(kText);
  EXPECT_EQ(L'0', doc.GetCharacterAt(0));
}

TEST(Document, GetCharacterAt_End) {
  wiese::Document doc(kText);
  EXPECT_EQ(L'9', doc.GetCharacterAt(9));
}

TEST(Document, InsertCharacterBefore_Beginning) {
  wiese::Document doc(kText);
  doc.InsertCharacterBefore(L's', 0);
  EXPECT_EQ(L's', doc.GetCharacterAt(0));
}

TEST(Document, InsertCharacterBefore_Middle) {
  wiese::Document doc(kText);
  doc.InsertCharacterBefore(L'm', 5);
  EXPECT_EQ(L'm', doc.GetCharacterAt(5));
}

TEST(Document, InsertCharacterBefore_End) {
  wiese::Document doc(kText);
  doc.InsertCharacterBefore(L'e', 10);
  EXPECT_EQ(L'e', doc.GetCharacterAt(10));
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

TEST(Document, EraseCharacterAt_Beginning) {
  wiese::Document doc(kText);
  doc.EraseCharacterAt(0);
  EXPECT_EQ(L"123456789", doc.GetText());
}

TEST(Document, EraseCharacterAt_Middle) {
  wiese::Document doc(kText);
  doc.EraseCharacterAt(5);
  EXPECT_EQ(L"012346789", doc.GetText());
}

TEST(Document, EraseCharacterAt_End) {
  wiese::Document doc(kText);
  doc.EraseCharacterAt(9);
  EXPECT_EQ(L"012345678", doc.GetText());
}

TEST(Span, MakePlain_returns_Span_that_is_a_Plain) {
  wiese::Span plain_span = wiese::Span::MakePlain(1, 2);
  EXPECT_TRUE(plain_span.IsPlain());
}

TEST(Span, MakeLineBreak_returns_Span_that_is_a_LineBreak) {
  wiese::Span line_break_span = wiese::Span::MakeLineBreak();
  EXPECT_TRUE(line_break_span.IsLineBreak());
}

TEST(Span, IsPlain_returns_false_when_it_is_not_a_Plain) {
  wiese::Span not_plain_span = wiese::Span::MakeLineBreak();
  EXPECT_FALSE(not_plain_span.IsPlain());
}

TEST(Span, IsLineBreak_returns_false_when_it_is_not_a_LineBreak) {
  wiese::Span not_line_break_span = wiese::Span::MakePlain(1, 2);
  EXPECT_FALSE(not_line_break_span.IsLineBreak());
}

TEST(Span, start_returns_value_passed_on_MakePlain) {
  {
    wiese::Span plain_span = wiese::Span::MakePlain(1, 2);
    EXPECT_EQ(1, plain_span.start());
  }
  {
    wiese::Span plain_span = wiese::Span::MakePlain(2, 3);
    EXPECT_EQ(2, plain_span.start());
  }
}

TEST(Span, end_returns_value_passed_on_MakePlain) {
  {
    wiese::Span plain_span = wiese::Span::MakePlain(1, 2);
    EXPECT_EQ(2, plain_span.end());
  }
  {
    wiese::Span plain_span = wiese::Span::MakePlain(2, 3);
    EXPECT_EQ(3, plain_span.end());
  }
}
