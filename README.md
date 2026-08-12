# ts-sw-sdk

SDK for TASSIC.
Intended to use as submodule in any firmware root repo.

# How to use

Add this as submodule to any directory (recommended name `sdk`).
Add to main `CMakeList.txt` before keyword `project`:

```
set(CSP "umc55")   # select CSP to use
set(MEMORY "ROM")  # select linker file (RAM/ROM) based target
# use user defined linker script if MEMORY variable undefined

set(DIR_SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk) # set variable where your SDK is located
include(${DIR_SDK}/sdk.cmake)  # configure toolchain and build SDK

# now continue with your project definitions
project( ... )

```

## Build dependencies

To build FW using this SDK on Linux machine, you need following dependencies
- `cmake` - version 3.24.3 or higher
- `lowrisc-toolchain` - See [Lowrisc toolchain releases](https://github.com/lowRISC/lowrisc-toolchains/releases).
   Must set `RISCV_TOOLCHAIN_ROOT` environment variable to location with the toolchain downloaded.


To enable GCC toolchain in Tropic Square internal environment :

```
# Generate temporary file which will configure SW
ts_sw_cfg.py -v sdk/csp/umc55/ts_sw_setup.yml

# Source it and get rid of it
source ./source_me_to_get_all_set
rm source_me_to_get_all_set
```


# Directory structure

```
├── api          # Tropic01 API definitions
├── common       # platform independent shared libraries and headers
├── csp          # Chip Support Packages 
│   ├── linux    # demo CSP for linux platform build (testng purposes)
│   └── umc55    # main platform Chip Support Package for TASSIC
│       └── inc  # optional additional includes for current CSP
├── drv          # drivers set for all supported platforms
└── hal          # main SDK API files
```

# Build documentation

We use Doxygen (1.10.0) and LaTeX (pdfTeX 3.141592653-2.6-1.40.24 (TeX Live 2022))

## Build HTML documentation
```
$ mkdir build/
$ cd build/
$ cmake ../examples/demo_umc55/ -DBUILD_DOCS=1
$ make doc_doxygen
```
Generated documentation will be available in directory `build/docs/doxygen/html`.

Start reading here: `build/docs/doxygen/html/index.html`.

## Build PDF documentation
*Not supported in current version.*

# Static code analysis

The CodeChecker tool is used to perform static code analysis and generate reports.

You need to install CodeChecker and dependencies. Check out the [official repository](https://github.com/Ericsson/codechecker) for guidance.

## Generating reports

There are 2 options to get reports:

1. Generate HTML report using CLI.
    - Run following commands:

    ```sh
    CodeChecker check -b "./scripts/codechecker/codechecker_build.sh" --config ./scripts/codechecker/codechecker_config.yml
    CodeChecker parse -e html ./.codechecker/reports -o ./.codechecker/reports_html
    ```
    
    - Open `./.codechecker/reports_html/index.html` in your favorite web browser.

2. Use VS Code CodeChecker add-on.
    - Install the add-on from [marketplace](https://marketplace.visualstudio.com/items?itemName=codechecker.vscode-codechecker).
    - Add these lines to your VS Code's workspace settings (`.vscode/settings.json`):

    ```json
    "codechecker.executor.executablePath": <path to CodeChecker>,
    "codechecker.analyze.arguments": "--config ./scripts/codechecker/codechecker_config.yml",
    "codechecker.log.buildCommand": "./scripts/codechecker/codechecker_build.sh",
    "codechecker.backend.compilationDatabasePath": "${workspaceFolder}/.codechecker/compile_commands.json"
    ```

    - Note: `<path to CodeChecker>` can be replaced with "CodeChecker" if the CodeChecker is available in your `$PATH`. Otherwise, you need to specify full path to the CodeChecker executable.
    - Refer to the add-on's documentation on how to run analysis. Usually, you need to click the "Run CodeChecker log" and "Analyze entire project" buttons.
    - After running analysis, errors will be available either in CodeChecker panel or in VS Code's problems tab in the bottom.

## Remarks
The current CodeChecker configuration is in YAML format, as it is more human-readable than JSON and also supports comments.  
The configuration file enables some strict checkers, which may produce a lot of warnings. It is recommended to run the analysis using the full configuration at least once. After that, you can manually disable any checkers you find unnecessary.
