<p align="center">
  <picture>
  	<source media="(prefers-color-scheme: dark)" srcset="logo/light.png">
  	<source media="(prefers-color-scheme: light)" srcset="logo/dark.png">
  <img alt="inspector-logo" src="logo/dark.png" width="400">
</picture>
  <h3 align="center">A Binary Instrumentation Tool <br> to Analyze Load Instructions in Off-the-Shelf x86-64 Binaries
  </h3>
</p>

<p align="center">
    <a href="https://github.com/CMU-SAFARI/Load-Inspector/blob/master/LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-yellow.svg"></a>
    <a href="https://github.com/CMU-SAFARI/Load-Inspector/releases"><img alt="GitHub release" src="https://img.shields.io/github/release/CMU-SAFARI/Load-Inspector"></a>
    <a href="https://arxiv.org/abs/2209.00188"><img src="https://img.shields.io/badge/cs.AR-2209.00188-b31b1b?logo=arxiv&logoColor=red" alt="DOI"></a>
</p>

<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#what-is-load-inspector">What is Load Inspector?</a></li>
    <li><a href="#citation">Citation</a></li>
    <li><a href="#tested-configuration">Tested Configuration</a></li>
    <li><a href="#installation">Installation</a></li>
    <li><a href="#using-the-tool">Using the Tool</a></li>
    <li><a href="#optional-arguments">Optional Arguments</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>

## What is Load Inspector?

The Load Inspector is a binary instrumentation tool that can analyze load instructions in any off-the-shelf x86(-64) binary to dump various statistics including (but not limited to):

  * Number of dynamic loads
  * Histogram of dynamic loads based on:
    * Load data size
    * Load addressing mode: PC-relative (aka RIP-relative), stack-relative, register-relative
  * Loads that always fetch the same data from the same memory address (aka stable loads as per [this paper]()), and their distribution based on :
    * Data size
    * Addressing mode

Load Inspector uses [Intel® Software Development Emulator (SDE)](https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html) API to emulate and instrument any x86(-64) binary running on an Intel processor. Load Inspector also supports the following features out-of-the-box:
  
  * Instrument binaries compiled using advanced (and maybe unsupported by the native machine) x86-64 ISA extensions (e.g., [Intel® Advanced Performance Extension (APX)](https://www.intel.com/content/www/us/en/developer/articles/technical/advanced-performance-extensions-apx.html) that doubles the architectural registers from 16 to 32).
  * Instrument only the regions of interest (ROI) of a binary (or a specific thread of a binary).

## Citation

The Load Inspector tool was a part of the following research paper published in ISCA 2024.

> _Rahul Bera, Adithya Ranganathan, Joydeep Rakshit, Sujit Mahto,
Anant V. Nori, Jayesh Gaur, Ataberk Olgun, Konstantinos Kanellopoulos,
Mohammad Sadrosadati, Sreenivas Subramoney, and Onur Mutlu, "[Constable: Improving Performance and Power Efficiency
by Safely Eliminating Load Execution]()", In Proceedings of the 51st International Symposium on Computer Architecture (ISCA), 2024_

If you find this tool useful for your research, please cite using:

```
@inproceedings{constable,
  author = {Bera, Rahul and Ranganathan, Adithya and Rakshit, Joydeep and Mahto, Sujit and Nori, Anant V. and Gaur, Jayesh and Olgun, Ataberk and Kanellopoulos, Konstantinos and Sadrosadati, Mohammad and Subramoney, Sreenivas and Mutlu, Onur},
  title = {{Constable: Improving Performance and Power Efficiency
by Safely Eliminating Load Execution}},
  booktitle = {Proceedings of the 51st International Symposium on Computer Architecture},
  year = {2024}
}
```

## Tested Configuration

The tool has been tested with the following system configuration:

  * Ubuntu 22.04.1 LTS, Kernel: 5.15.0-56-generic
  * Intel® SDE 9.33
  * gcc 11.3.0
  * python 3.8.0
  * GNU Wget 1.21.2

We have also tested binaries compiled by the following compilers:

  * gcc/g++ 11.3.0 (with optimizations for up-to Skylake uArch)
  * Clang 18.1.4 (with `-mapxf` flag for Intel® APX ISA extension)

## Installation

Clone the repository:
  
  ```bash
    git clone https://github.com/CMU-SAFARI/Load-Inspector.git
  ```

Run the installation script:
   
   ```bash
   cd Load-Inspector/
   ./install.sh
   ```

This should automatically download the latest version of Intel® SDE and compile the tool using the SDE API.

## Using the Tool

First, initialize the shell environment by:

  ```bash
  cd Load-Inspector/
  source setvars.sh
  ```

Compile a test binary:

  ```bash
  cd test/fft
  make all
  ```

Instrument the test binary:

  ```bash
  # inspector <optional arguments> -- <target binary> <arguments to target binary>
  inspector -- ./fft
  ```

This should generate a stats file `inspector.stats.txt` in the run directory containing all statistics.

## Optional Arguments

Load Inspector supports the following optional arguments. To get the full list, run `inspector -h`:

| Argument | Type | Description | Default Value |
| ---------| -----| ------------| --------------|
| `-o`, `--output` | String | Specifies the output filename prefix. | `"inspector"` |
| `--dump-loads` | Boolean | If provided 1, the tool will dump a CSV file containing all load PCs that are stable across the instrumentation. | 0 |
| `--start-icount` | Integer | If provided non-zero, the tool will start instrumenting the binary from the given instruction count. Zero signifies instrumentation from the beginning. | 0 |
| `--instr-length` | Integer | if provided non-zero, the tool will stop the instrumentation after the given number of instruction has been retired. Zero signifies the instrumentation will continue till the end of the binary. | 0 |

### Some Examples

1. To instrument `/bin/ls` binary with optional arguments `-lhrt` end-to-end and generate statistics in file named `ls_profile`:
    
    ```bash
    inspector -o ls_profile -- /bin/ls -lhrt
    ```

2. To dump the PCs of all stable load instructions from `test/fft/fft` binary in a file named `fft_stable_loads`:
    
    ```bash
    inspector -o fft_stable_loads --dump-loads 1 -- test/fft/fft
    ```

3. To instrument a region of interest of the `test/fft/fft` between 3M and 7M instructions:

    ```bash
    inspector --start-icount 3000000 --instr-length 4000000 -- test/fft/fft
    ```

## License

Distributed under the MIT License. See `LICENSE` for more information.

## Contact

Rahul Bera - write2bera@gmail.com