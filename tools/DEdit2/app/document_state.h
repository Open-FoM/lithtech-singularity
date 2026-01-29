#pragma once

#include <cstddef>

/// Tracks document dirty state (unsaved changes).
///
/// Integrates with the undo system by tracking the save point -
/// the undo stack position at the time of the last save.
class DocumentState {
public:
  /// Mark the document as modified.
  void MarkDirty() { m_dirty = true; }

  /// Mark the document as saved (clears dirty flag and updates save point).
  void MarkSaved(size_t undo_position) {
    m_dirty = false;
    m_save_point = undo_position;
  }

  /// Check if document has unsaved changes.
  [[nodiscard]] bool IsDirty() const { return m_dirty; }

  /// Get the save point (undo stack position at last save).
  [[nodiscard]] size_t GetSavePoint() const { return m_save_point; }

  /// Update dirty state based on current undo position.
  ///
  /// The document is dirty if the current undo position differs from the save point.
  /// This clears dirty when undoing back to the save point.
  void UpdateFromUndoPosition(size_t current_position) {
    m_dirty = (current_position != m_save_point);
  }

  /// Reset to clean state (for new documents).
  void Reset() {
    m_dirty = false;
    m_save_point = 0;
  }

private:
  bool m_dirty = false;
  size_t m_save_point = 0;
};
