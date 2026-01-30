#include "ui_texture_browser.h"

#include <gtest/gtest.h>

// =============================================================================
// FormatFileSize Tests
// =============================================================================

TEST(TextureBrowser, FormatFileSize_Bytes) {
  EXPECT_EQ(FormatFileSize(0), "0 B");
  EXPECT_EQ(FormatFileSize(1), "1 B");
  EXPECT_EQ(FormatFileSize(512), "512 B");
  EXPECT_EQ(FormatFileSize(1023), "1023 B");
}

TEST(TextureBrowser, FormatFileSize_Kilobytes) {
  EXPECT_EQ(FormatFileSize(1024), "1.0 KB");
  EXPECT_EQ(FormatFileSize(1536), "1.5 KB");
  EXPECT_EQ(FormatFileSize(2048), "2.0 KB");
  EXPECT_EQ(FormatFileSize(10240), "10.0 KB");
  EXPECT_EQ(FormatFileSize(102400), "100.0 KB");
}

TEST(TextureBrowser, FormatFileSize_Megabytes) {
  EXPECT_EQ(FormatFileSize(1024 * 1024), "1.0 MB");
  EXPECT_EQ(FormatFileSize(1024 * 1024 * 2), "2.0 MB");
  EXPECT_EQ(FormatFileSize(1024 * 1024 + 512 * 1024), "1.5 MB");
  EXPECT_EQ(FormatFileSize(1024 * 1024 * 10), "10.0 MB");
}

// =============================================================================
// FormatBPP Tests
// =============================================================================

TEST(TextureBrowser, FormatBPP_DXT1) {
  EXPECT_EQ(FormatBPP(4), "DXT1");
}

TEST(TextureBrowser, FormatBPP_DXT3_5) {
  EXPECT_EQ(FormatBPP(8), "DXT3/5");
}

TEST(TextureBrowser, FormatBPP_RGB16) {
  EXPECT_EQ(FormatBPP(16), "RGB16");
}

TEST(TextureBrowser, FormatBPP_RGB24) {
  EXPECT_EQ(FormatBPP(24), "RGB24");
}

TEST(TextureBrowser, FormatBPP_RGBA32) {
  EXPECT_EQ(FormatBPP(32), "RGBA32");
}

TEST(TextureBrowser, FormatBPP_Other) {
  EXPECT_EQ(FormatBPP(12), "12bpp");
  EXPECT_EQ(FormatBPP(48), "48bpp");
}

// =============================================================================
// SortTextureEntries Tests
// =============================================================================

TEST(TextureBrowser, SortEntries_ByName_Ascending) {
  std::vector<TextureBrowserEntry> entries;
  entries.push_back({"zebra.dtx", "", "", 0, 0, 0, "", "", false});
  entries.push_back({"alpha.dtx", "", "", 0, 0, 0, "", "", false});
  entries.push_back({"middle.dtx", "", "", 0, 0, 0, "", "", false});

  SortTextureEntries(entries, TextureBrowserSortMode::Name, true);

  EXPECT_EQ(entries[0].name, "alpha.dtx");
  EXPECT_EQ(entries[1].name, "middle.dtx");
  EXPECT_EQ(entries[2].name, "zebra.dtx");
}

TEST(TextureBrowser, SortEntries_ByName_Descending) {
  std::vector<TextureBrowserEntry> entries;
  entries.push_back({"alpha.dtx", "", "", 0, 0, 0, "", "", false});
  entries.push_back({"zebra.dtx", "", "", 0, 0, 0, "", "", false});
  entries.push_back({"middle.dtx", "", "", 0, 0, 0, "", "", false});

  SortTextureEntries(entries, TextureBrowserSortMode::Name, false);

  EXPECT_EQ(entries[0].name, "zebra.dtx");
  EXPECT_EQ(entries[1].name, "middle.dtx");
  EXPECT_EQ(entries[2].name, "alpha.dtx");
}

TEST(TextureBrowser, SortEntries_BySize_Ascending) {
  std::vector<TextureBrowserEntry> entries;
  entries.push_back({"large.dtx", "", "", 0, 0, 10000, "", "", false});
  entries.push_back({"small.dtx", "", "", 0, 0, 100, "", "", false});
  entries.push_back({"medium.dtx", "", "", 0, 0, 1000, "", "", false});

  SortTextureEntries(entries, TextureBrowserSortMode::Size, true);

  EXPECT_EQ(entries[0].name, "small.dtx");
  EXPECT_EQ(entries[1].name, "medium.dtx");
  EXPECT_EQ(entries[2].name, "large.dtx");
}

TEST(TextureBrowser, SortEntries_BySize_Descending) {
  std::vector<TextureBrowserEntry> entries;
  entries.push_back({"small.dtx", "", "", 0, 0, 100, "", "", false});
  entries.push_back({"large.dtx", "", "", 0, 0, 10000, "", "", false});
  entries.push_back({"medium.dtx", "", "", 0, 0, 1000, "", "", false});

  SortTextureEntries(entries, TextureBrowserSortMode::Size, false);

  EXPECT_EQ(entries[0].name, "large.dtx");
  EXPECT_EQ(entries[1].name, "medium.dtx");
  EXPECT_EQ(entries[2].name, "small.dtx");
}

TEST(TextureBrowser, SortEntries_ByDimensions_Ascending) {
  std::vector<TextureBrowserEntry> entries;
  entries.push_back({"large.dtx", "", "", 512, 512, 0, "", "", false});  // 262144
  entries.push_back({"small.dtx", "", "", 64, 64, 0, "", "", false});     // 4096
  entries.push_back({"medium.dtx", "", "", 256, 256, 0, "", "", false});  // 65536

  SortTextureEntries(entries, TextureBrowserSortMode::Dimensions, true);

  EXPECT_EQ(entries[0].name, "small.dtx");
  EXPECT_EQ(entries[1].name, "medium.dtx");
  EXPECT_EQ(entries[2].name, "large.dtx");
}

TEST(TextureBrowser, SortEntries_ByDimensions_Descending) {
  std::vector<TextureBrowserEntry> entries;
  entries.push_back({"small.dtx", "", "", 64, 64, 0, "", "", false});
  entries.push_back({"large.dtx", "", "", 512, 512, 0, "", "", false});
  entries.push_back({"medium.dtx", "", "", 256, 256, 0, "", "", false});

  SortTextureEntries(entries, TextureBrowserSortMode::Dimensions, false);

  EXPECT_EQ(entries[0].name, "large.dtx");
  EXPECT_EQ(entries[1].name, "medium.dtx");
  EXPECT_EQ(entries[2].name, "small.dtx");
}

TEST(TextureBrowser, SortEntries_EmptyList) {
  std::vector<TextureBrowserEntry> entries;
  SortTextureEntries(entries, TextureBrowserSortMode::Name, true);
  EXPECT_TRUE(entries.empty());
}

TEST(TextureBrowser, SortEntries_SingleEntry) {
  std::vector<TextureBrowserEntry> entries;
  entries.push_back({"only.dtx", "", "", 0, 0, 0, "", "", false});
  SortTextureEntries(entries, TextureBrowserSortMode::Name, true);
  EXPECT_EQ(entries.size(), 1u);
  EXPECT_EQ(entries[0].name, "only.dtx");
}

// =============================================================================
// TextureBrowserState Tests
// =============================================================================

TEST(TextureBrowserState, DefaultConstruction) {
  TextureBrowserState state;
  EXPECT_EQ(state.view_mode, TextureBrowserViewMode::Grid);
  EXPECT_EQ(state.sort_mode, TextureBrowserSortMode::Name);
  EXPECT_TRUE(state.sort_ascending);
  EXPECT_TRUE(state.entries.empty());
  EXPECT_EQ(state.selected_index, -1);
  EXPECT_TRUE(state.current_texture.empty());
  EXPECT_EQ(state.thumbnail_size, 64);
  EXPECT_TRUE(state.needs_refresh);
}

TEST(TextureBrowserAction, DefaultConstruction) {
  TextureBrowserAction action;
  EXPECT_FALSE(action.texture_selected);
  EXPECT_FALSE(action.apply_to_faces);
  EXPECT_TRUE(action.selected_texture.empty());
}

// =============================================================================
// ScanTextureDirectories Tests
// =============================================================================

TEST(TextureBrowser, ScanTextureDirectories_EmptyRoot) {
  auto entries = ScanTextureDirectories("");
  EXPECT_TRUE(entries.empty());
}

TEST(TextureBrowser, ScanTextureDirectories_NonExistentRoot) {
  auto entries = ScanTextureDirectories("/nonexistent/path/that/does/not/exist");
  EXPECT_TRUE(entries.empty());
}
