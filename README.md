# Yalaz Engine

A professional, cross-platform 3D rendering engine built with **Vulkan** featuring PBR materials, real-time lighting, and a modern editor interface.

![Vulkan](https://img.shields.io/badge/Vulkan-1.3+-red.svg)
![C++20](https://img.shields.io/badge/C++-20-blue.svg)
![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20macOS%20%7C%20Linux-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

---

## Table of Contents

1. [For Users (Just Run the App)](#for-users-just-run-the-app)
2. [For Developers (Build from Source)](#for-developers-build-from-source)
3. [Detailed Requirements](#detailed-requirements)
4. [Step-by-Step Build Guide](#step-by-step-build-guide)
5. [Understanding Build Presets](#understanding-build-presets)
6. [Creating a Release](#creating-a-release)
7. [IDE Setup](#ide-setup)
8. [Project Structure](#project-structure)
9. [Troubleshooting](#troubleshooting)
10. [For Contributors](#for-contributors)

---

## For Users (Just Run the App)

**You do NOT need to build anything.** Download a pre-built release:

### Step 1: Download

Go to [Releases](https://github.com/emrebilici/Yalaz-Engine/releases) and download:

| Your System | Download This File |
|-------------|-------------------|
| Windows 10/11 | `Yalaz-Engine-x.x.x-Windows-AMD64.zip` |
| macOS (Apple Silicon M1/M2/M3) | `Yalaz-Engine-x.x.x-Darwin-arm64.zip` |
| macOS (Intel) | `Yalaz-Engine-x.x.x-Darwin-x86_64.zip` |
| Linux | `Yalaz-Engine-x.x.x-Linux-x86_64.tar.gz` |

### Step 2: Extract

- **Windows:** Right-click → Extract All
- **macOS:** Double-click the .zip file
- **Linux:** `tar -xzf Yalaz-Engine-*.tar.gz`

### Step 3: Run

| System | How to Run |
|--------|-----------|
| **Windows** | Double-click `Yalaz-Engine.bat` |
| **macOS** | Double-click `Yalaz-Engine.command` (first time: right-click → Open) |
| **Linux** | Run `./yalaz-engine.sh` in terminal |

**That's it!** No installation, no dependencies to install. Everything is included.

### What's in the Release Folder

```
Yalaz-Engine/
├── Yalaz-Engine.bat/.command/.sh    # Launcher script (double-click this)
├── bin/
│   ├── Yalaz_Engine                 # The actual executable
│   ├── SDL2.dll / libSDL2.dylib     # Window management library
│   └── (other required libraries)
├── shaders/                          # Compiled GPU shaders
└── assets/                           # Sample 3D models (optional)
```

---

## For Developers (Build from Source)

### What You Need (Requirements Summary)

| Tool | Required? | Why |
|------|-----------|-----|
| **Git** | YES | To clone the repository |
| **CMake 3.15+** | YES | Build system |
| **C++20 Compiler** | YES | To compile the code |
| **Vulkan SDK 1.3+** | YES | Graphics API + shader compiler |
| **Ninja** | NO (optional) | Faster builds, but Make/VS/Xcode work too |

### Quick Start (Copy-Paste Commands)

**macOS:**
```bash
# Install tools
xcode-select --install
brew install cmake ninja

# Install Vulkan SDK from https://vulkan.lunarg.com/sdk/home

# Clone and build
git clone --recursive https://github.com/emrebilici/Yalaz-Engine.git
cd Yalaz-Engine
cmake --preset macos-release
cmake --build build/release
cmake --build build/release -t release

# Run
./release/Yalaz-Engine.command
```

**Linux (Ubuntu/Debian):**
```bash
# Install tools
sudo apt-get update
sudo apt-get install -y git cmake ninja-build build-essential \
    libvulkan-dev vulkan-tools glslang-tools \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

# Clone and build
git clone --recursive https://github.com/emrebilici/Yalaz-Engine.git
cd Yalaz-Engine
cmake --preset linux-release
cmake --build build/release
cmake --build build/release -t release

# Run
./release/yalaz-engine.sh
```

**Windows (Visual Studio):**
```cmd
:: Install Visual Studio 2022 with "Desktop development with C++"
:: Install Vulkan SDK from https://vulkan.lunarg.com/sdk/home
:: Install Git from https://git-scm.com/

:: Clone and build
git clone --recursive https://github.com/emrebilici/Yalaz-Engine.git
cd Yalaz-Engine
cmake --preset vs2022
cmake --build build/vs2022 --config Release
cmake --build build/vs2022 --config Release -t release

:: Run
release\Yalaz-Engine.bat
```

---

## Detailed Requirements

### Required Tools (ALL Platforms)

#### 1. Git
**What:** Version control to download the code
**Install:**
- Windows: https://git-scm.com/download/win
- macOS: `xcode-select --install` (included with Xcode tools)
- Linux: `sudo apt install git` or `sudo dnf install git`

**Verify:** `git --version` should show version 2.x+

#### 2. CMake 3.15 or newer
**What:** Build system that generates project files
**Install:**
- Windows: https://cmake.org/download/ (use installer, check "Add to PATH")
- macOS: `brew install cmake` or https://cmake.org/download/
- Linux: `sudo apt install cmake` or `sudo dnf install cmake`

**Verify:** `cmake --version` should show 3.15+

#### 3. C++20 Compiler
**What:** Compiles the source code into executable

| Platform | Compiler Options |
|----------|-----------------|
| **Windows** | Visual Studio 2022 (recommended), Visual Studio 2019, MinGW-w64 |
| **macOS** | Apple Clang (included with Xcode Command Line Tools) |
| **Linux** | GCC 10+, Clang 12+ |

**Windows - Visual Studio 2022 (Recommended):**
1. Download from https://visualstudio.microsoft.com/
2. Run installer
3. Select "Desktop development with C++" workload
4. Install

**Windows - MinGW (Alternative):**
1. Install MSYS2 from https://www.msys2.org/
2. Open "MSYS2 MinGW64" terminal
3. Run: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja`

**macOS:**
```bash
xcode-select --install
# Click "Install" when prompted
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install build-essential
```

**Linux (Fedora):**
```bash
sudo dnf install gcc-c++
```

**Verify:**
- Windows (VS): Open "Developer Command Prompt" and run `cl`
- Windows (MinGW): `g++ --version`
- macOS/Linux: `g++ --version` or `clang++ --version`

#### 4. Vulkan SDK 1.3 or newer
**What:** Graphics API and shader compiler (glslangValidator)

**Windows:**
1. Download from https://vulkan.lunarg.com/sdk/home
2. Run installer
3. **Important:** Keep all components selected (especially "Shader Toolchain")
4. Restart your terminal after installation

**macOS:**
1. Download from https://vulkan.lunarg.com/sdk/home
2. Open the DMG
3. Run the installer package
4. Add to your shell profile (~/.zshrc or ~/.bash_profile):
   ```bash
   export VULKAN_SDK="$HOME/VulkanSDK/1.3.xxx/macOS"
   export PATH="$VULKAN_SDK/bin:$PATH"
   ```
5. Restart terminal

**Linux:**
```bash
# Ubuntu/Debian
sudo apt-get install libvulkan-dev vulkan-tools glslang-tools

# Fedora
sudo dnf install vulkan-devel vulkan-tools glslang

# Arch
sudo pacman -S vulkan-devel vulkan-tools glslang
```

**Verify:** `glslangValidator --version` should work

### Optional Tools

#### Ninja (Faster Builds)
**What:** Fast build system, makes compilation faster
**Required?** NO - CMake can use Make, Visual Studio, or Xcode instead
**Recommended?** YES for command-line builds

**Install:**
- Windows: Download from https://ninja-build.org/ and add to PATH
- macOS: `brew install ninja`
- Linux: `sudo apt install ninja-build` or `sudo dnf install ninja-build`

**If you don't have Ninja:** Use alternative presets:
- Windows: `vs2022` preset (uses Visual Studio)
- macOS: `xcode` preset (uses Xcode)
- Linux: `linux-makefiles-release` preset (uses Make)

### Platform-Specific Dependencies

#### Linux Only - X11 Development Libraries
```bash
# Ubuntu/Debian
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

# Fedora
sudo dnf install libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel

# Arch
sudo pacman -S libx11 libxrandr libxinerama libxcursor libxi
```

#### Linux Only - Wayland (Optional)
```bash
# Ubuntu/Debian
sudo apt-get install libwayland-dev libxkbcommon-dev

# Fedora
sudo dnf install wayland-devel libxkbcommon-devel
```

---

## Step-by-Step Build Guide

### Step 1: Clone the Repository

```bash
git clone --recursive https://github.com/emrebilici/Yalaz-Engine.git
cd Yalaz-Engine
```

**Important:** The `--recursive` flag downloads all dependencies. If you forgot it:
```bash
git submodule update --init --recursive
```

### Step 2: Choose Your Build Configuration

| What You Want | Use This Preset | Generator |
|---------------|-----------------|-----------|
| **Debug build** (for development, has validation) | `macos-debug` / `linux-debug` / `debug` | Ninja |
| **Release build** (optimized, for distribution) | `macos-release` / `linux-release` / `release` | Ninja |
| **Visual Studio project** | `vs2022` | Visual Studio |
| **Xcode project** | `xcode` | Xcode |
| **Makefiles** (no Ninja) | `linux-makefiles-release` | Make |

### Step 3: Configure the Project

**Using Ninja (fastest):**
```bash
# macOS
cmake --preset macos-release    # or macos-debug for development

# Linux
cmake --preset linux-release    # or linux-debug for development

# Windows (with MinGW)
cmake --preset release          # or debug for development
```

**Using Visual Studio (Windows):**
```cmd
cmake --preset vs2022
```

**Using Xcode (macOS):**
```bash
cmake --preset xcode
```

**Using Makefiles (if no Ninja):**
```bash
cmake --preset linux-makefiles-release
```

**What this does:**
- Creates a `build/` folder with project files
- Detects your compiler and Vulkan SDK
- Prepares everything for building

### Step 4: Build the Project

**Ninja/Makefiles:**
```bash
cmake --build build/release        # Release build
cmake --build build/debug          # Debug build
```

**Visual Studio:**
```cmd
cmake --build build/vs2022 --config Release
cmake --build build/vs2022 --config Debug
```

**Xcode:**
```bash
cmake --build build/xcode --config Release
cmake --build build/xcode --config Debug
```

**Speed up with parallel jobs:**
```bash
cmake --build build/release --parallel 8    # Use 8 CPU cores
cmake --build build/release -j              # Use all CPU cores
```

### Step 5: Find Your Executable

After building, the executable is at:

| Build Type | Location |
|------------|----------|
| Debug | `bin/Debug/Yalaz_Engine` |
| Release | `bin/Release/Yalaz_Engine` |

### Step 6: Create Release Folder (Optional)

To create a distributable folder with all dependencies:

```bash
cmake --build build/release -t release
```

This creates `release/` folder with:
- Executable
- All required libraries (SDL2, Vulkan, etc.)
- Compiled shaders
- Assets (if enabled)

---

## Understanding Build Presets

### What is a Preset?

A preset is a pre-configured build setup. Instead of typing long CMake commands, you just use `--preset name`.

### List All Available Presets

```bash
cmake --list-presets
```

### Preset Reference Table

#### Windows Presets

| Preset | Generator | Build Type | When to Use |
|--------|-----------|------------|-------------|
| `debug` | Ninja | Debug | Development with MinGW |
| `release` | Ninja | Release | Production with MinGW |
| `relwithdebinfo` | Ninja | RelWithDebInfo | Profiling with MinGW |
| `minsizerel` | Ninja | MinSizeRel | Smallest binary |
| `package` | Ninja | Release | Creating installers |
| `mingw-debug` | MinGW Makefiles | Debug | No Ninja, MinGW |
| `mingw-release` | MinGW Makefiles | Release | No Ninja, MinGW |
| `vs2022` | Visual Studio 17 | Multi | Visual Studio IDE |
| `vs2022-static` | Visual Studio 17 | Multi | No runtime DLL dependency |

#### macOS Presets

| Preset | Generator | Build Type | When to Use |
|--------|-----------|------------|-------------|
| `macos-debug` | Ninja | Debug | Development |
| `macos-release` | Ninja | Release | Production |
| `macos-relwithdebinfo` | Ninja | RelWithDebInfo | Profiling |
| `macos-package` | Ninja | Release | Creating DMG |
| `xcode` | Xcode | Multi | Xcode IDE |

#### Linux Presets

| Preset | Generator | Build Type | When to Use |
|--------|-----------|------------|-------------|
| `linux-debug` | Ninja | Debug | Development |
| `linux-release` | Ninja | Release | Production |
| `linux-relwithdebinfo` | Ninja | RelWithDebInfo | Profiling |
| `linux-package` | Ninja | Release | Creating packages |
| `linux-makefiles-debug` | Unix Makefiles | Debug | No Ninja |
| `linux-makefiles-release` | Unix Makefiles | Release | No Ninja |

### Build Types Explained

| Type | Optimization | Debug Symbols | Vulkan Validation | Use For |
|------|-------------|---------------|-------------------|---------|
| **Debug** | None (-O0) | Full | ON | Development, debugging |
| **Release** | Maximum (-O3) | None | OFF | Distribution, users |
| **RelWithDebInfo** | High (-O2) | Yes | OFF | Profiling, crash analysis |
| **MinSizeRel** | Size (-Os) | None | OFF | Embedded, small binary |

### Generator Comparison

| Generator | Speed | IDE Support | Requires |
|-----------|-------|-------------|----------|
| **Ninja** | Fastest | No | Ninja installed |
| **Unix Makefiles** | Medium | No | Make (built-in on Unix) |
| **Visual Studio** | Medium | Full VS IDE | Visual Studio |
| **Xcode** | Medium | Full Xcode IDE | Xcode |

---

## Creating a Release

### Method 1: Release Folder (Recommended)

Creates a standalone folder you can zip and share:

```bash
# 1. Build first
cmake --preset macos-release
cmake --build build/release

# 2. Create release folder
cmake --build build/release -t release

# 3. Your distributable is at release/
ls release/
```

**Release folder contents:**

```
release/
├── Yalaz-Engine.command     # Double-click to run (macOS)
├── Yalaz-Engine.bat         # Double-click to run (Windows)
├── yalaz-engine.sh          # Run this (Linux)
├── bin/
│   ├── Yalaz_Engine         # Main executable
│   ├── Yalaz_Engine-1.0.0   # Versioned executable
│   ├── libSDL2.dylib        # SDL2 library
│   ├── libMoltenVK.dylib    # Vulkan→Metal (macOS)
│   └── libvulkan.dylib      # Vulkan loader (macOS)
├── shaders/                  # Compiled .spv shaders only
└── assets/                   # 3D models (if included)
```

### Method 2: Installer Packages

Create proper installers:

```bash
# Build first
cmake --preset macos-package
cmake --build build/package

# Create package
cd build/package
cpack -G ZIP          # ZIP file
cpack -G DragNDrop    # macOS DMG
cpack -G NSIS         # Windows installer
cpack -G DEB          # Debian/Ubuntu package
cpack -G TGZ          # Linux tarball
```

### Exclude Assets (Smaller Release)

Assets are ~900MB. To exclude:

```bash
cmake --preset macos-release -DYALAZ_INCLUDE_ASSETS=OFF
cmake --build build/release -t release
# Result: ~15 MB release folder
```

---

## IDE Setup

### Visual Studio 2022

1. **Open:** File → Open → Folder → select `Yalaz-Engine`
2. **Wait:** VS detects CMakePresets.json automatically
3. **Select preset:** Dropdown in toolbar → choose `vs2022`
4. **Build:** Build → Build All (F7)
5. **Run:** Select `Yalaz_Engine.exe` → Debug → Start Without Debugging (Ctrl+F5)

### Visual Studio Code

1. **Install extensions:**
   - C/C++ (Microsoft)
   - CMake Tools (Microsoft)

2. **Open folder:** File → Open Folder → select `Yalaz-Engine`

3. **Select kit:** Click "No Kit Selected" in status bar → choose your compiler

4. **Select preset:**
   - Press Ctrl+Shift+P
   - Type "CMake: Select Configure Preset"
   - Choose preset (e.g., `macos-release`)

5. **Build:** Press F7 or click Build in status bar

6. **Run:** Press F5 or click Run

### CLion

1. **Open:** File → Open → select `Yalaz-Engine` folder
2. **Presets auto-detected:** CMake profiles appear automatically
3. **Select profile:** Build dropdown → choose configuration
4. **Build:** Build → Build Project (Ctrl+F9)
5. **Run:** Run → Run 'Yalaz_Engine' (Shift+F10)

### Xcode

```bash
# Generate Xcode project
cmake --preset xcode

# Open in Xcode
open build/xcode/Yalaz_Engine.xcodeproj
```

In Xcode:
1. Select `Yalaz_Engine` scheme (top left)
2. Select `Release` or `Debug`
3. Press ⌘R to build and run

---

## Project Structure

```
Yalaz-Engine/
│
├── src/                              # Source code
│   ├── main.cpp                      # Entry point
│   ├── vk_engine.cpp/h               # Core Vulkan engine
│   ├── vk_types.h                    # Type definitions
│   ├── vk_initializers.cpp/h         # Vulkan helpers
│   ├── vk_images.cpp/h               # Texture handling
│   ├── vk_descriptors.cpp/h          # Descriptor sets
│   ├── vk_pipelines.cpp/h            # Graphics pipelines
│   ├── vk_loader.cpp/h               # Model loading
│   ├── camera.cpp/h                  # Camera system
│   └── ui/                           # Editor UI
│       ├── EditorUI.cpp/h            # Main UI class
│       └── panels/                   # UI panels
│
├── shaders/                          # GLSL shaders
│   ├── *.vert                        # Vertex shaders
│   ├── *.frag                        # Fragment shaders
│   ├── *.comp                        # Compute shaders
│   └── *.spv                         # Compiled (generated)
│
├── assets/                           # 3D models & textures
│
├── third_party/                      # Dependencies (submodules)
│   ├── SDL/                          # Window & input
│   ├── imgui/                        # UI framework
│   ├── glm/                          # Math library
│   ├── vma/                          # Memory allocator
│   ├── fastgltf/                     # glTF loader
│   └── ...
│
├── cmake/                            # CMake modules
│   ├── Version.cmake                 # Version number
│   ├── Packaging.cmake               # CPack config
│   └── CopyShaders.cmake             # Shader copy helper
│
├── scripts/                          # Build scripts
│   ├── build-release.sh/bat          # One-click release build
│   ├── build-debug.sh                # One-click debug build
│   └── run-yalaz.sh/bat              # Launcher scripts
│
├── .github/workflows/                # CI/CD
│   └── release.yml                   # Auto-build on tag
│
├── CMakeLists.txt                    # Main CMake file
├── CMakePresets.json                 # Build presets
└── README.md                         # This file
```

---

## Troubleshooting

### "CMake Error: Could not find Vulkan"

**Solution:**
1. Install Vulkan SDK from https://vulkan.lunarg.com/sdk/home
2. Set environment variable:
   ```bash
   # macOS/Linux - add to ~/.zshrc or ~/.bashrc
   export VULKAN_SDK="$HOME/VulkanSDK/1.3.xxx/macOS"

   # Windows - set in System Properties → Environment Variables
   VULKAN_SDK=C:\VulkanSDK\1.3.xxx
   ```
3. Restart terminal
4. Re-run cmake

### "glslangValidator not found"

**Solution:** This is part of Vulkan SDK.
- Windows: Reinstall Vulkan SDK, ensure "Shader Toolchain" is checked
- macOS/Linux: `which glslangValidator` should show path. If not, add to PATH:
  ```bash
  export PATH="$VULKAN_SDK/bin:$PATH"
  ```

### "Ninja not found"

**Solutions:**

Option A - Install Ninja:
```bash
# macOS
brew install ninja

# Linux
sudo apt install ninja-build

# Windows
# Download from https://ninja-build.org/
```

Option B - Use different generator:
```bash
# macOS - use Xcode
cmake --preset xcode

# Linux - use Makefiles
cmake --preset linux-makefiles-release

# Windows - use Visual Studio
cmake --preset vs2022
```

### "fatal error: submodule not found" or missing headers

**Solution:**
```bash
git submodule update --init --recursive
```

### Build fails with weird errors

**Solution - Clean rebuild:**
```bash
rm -rf build/ bin/ lib/ release/
cmake --preset macos-release
cmake --build build/release
```

### Application crashes on startup

**Checklist:**
1. GPU drivers up to date?
2. Vulkan supported? Run `vulkaninfo` to check
3. Shaders compiled? Check `shaders/*.spv` files exist
4. Release folder complete? All files in `bin/`?

### macOS: "App is damaged and can't be opened"

**Solution:**
```bash
# Remove quarantine attribute
xattr -cr /path/to/Yalaz-Engine.command

# Or right-click → Open (first time only)
```

### Linux: "libvulkan.so.1: cannot open shared object file"

**Solution:**
```bash
sudo apt install libvulkan1    # Debian/Ubuntu
sudo dnf install vulkan-loader # Fedora
```

---

## For Contributors

### Version Management

Edit `cmake/Version.cmake`:
```cmake
set(YALAZ_VERSION_MAJOR 1)
set(YALAZ_VERSION_MINOR 0)
set(YALAZ_VERSION_PATCH 0)
set(YALAZ_VERSION_SUFFIX "")  # "-alpha", "-beta", or ""
```

### Creating a GitHub Release

```bash
# 1. Update version in cmake/Version.cmake
# 2. Commit
git add .
git commit -m "Release v1.0.0"

# 3. Tag
git tag -a v1.0.0 -m "Release 1.0.0"

# 4. Push
git push origin main
git push origin v1.0.0
```

GitHub Actions automatically:
1. Builds for Windows, macOS, Linux
2. Creates packages
3. Publishes release

### Dependencies

| Library | Purpose | License |
|---------|---------|---------|
| Vulkan SDK | Graphics API | Apache 2.0 |
| SDL2 | Windowing | zlib |
| Dear ImGui | UI | MIT |
| GLM | Math | MIT |
| VMA | Memory | MIT |
| fastgltf | glTF loading | MIT |
| stb_image | Image loading | Public Domain |

---

## License

MIT License - see [LICENSE](LICENSE) file.

---

<p align="center">
  <b>Yalaz Engine</b><br>
  Professional Vulkan Rendering Engine<br>
  C++20 • Vulkan 1.3 • Cross-Platform
</p>
refresh
analytics trigger Sun Jan 25 15:48:03 +03 2026
analytics trigger Sun Jan 25 15:52:43 +03 2026
trigger Sun Jan 25 16:07:27 +03 2026
