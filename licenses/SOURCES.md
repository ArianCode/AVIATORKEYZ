# Source pins — reproducible builds (AviatorKeyz)

| Component | Version / ref | URL |
|-----------|----------------|-----|
| JUCE | `8.0.9` | `https://github.com/juce-framework/JUCE` (Git tag `8.0.9`) |
| CMake minimum | 3.22 | (toolchain, not a vendored dep) |
| C++ standard | C++20 | `set(CMAKE_CXX_STANDARD 20)` in [CMakeLists.txt](../CMakeLists.txt) |

`FetchContent` in [CMakeLists.txt](../CMakeLists.txt) declares:

- `GIT_REPOSITORY` → JUCE as above  
- `GIT_TAG` → `8.0.9`  
- `GIT_SHALLOW` → `TRUE`  

After configure, the framework is typically under your build tree (e.g. `_deps/juce-src`). The **exact** checkout should match `8.0.9` for license and behavior audits.

**Product-specific assets** (not third-party): factory WAV and factory presets — see [IP_SUMMARY.md](IP_SUMMARY.md).
