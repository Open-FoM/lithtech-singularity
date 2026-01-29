#pragma once

#include "../editor_state.h"

/// Create a NodeProperties with defaults for the given type.
/// Extracted to allow inclusion in tests without heavy dependencies.
NodeProperties MakeProps(const char* type);
