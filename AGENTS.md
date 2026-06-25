# ULTRA Coding Style

Follow these instructions when working in the ULTRA repository.

## Core Rules

- Follow C++23 and the C++ Core Guidelines where practical.
- Preserve nearby style and avoid unrelated reformatting.
- Use Allman braces with two-space indentation.
- Omit braces only for simple one-statement blocks, matching nearby code.
- Use lowercase snake_case for type names and function names.
- Place `const` before the type: `const int value`.
- Raw pointers do not transfer ownership unless explicitly documented.
- Prefer `unsigned` for natural/count values, matching nearby code.
- Prefer `using` over `typedef`.
- Omit parameter names in function declarations unless they add information.
- Prefer direct initialization or brace initialization. Copy initialization is allowed for simple scalar values.
- Use in-class brace initializers for default member values.
- Public data members are acceptable for simple aggregate-like types when they improve readability.

## C++ Headers

- Use `.h` headers and `.cc` implementation files.
- In implementation `.cc` files, include the corresponding interface header first where one exists.
- Use the existing `ULTRA_*_H` include-guard style for new project headers.
- Wrap project declarations in `namespace ultra` or an appropriate nested namespace.
- Order class sections as `public`, then `protected`, then `private`.
- Keep template implementations in headers or included `.tcc` files when needed.
- Prefer this include order, matching nearby code when it differs:
  1. matching prototype/interface header
  2. same-project headers
  3. non-standard external library headers
  4. third-party library headers such as Boost
  5. standard C++ headers
  6. standard C headers

## Contracts And Invariants

- Use `Expects(...)` for preconditions and `Ensures(...)` for postconditions.
- Classes with invariants should expose `[[nodiscard]] bool is_valid() const`.
- Constructors and non-const methods should preserve invariants.
- Do not add redundant checks for another object's invariant when public methods already preserve it.

## Comments And Documentation

- Keep the MPL file banner on source/header files.
- Prefer Doxygen `///` blocks for API comments.
- First Doxygen sentence should be a concise summary.
- Use third person, descriptive wording.
- Do not add `\author`, `\date`, or `\version`.
- `\param` descriptions begin lowercase and do not end with a period.
- Use `\related` for standalone operators/functions conceptually attached to a class.
- Add normal code comments sparingly, only where they clarify non-obvious logic.

## Tests

- Tests live under `src/test`.
- Use doctest with `TEST_SUITE`, `TEST_CASE`, `SUBCASE`, and `CHECK`.
- Add or extend focused tests for new behavior and bug fixes.
- Speed tests are named with the `speed_` prefix and are not registered as normal CTest tests.
- Prefer deterministic tests; explicitly seed randomness when needed.

## Build And Validation

- Configure from the repository root with `cmake -B build/ src/`, or use presets from `src/`, for example `cmake --preset gcc_debug -S src`.
- Build with: `cmake --build build/`.
- Run tests through CTest from the build directory when relevant.
- For significant changes, consider GCC/Clang presets and sanitizer presets.
- Static analysis workflow documented by the repo uses `scan-build`.
- Use `build/compile_commands.json` for C++ checks; run `clang-tidy -p build <file>`, never without the compilation database.

## Contribution Hygiene

- Keep diffs small and logically coherent.
- Do not mix formatting churn with behaviour changes.
- Do not put contributor names in code.
- Commit summaries use prefixes such as `build`, `chore`, `ci`, `docs`, `feat`, `fix`, `perf`, `refactor`, `revert`, `style`, and `test`.
- Commit summary lines use imperative mood and do not end with a period.
