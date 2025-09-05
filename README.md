# CH32V103 Template Project

A complete template project for the CH32V103 RISC-V microcontroller with CMake build system and automated CI/CD.

## Project Structure

```
ch32v103-template/
├── .gitignore
├── CMakeLists.txt
├── LICENSE
├── README.md
├── .github/
├── apps/                 # Application examples
│   └── framework/        # Application framework
├── core/                 # Core system files
├── cpu/                  # CPU-specific code
├── driver/               # Hardware abstraction layer
│   ├── inc/             # Driver header files
│   └── src/             # Driver source files
├── lib/                  # Libraries
│   └── debug/           # Debug utilities
└── system/               # System-level code
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
   
## Flashing with WCH-Link

To flash the compiled firmware to the CH32V103 microcontroller, use the WCH-Link tool (wlink).

### Installation

Install wlink using Cargo:

```bash
cargo install --git https://github.com/ch32-rs/wlink
```

### Usage

WCH-Link is a flash tool for WCH's RISC-V MCUs (CH32V, CH56X, CH57X, CH58X, CH59X, CH32L103, CH32X035, CH641, CH643).

Example: Flash the firmware binary to address 0x08000000:

```bash
wlink flash --address 0x08000000 ./firmware.bin
```

## License

This project template is provided as-is for educational and commercial use. Please check individual component licenses for specific terms.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with the CI/CD pipeline
5. Submit a pull request
