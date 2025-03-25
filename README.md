# Embedded Virtual File System (eVFS)

eVFS is a lightweight and flexible virtual file system designed for embedded systems written in C++23. It provides a unified interface for file operations across various storage mediums, enabling seamless integration and portability in embedded applications.

## Features

- **Modular Architecture**: Easily integrates with different storage backends, allowing for customization based on specific project requirements.
- **Platform Agnostic**: Designed to be compatible with various embedded platforms, facilitating code reuse and reducing development time.
- **Efficient Resource Utilization**: Optimized for minimal memory footprint and low computational overhead, making it suitable for resource-constrained environments.
- **Syscalls**: Provides ready-to-use integration with Newlib's syscalls.
- **C++ support**: Enables `std::filesystem` functionality like `std::directory_iterator`
- **Filesystems**: Out-of-box support for ext4, FAT filesystems

## Getting Started

### Prerequisites

- A compatible C/C++ compiler.
- [Meson](https://mesonbuild.com/) build system.

### Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/mp-coding/evfs.git
   ```

2. **Navigate to the Project Directory**:
   ```bash
   cd evfs
   ```

3. **Build the Project Using Meson**:
   ```bash
   meson setup build
   meson compile -C build
   ```
4. **Run tests**:
   ```bash
   meson test -C build
   ```

## Usage

Include the eVFS library in your project via Git submodule or [Meson subprojects](https://mesonbuild.com/Subprojects.html).

eVFS exports several dependencies:
* `evfs_dep` - main library dependency
* `evfs_syscalls_dep` - generic syscalls dependency to integrate with libs other than Newlib
* `evfs_syscalls_newlib_dep` - ready-to-use dependency with implementations for all the required Newlib syscalls

During constructing of eVFS instance you need to provide an implementation of `StdStream`. 
Doing so enables use of `stdin/stdout/stderr`, and corresponding accompanying std API like `printf`, `scanf`, etc.

From this point, you can use of vast sdlib/posix API:
* `fopen/fclose/fwrite/fread/fstat/fseek`
* `mount/umount`
* `stdin/stdout/stderr`
* `mkdir/rmdir/opendir/dirnext`
* ... and many more

## Contributing

Contributions are welcome! Please follow these steps:

1. **Fork the Repository**.
2. **Create a New Branch**:
   ```bash
   git checkout -b feature/YourFeatureName
   ```
3. **Commit Your Changes**:
   ```bash
   git commit -m 'Add some feature'
   ```
4. **Push to the Branch**:
   ```bash
   git push origin feature/YourFeatureName
   ```
5. **Open a Pull Request**.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---