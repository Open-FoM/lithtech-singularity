#include "app/document_state.h"

#include <gtest/gtest.h>

TEST(DocumentState, InitiallyClean)
{
  DocumentState state;
  EXPECT_FALSE(state.IsDirty());
  EXPECT_EQ(0u, state.GetSavePoint());
}

TEST(DocumentState, MarkDirtySetsDirty)
{
  DocumentState state;
  state.MarkDirty();
  EXPECT_TRUE(state.IsDirty());
}

TEST(DocumentState, MarkSavedClearsDirty)
{
  DocumentState state;
  state.MarkDirty();
  EXPECT_TRUE(state.IsDirty());

  state.MarkSaved(5);
  EXPECT_FALSE(state.IsDirty());
  EXPECT_EQ(5u, state.GetSavePoint());
}

TEST(DocumentState, UpdateFromUndoPositionDirtyWhenDifferent)
{
  DocumentState state;
  state.MarkSaved(10);
  EXPECT_FALSE(state.IsDirty());

  // Position differs from save point - should be dirty
  state.UpdateFromUndoPosition(15);
  EXPECT_TRUE(state.IsDirty());

  // Also dirty when position is less than save point (undo past save)
  state.UpdateFromUndoPosition(5);
  EXPECT_TRUE(state.IsDirty());
}

TEST(DocumentState, UpdateFromUndoPositionCleanWhenAtSavePoint)
{
  DocumentState state;
  state.MarkSaved(10);

  // Make it dirty first
  state.UpdateFromUndoPosition(15);
  EXPECT_TRUE(state.IsDirty());

  // Return to save point - should be clean
  state.UpdateFromUndoPosition(10);
  EXPECT_FALSE(state.IsDirty());
}

TEST(DocumentState, ResetClearsState)
{
  DocumentState state;
  state.MarkSaved(10);
  state.MarkDirty();
  EXPECT_TRUE(state.IsDirty());
  EXPECT_EQ(10u, state.GetSavePoint());

  state.Reset();
  EXPECT_FALSE(state.IsDirty());
  EXPECT_EQ(0u, state.GetSavePoint());
}

TEST(DocumentState, SavePointTracksUndoPosition)
{
  DocumentState state;

  // Simulate: make edits (position 1, 2, 3), save at 3
  state.UpdateFromUndoPosition(1);
  state.UpdateFromUndoPosition(2);
  state.UpdateFromUndoPosition(3);
  EXPECT_TRUE(state.IsDirty());

  state.MarkSaved(3);
  EXPECT_FALSE(state.IsDirty());

  // Undo to position 2
  state.UpdateFromUndoPosition(2);
  EXPECT_TRUE(state.IsDirty());

  // Redo back to position 3
  state.UpdateFromUndoPosition(3);
  EXPECT_FALSE(state.IsDirty());

  // Make more edits past save point
  state.UpdateFromUndoPosition(4);
  EXPECT_TRUE(state.IsDirty());
}
