# Integrating the deadcode-api SDK

The SDK is one header (`include/deadcode-api.h`) and one static library
(`deadcode-api.lib` on Windows, `libdeadcode-api.a` on Linux). You compile your
application against the header, link the static library, and place the marker
calls in your code. The engine, `deadcode-cli`, is a separate product that
processes the built binary afterward.

> **Mandatory:** the engine locates markers through your binary's **PDB**. Any
> binary you intend to protect **must** be compiled with debug information and
> linked with `/DEBUG`, in **both** Debug and Release, and the resulting `.pdb`
> must be shipped to `deadcode-cli` alongside the executable. A binary built
> without a PDB cannot be processed. See [the FAQ](faq.md) for what happens if
> you forget.

---

## 1. CMake

The SDK exports a CMake package and an `deadcode-api::deadcode-api` target.

### As a subdirectory

```cmake
add_subdirectory(third_party/deadcode-api)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE deadcode-api::deadcode-api)

# Required so the engine can read marker symbols from the PDB.
if(MSVC)
    target_compile_options(my_app PRIVATE /Zi)
    target_link_options(my_app PRIVATE /DEBUG)
endif()
```

`/Zi` emits debug info even in Release; `/DEBUG` keeps a full PDB at link time.
Apply both to **every** configuration you ship, not just Debug.

### Via an installed package

After `cmake --install`, consumers can use `find_package`:

```cmake
find_package(deadcode-api REQUIRED)
target_link_libraries(my_app PRIVATE deadcode-api::deadcode-api)
```

---

## 2. MSBuild / Visual Studio

1. **Include path** — add the SDK's `include/` directory to
   *C/C++ → General → Additional Include Directories*.
2. **Linker input** — add `deadcode-api.lib` to
   *Linker → Input → Additional Dependencies*, and its directory to
   *Linker → General → Additional Library Directories*.
3. **Debug info (required)** — for **every** configuration including Release:
   * *C/C++ → General → Debug Information Format* → **Program Database (/Zi)**.
   * *Linker → Debugging → Generate Debug Info* → **Generate Debug Information
     (/DEBUG)**.
4. Build. Then ship the `.exe`/`.dll` **and** its `.pdb` to `deadcode-cli`.

The default Release linker settings `/OPT:REF` and `/OPT:ICF` can stay on — the
SDK is built to survive them (see the symbol-survival test).

---

## 3. Manual compilation

### MSVC (x64 shown; use the x86 toolchain for 32-bit)

```bat
:: build the static library
cl /nologo /c /std:c++17 /O2 /Zi /Iinclude src\deadcode-api.cpp /Fo:deadcode-api.obj
lib /nologo deadcode-api.obj /OUT:deadcode-api.lib

:: build and link your app with a PDB
cl /nologo /std:c++17 /O2 /Zi /Iinclude main.cpp deadcode-api.lib /link /DEBUG
```

### clang-cl

Same flags as MSVC (`clang-cl` accepts the MSVC command line):

```bat
clang-cl /std:c++17 /O2 /Zi /Iinclude main.cpp deadcode-api.lib /link /DEBUG
```

### GCC / Clang (Linux — SDK portability only; see below)

```sh
c++ -std=c++17 -O2 -g -c src/deadcode-api.cpp -o deadcode-api.o
ar rcs libdeadcode-api.a deadcode-api.o
c++ -std=c++17 -O2 -g -Iinclude main.cpp libdeadcode-api.a -o my_app
```

---

## Platform support

| Platform | Header compiles | Library builds | Engine processing |
|----------|:---------------:|:--------------:|-------------------|
| Windows (MSVC, clang-cl), x86 + x64 | yes | yes | **yes** |
| Linux (GCC, Clang) | yes | yes | no |

**Engine support is Windows only.** The Linux build exists so the SDK itself is
portable — the header compiles cleanly and the library builds — but
`deadcode-cli` processes PE binaries. There is no ELF processing.

---

## C usage

The API is `extern "C"`, so it is usable from C as well as C++. Compile the
implementation as C++ (it uses C++ for the anti-fold guards) but include the
header from either language:

```c
#include <deadcode-api.h>

int verify(const char* key) {
    DCApiBeginUltra("verify");
    int ok = check(key);
    DCApiEnd();
    return ok;
}
```
