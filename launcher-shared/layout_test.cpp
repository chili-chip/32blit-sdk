#include "layout.hpp"

#include <cstdio>
#include <string>

// Host test for the layout helpers the console OS draws with. Built and run by ctest on desktop builds.
static int failures = 0;

#define EXPECT_EQ(got, expected) do { \
  auto g_ = (got); auto e_ = (expected); \
  if(!(g_ == e_)) { \
    std::fprintf(stderr, "%s:%i: %s is %s, expected %s\n", __FILE__, __LINE__, #got, std::to_string(g_).c_str(), std::to_string(e_).c_str()); \
    failures++; \
  } \
} while(0)

#define EXPECT_STR(got, expected) do { \
  std::string g_ = (got), e_ = (expected); \
  if(g_ != e_) { \
    std::fprintf(stderr, "%s:%i: %s is \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #got, g_.c_str(), e_.c_str()); \
    failures++; \
  } \
} while(0)

static void test_text_width() {
  EXPECT_EQ(text_width(""), 0);
  EXPECT_EQ(text_width("A"), 6);
  EXPECT_EQ(text_width("AB"), 12);
  EXPECT_EQ(text_width(".."), 4);
  EXPECT_EQ(text_width("1/7"), 3 + 4 + 6);
}

static void test_fit_text() {
  // fits: unchanged
  EXPECT_STR(fit_text("Snek", 100), "Snek");
  EXPECT_STR(fit_text("AB", 12), "AB");

  // too long: cut with ".." and fits
  auto cut = fit_text("Space Fighters TURBO", 60);
  EXPECT_EQ(cut.size() > 2 && cut.substr(cut.size() - 2) == "..", true);
  EXPECT_EQ(text_width(cut) <= 60, true);

  // no space left in front of the dots
  EXPECT_STR(fit_text("AB CDEFGH", text_width("AB ..")), "AB..");

  // nothing fits
  EXPECT_STR(fit_text("ABC", 2), "..");
}

static void test_scroll_offset() {
  // fits: never moves
  EXPECT_EQ(scroll_offset(50, 60, 0), 0);
  EXPECT_EQ(scroll_offset(50, 60, 5000), 0);

  // 30 px too wide: cut for 1 s, 1 s to scroll at 30 px/s, 1 s at the end, then again
  EXPECT_EQ(scroll_offset(90, 60, 0), -1);
  EXPECT_EQ(scroll_offset(90, 60, 999), -1);
  EXPECT_EQ(scroll_offset(90, 60, 1000), 0);
  EXPECT_EQ(scroll_offset(90, 60, 1500), 15);
  EXPECT_EQ(scroll_offset(90, 60, 2000), 30);
  EXPECT_EQ(scroll_offset(90, 60, 2999), 30);
  EXPECT_EQ(scroll_offset(90, 60, 3000), -1);
  EXPECT_EQ(scroll_offset(90, 60, 4500), 15);
}

static void test_scrolling_text() {
  ScrollingText s;
  // starts when first seen
  EXPECT_EQ(s.offset("Space Fighters TURBO", 90, 60, 10000), -1);
  EXPECT_EQ(s.offset("Space Fighters TURBO", 90, 60, 11500), 15);
  // a new text starts again from the cut
  EXPECT_EQ(s.offset("Chili Tetris Deluxe", 90, 60, 11600), -1);
  EXPECT_EQ(s.offset("Chili Tetris Deluxe", 90, 60, 12600), 0);
  // restart() goes back to the cut, for the same text shown again
  s.restart(13000);
  EXPECT_EQ(s.offset("Chili Tetris Deluxe", 90, 60, 13500), -1);
}

static void test_list_first_row() {
  // fits: from the top
  EXPECT_EQ(list_first_row(0, 4, 6), 0);
  EXPECT_EQ(list_first_row(3, 4, 6), 0);
  // the selected row stays on the last line
  EXPECT_EQ(list_first_row(5, 9, 6), 0);
  EXPECT_EQ(list_first_row(6, 9, 6), 1);
  EXPECT_EQ(list_first_row(8, 9, 6), 3);
  // nothing selected
  EXPECT_EQ(list_first_row(-1, 9, 6), 0);
}

static void test_scroll_thumb() {
  // fits: no bar
  EXPECT_EQ(scroll_thumb(0, 6, 6, 96).size, 0);
  // nine rows, six shown: two thirds of the track
  EXPECT_EQ(scroll_thumb(0, 9, 6, 96).size, 64);
  EXPECT_EQ(scroll_thumb(0, 9, 6, 96).pos, 0);
  EXPECT_EQ(scroll_thumb(3, 9, 6, 96).pos, 32);
  EXPECT_EQ(scroll_thumb(1, 9, 6, 96).pos, 11);
  // never thinner than 4 px
  EXPECT_EQ(scroll_thumb(0, 300, 6, 96).size, 4);
  EXPECT_EQ(scroll_thumb(294, 300, 6, 96).pos, 92);
}

static void test_position_bar() {
  // one tile: the whole width
  EXPECT_EQ(position_bar(0, 1, 128).size, 128);
  // seven tiles
  EXPECT_EQ(position_bar(0, 7, 128).size, 18);
  EXPECT_EQ(position_bar(0, 7, 128).pos, 0);
  EXPECT_EQ(position_bar(6, 7, 128).pos, 110);
  EXPECT_EQ(position_bar(3, 7, 128).pos, 55);
  // 31 tiles: never thinner than 8 px
  EXPECT_EQ(position_bar(0, 31, 128).size, 8);
  EXPECT_EQ(position_bar(30, 31, 128).pos, 120);
}

static void test_slide() {
  // 150 ms from a whole cover away to rest, in whole pixels
  EXPECT_EQ(slide_offset(128, 0), 128);
  EXPECT_EQ(slide_offset(128, 75), 64);
  EXPECT_EQ(slide_offset(128, 150), 0);
  EXPECT_EQ(slide_offset(-128, 75), -64);
  EXPECT_EQ(slide_offset(128, 1000), 0);
}

static void test_wrap_words() {
  auto lines = wrap_words("MOSS GARDEN", text_width("MOSS GARDEN"));
  EXPECT_EQ(lines.size(), size_t(1));

  lines = wrap_words("SPACE FIGHTERS TURBO", text_width("SPACE FIGHTERS"));
  EXPECT_EQ(lines.size(), size_t(2));
  EXPECT_STR(lines[0], "SPACE FIGHTERS");
  EXPECT_STR(lines[1], "TURBO");

  // at 2x, half the width
  lines = wrap_words("SPACE FIGHTERS TURBO", text_width("SPACE FIGHTERS") * 2, 2);
  EXPECT_EQ(lines.size(), size_t(2));

  // a word longer than the line gets a line of its own
  lines = wrap_words("A SUPERCALIFRAGILISTIC B", 20);
  EXPECT_EQ(lines.size(), size_t(3));
  EXPECT_STR(lines[1], "SUPERCALIFRAGILISTIC");

  EXPECT_EQ(wrap_words("", 20).size(), size_t(0));
}

int main() {
  test_text_width();
  test_fit_text();
  test_scroll_offset();
  test_scrolling_text();
  test_list_first_row();
  test_scroll_thumb();
  test_position_bar();
  test_slide();
  test_wrap_words();

  if(failures)
    std::fprintf(stderr, "%i failed\n", failures);
  else
    std::printf("layout: all passed\n");

  return failures ? 1 : 0;
}
