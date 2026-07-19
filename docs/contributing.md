# Contributing to MacBenchForge

We welcome contributions! Please follow these guidelines to help keep the project clean and maintainable.

## Development Setup

1. **Prerequisites**: macOS 13+, Apple Silicon, `cmake`, `curl`, `openssl`.
2. **Build**:
   ```bash
   mkdir build && cd build
   cmake ..
   make -j
   ```
3. **Run**: `./bin/MacBenchForge`

## Code Style

- **Standard**: C++17
- **Namespaces**: All code should reside within the `macbenchforge` namespace.
- **Headers**: Use `#pragma once` for include guards.
- **Formatting**: We use `clang-format`. Please format your code before submitting a PR.
- **Error Handling**: Prefer exceptions for fatal initialization errors, but use return codes or `std::optional` for expected runtime failures (like a failed download).

## Pull Request Process

1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/my-new-feature`).
3. Commit your changes (`git commit -am 'Add some feature'`).
4. Push to the branch (`git push origin feature/my-new-feature`).
5. Create a new Pull Request.

Ensure your code compiles without warnings on Apple Silicon and that the frontend works in Safari/Chrome.

## Security Vulnerabilities

If you discover a security vulnerability, please do not report it in public issues or pull requests. See our [Security Policy](../SECURITY.md) for instructions on how to securely report it.
