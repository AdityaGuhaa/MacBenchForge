<div align="center">
  
# MacBenchForge

**The Ultimate Native LLM Benchmarking Dashboard for Apple Silicon**

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![macOS](https://img.shields.io/badge/macOS-13.0%2B-black?logo=apple)](#)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-purple)](#)
[![Apple Silicon](https://img.shields.io/badge/Apple-Silicon_M1%2FM2%2FM3%2FM4%2FM5-orange)](#)

</div>

<p align="center">
  MacBenchForge is a lightning-fast, C++ powered local benchmarking suite designed exclusively for Apple Silicon (M-Series) Macs. It provides a stunning, glassmorphism-styled web interface to download, run, and analyze the performance of Large Language Models (LLMs) locally using maximum Metal GPU acceleration.
</p>

---

## Table of Contents
- [Key Features](#key-features)
- [Under the Hood](#under-the-hood)
- [Prerequisites](#prerequisites)
- [Installation & Build](#installation--build)
- [Usage & Dashboard](#usage--dashboard)
- [Configuration (config.toml)](#configuration-configtoml)
- [Distribution (.dmg)](#distribution-dmg)
- [Contributing](#contributing)
- [License & Attribution](#license--attribution)

---

## Key Features

* **Apple Silicon Native**: Built from the ground up to aggressively leverage Apple's Metal Performance Shaders via `llama.cpp`, pushing your Unified Memory and GPU cores to their absolute limit.
* **HuggingFace Hub Integration**: Browse, search, and download `.gguf` models straight from the HuggingFace Hub inside the app. 
* **Native macOS File Picker & Discovery**: Securely load local models right from Finder into the dashboard, or scan common macOS directories (`~/Downloads`, `~/.lmstudio`, etc.) for instant discovery.
* **Intelligent File Filtering**: Automatically ignores non-generative multi-modal projector weights (`mmproj-`) and sub-100MB vocabulary files to ensure benchmark stability.
* **Smart Data Persistence**: SQLite background engine uses `UPSERT` logic to seamlessly track, clean up, and reactivate dormant local models.
* **Zero-Bloat Architecture**: The frontend is 100% Vanilla HTML, CSS, and JS. No React, no Node.js, no `npm install`. Just blazing fast execution.
* **Advanced Benchmarking**: 
  * Test **Time To First Token (TTFT)** and **Tokens per Second (t/s)**.
  * Simulate real-world loads with customizable **Prompt Types** (Short QA, Long Code Generation, etc.) and **Presets** (Quick vs. Thorough).
* **Hardware Telemetry**: Automatically detects your specific M-series chip, CPU/GPU core counts, system RAM, and dynamic VRAM Metal wiring limits (via `iogpu.wired_limit_mb`) in real-time.
* **Beautiful Visualizations**: A sleek, dark-themed glassmorphic UI featuring live progress bars, toast notifications, and `Chart.js` graphs with inline tokens-per-second data labels.
* **Data Portability**: Export your benchmark run history seamlessly to JSON or CSV.

---

## Under the Hood

MacBenchForge bridges the gap between low-level hardware performance and modern web design:
1. **The Backend (C++17)**: Uses `cpp-httplib` for an ultra-lightweight REST API, `SQLiteCpp` for persistent history tracking, and native `osascript` bindings for macOS integration.
2. **The Inference Engine**: Orchestrates the heavily-optimized `llama-bench` tool to isolate LLM throughput away from interface overhead.
3. **The Frontend (Vanilla JS)**: A highly responsive Single Page Application (SPA) served entirely from memory by the C++ backend.

---

## Prerequisites

Before you begin, ensure your Mac meets the following requirements:
- **macOS Ventura (13.0)** or newer.
- An **Apple Silicon Mac** (M1, M2, M3, M4, or M5 series).
- **Homebrew** installed (`/opt/homebrew`).

---

## Installation & Build

1. **Clone the Repository**
   ```bash
   git clone --recursive https://github.com/AdityaGuhaa/MacBenchForge.git
   cd MacBenchForge
   ```

2. **Install Dependencies**
   MacBenchForge relies on standard C++ build tools and `llama.cpp` for its underlying engine.
   ```bash
   brew install curl openssl cmake llama.cpp
   ```

3. **Build the Application**
   ```bash
   mkdir build && cd build
   cmake ..
   cmake --build . -j
   ```

4. **Launch**
   ```bash
   ./bin/MacBenchForge
   ```
   The backend will start and your default web browser will automatically open to `http://localhost:7860`.

---

## Usage & Dashboard

When you launch MacBenchForge, the First-Run Wizard will analyze your hardware and tailor the environment to your system.

### The Tabs:
- **Dashboard**: High-level overview of your fastest runs, recent activity, and total models.
- **Models**: View locally scanned models or use the **"Pick from Finder"** button to instantly add `.gguf` files from anywhere on your Mac.
- **HuggingFace**: Search the global AI community for new models or grab one of our Curated Apple-Silicon recommendations.
- **Benchmark**: Select a model, choose your workload (e.g. *Medium Reasoning*), and hit Start. Watch the live hardware telemetry as it runs.
- **History & Export**: Graph your historical data, compare models side-by-side on radar charts, and export results for external analysis.

---

## Configuration (config.toml)

All core behavior is driven by `config.toml`, generated next to your executable. 

Key tunable parameters include:
* `benchmark.threads`: The number of CPU threads used during processing (Default: 8).
* `benchmark.gpu_layers`: Layers offloaded to the GPU (Default: 99 for full Metal acceleration).
* `general.port`: Change the UI port if `7860` is in use.
* `huggingface.api_token`: Add your HF token to access gated/private models.

---

## Distribution (.dmg)

Want to share your compiled build with others? MacBenchForge includes a packaging script to generate a beautiful, self-contained macOS Disk Image (`.dmg`).

*Note: You must build the project first (see above).*
```bash
./packaging/create_dmg.sh
```
This drops a deployable `.dmg` in your root folder. *(Keep in mind: end-users will still need to run `brew install llama.cpp` to execute the benchmarks).*

### Troubleshooting: "App is damaged" Error

Because macOS applies a quarantine flag to downloaded `.dmg` files from unidentified developers, you may encounter an error stating the app is "damaged and can't be opened" when first trying to run it.

To bypass this error, simply **Right-Click (or Control-Click)** the `MacBenchForge.app` inside your Applications folder and select **Open**.

Check out this quick video tutorial on how to resolve the issue:

[![Gatekeeper Fix Tutorial](https://img.youtube.com/vi/XOzACmwUTOY/0.jpg)](https://www.youtube.com/watch?v=XOzACmwUTOY)

---

## Security

If you discover a security vulnerability in MacBenchForge, please see our [Security Policy](SECURITY.md) for instructions on how to securely report it. We take security seriously and appreciate your help in keeping the app safe.

---

## Contributing

We welcome pull requests! Whether it's adding new prompt types, expanding the hardware telemetry, or optimizing the C++ routes. 

1. Fork the Project.
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`).
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`).
4. Push to the Branch (`git push origin feature/AmazingFeature`).
5. Open a Pull Request.

Please ensure your code follows the existing style guidelines (C++17 standards, `#pragma once` headers, and encapsulated namespaces).

---

## License & Attribution

This project is licensed under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for the full legal text.

**Attribution Request**  
MacBenchForge is a labor of love. If you use this tool, modify it for your own purposes, or build a commercial/open-source product on top of it, **please provide credit to Aditya Guha**. 

A simple inclusion of *"Powered by MacBenchForge by Aditya Guha"* or a link back to this GitHub repository in your project's `README.md` or application "About" page is highly appreciated!
