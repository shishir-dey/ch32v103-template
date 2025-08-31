# CH32V103 Template Project

A complete template project for the CH32V103 RISC-V microcontroller with CMake build system and automated CI/CD.

## Project Structure

```
ch32v103-template/
├── .github/workflows/     # GitHub Actions CI/CD
│   └── build.yml         # Automated build workflow
├── apps/                 # Application code
│   └── hello.c          # Example application
├── core/                 # Core system files
│   ├── ch32v10x_conf.h  # Configuration header
│   ├── ch32v10x_it.c    # Interrupt handlers
│   ├── ch32v10x_it.h    # Interrupt handler declarations
│   └── main.cpp         # Main application entry point
├── cpu/                  # CPU-specific code
│   ├── core_riscv.c     # RISC-V core functions
│   └── core_riscv.h     # RISC-V core definitions
├── driver/               # Hardware abstraction layer
│   ├── inc/             # Driver header files
│   └── src/             # Driver source files
├── lib/                  # Libraries
│   └── debug/           # Debug utilities
├── system/               # System-level code
│   ├── Link.ld          # Linker script
│   ├── startup_ch32v10x.S  # Startup assembly code
│   ├── system_ch32v10x.c   # System initialization
│   ├── system_ch32v10x.h   # System definitions
│   └── syscalls.c       # POSIX syscalls implementation
└── CMakeLists.txt        # CMake build configuration
```

## Prerequisites

### Local Development

1. **RISC-V GCC Toolchain**: Install using xPack
   ```bash
   npm install -g xpm
   xpm install @xpack-dev-tools/riscv-none-elf-gcc@14.2.0-3.1
   ```

2. **CMake**: Version 3.16 or higher
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake
   
   # macOS
   brew install cmake
   
   # Windows
   # Download from https://cmake.org/download/
   ```

3. **Make**: Build system
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential
   
   # macOS (via Xcode Command Line Tools)
   xcode-select --install
   ```

### Toolchain Setup

The toolchain will be installed in:
- Linux/macOS: `~/.local/xPacks/@xpack-dev-tools/riscv-none-elf-gcc/14.2.0-3.1/.content/bin/`
- Windows: `%APPDATA%\xPacks\@xpack-dev-tools\riscv-none-elf-gcc\14.2.0-3.1\.content\bin\`

Add this directory to your PATH environment variable.

## Building

### Quick Start

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd ch32v103-template
   ```

2. **Create build directory**:
   ```bash
   mkdir build
   cd build
   ```

3. **Configure and build**:
   ```bash
   cmake ..
   make -j$(nproc)
   ```

## License

This project template is provided as-is for educational and commercial use. Please check individual component licenses for specific terms.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with the CI/CD pipeline
5. Submit a pull request
