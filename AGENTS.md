# AGENTS.md

Guidelines for working with the LTJS (LithTech Jupiter System) codebase.

## Build System

CMake 3.24.4+ with Ninja generator. Use presets for all builds.

### Configure & Build

```bash
# Engine (RelWithDebInfo - recommended for development)
cmake --preset engine
cmake --build --preset engine

# Engine variants
cmake --preset engine-debug && cmake --build --preset engine-debug
cmake --preset engine-release && cmake --build --preset engine-release

# DEdit2 editor
cmake --preset dedit2 && cmake --build --preset dedit2
```

### Available Presets

| Preset | Description | Build Directory |
|--------|-------------|-----------------|
| `engine` | Engine RelWithDebInfo | `build/engine` |
| `engine-debug` | Engine Debug | `build/engine-debug` |
| `engine-release` | Engine Release | `build/engine-release` |
| `dedit2` | DEdit2 RelWithDebInfo | `build/dedit2` |
| `dedit2-debug` | DEdit2 Debug | `build/dedit2-debug` |
| `dedit2-release` | DEdit2 Release | `build/dedit2-release` |

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `LTJS_SDL_BACKEND` | ON | Use SDL backend |
| `LTJS_USE_PCH` | ON | Use precompiled headers |
| `LTJS_USE_CCACHE` | ON | Use ccache if available |
| `LTJS_BUILD_DEDIT` | OFF | Build original DEdit |
| `LTJS_BUILD_DEDIT2` | OFF | Build DEdit2 editor |

### Platform Support

Windows and macOS only. Linux is not supported.

## C++ Standard

**C++20** is required. Project targets use `ltjs_add_defaults()` from `cmake/ltjs_common.cmake` which sets:

```cmake
CXX_STANDARD 20
CXX_STANDARD_REQUIRED ON
CXX_EXTENSIONS OFF
```

### Modern C++ Guidelines

- Use `std::string_view` over `const char*` or `const std::string&` for read-only string parameters
- Use `std::span` for array views instead of pointer+size pairs
- Use `std::optional` for optional return values, not sentinel values or out-params
- Use `std::variant` over unions with type tags
- Use structured bindings: `auto [key, value] = pair;`
- Use range-based for loops and `<ranges>` algorithms
- Use `constexpr` for compile-time computation
- Use `[[nodiscard]]` on functions where ignoring the return value is likely a bug
- Use `enum class` over plain `enum`
- Use `nullptr` over `NULL` or `0`

## Code Formatting

Enforced via `.clang-format` (LLVM-based):

```yaml
BasedOnStyle: LLVM
IndentWidth: 2
TabWidth: 2
UseTab: Never
ColumnLimit: 120
AllowShortCaseLabelsOnASingleLine: true
```

Run formatter:
```bash
clang-format -i <file>
# Or format all project files:
find engine libs/ltjs tools -name '*.cpp' -o -name '*.h' | xargs clang-format -i
```

## Documentation Standards

- **Public APIs**: Use Doxygen `///` or `/** */` with a brief summary, details where non-obvious, and **usage examples** for non-trivial functions.
- **Module headers**: Each renderer header (`engine/runtime/render_a/src/sys/diligent/*.h`) should start with a short Doxygen “module essay” describing purpose, responsibilities, and ownership boundaries.
- **Internal helpers**: Document intent (why), not just mechanics (what). Avoid restating the function name.
- **No docstrings on pure forward declarations**: Document at the primary declaration site instead.

## Engineering Patterns & File Size

- **Single Responsibility Principle (SRP)**: Each file/module should own one concern (e.g., postfx, model draw, world draw, debug draw).
- **Avoid god‑files**: If a file grows beyond ~600–800 lines or spans multiple concerns, split it.
- **State isolation**: Separate “state/ownership” from “operations” wherever practical.
- **API simplification**: Prefer narrowing APIs and pushing details behind module boundaries over adding new global access points.

## Include & API Hygiene

- **Public headers**: Keep minimal and stable; avoid including internal headers unless necessary.
- **IWYU**: Include what you use; minimize transitive dependency reliance.
- **No function forward declarations**: Define before use or include header (per existing rule).

## Refactor Quality Checklist

- **Build proof**: Run the relevant preset build(s) for touched targets when feasible.
- **Coupling**: Reduce cross‑module entanglement; avoid new globals.
- **Readability**: Prefer smaller units with clear names and minimal side effects.
- **Docs**: Update or add docs for any changes to public API behavior.

## Static Analysis

Enabled checks in `.clang-tidy`:

- `bugprone-*` — Common bug patterns
- `performance-*` — Performance pitfalls
- `clang-diagnostic-*` — Compiler diagnostics
- `clang-analyzer-*` — Static analysis checks
- `modernize-use-override` — Require `override` on virtual overrides
- `modernize-use-nullptr` — Use `nullptr` not `NULL`
- `readability-implicit-bool-conversion` — Explicit bool conversions
- `readability-redundant-parentheses` — Remove unnecessary parens

Run clang-tidy:
```bash
clang-tidy <file> -- -std=c++20 -I<include_paths>
# Or with compile_commands.json:
clang-tidy -p build/engine <file>
```

## First-Party Guidelines

### No Forward Declarations of Functions

Define functions before use or use header includes. Forward declarations obscure dependencies and complicate refactoring.

```cpp
// BAD - forward declaration
void helper();
void caller() { helper(); }
void helper() { /* ... */ }

// GOOD - define before use or include header
void helper() { /* ... */ }
void caller() { helper(); }
```

### Header Organization

- Use `#pragma once` for header guards
- Include what you use (IWYU principle)
- Prefer forward declarations for **types** in headers when possible to reduce compile times
- Keep headers minimal; implementation details go in `.cpp` files

### Naming Conventions

Follow existing codebase patterns:
- Classes: `PascalCase`
- Functions/methods: `PascalCase` (legacy) or `camelCase` (newer code)
- Variables: `camelCase` or `snake_case`
- Constants/enums: `UPPER_SNAKE_CASE` or `k` prefix (`kConstantName`)
- Member variables: `m_` prefix (`m_memberVar`)

### Error Handling

- Use exceptions for exceptional conditions
- Use `std::optional` or `std::expected` (C++23) for expected failures
- Never silently swallow errors
- Log errors with context before propagating

### Memory Management

- Prefer automatic storage (stack) over dynamic allocation
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw `new`/`delete`
- Use RAII for all resource management
- Avoid raw owning pointers

### Const Correctness

- Mark methods `const` when they don't modify state
- Pass large objects by `const&` or `std::string_view`/`std::span`
- Use `constexpr` where possible
