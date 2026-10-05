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

## Run

There are 2 options to run the static code analysis:

1. In command line:
    ```sh
    ./scripts/codechecker/codechecker_run.sh
    ```
    To generate JSON (Code Climate format, handy in MRs) and HTML reports, add the `--gen-reports` flag. To see HTML reports, open `./.codechecker/reports_html/index.html` in your favorite web browser.

2. Use [VS Code CodeChecker add-on](https://marketplace.visualstudio.com/items?itemName=codechecker.vscode-codechecker).

## Remarks
The current CodeChecker configuration is in YAML format, as it is more human-readable than JSON and also supports comments.

# Rebuilding lib-nonpublic binaries

The SDK ships pre-built binary objects in `bin/`, produced from a separate
`ts-tr01-lib-nonpublic` repository. To rebuild them — for an update, or to
verify what's committed matches the recorded source commit — use
`build_lib_nonpublic.sh`.

The script needs the lib-nonpublic repo URL. Supply it one of three ways:

```sh
# 1. environment variable (e.g. in your shell rc)
export TS_LIB_NONPUBLIC_REPO_URL=ssh://…/ts-tr01-lib-nonpublic.git

# 2. one-shot env override
TS_LIB_NONPUBLIC_REPO_URL=ssh://…/ts-tr01-lib-nonpublic.git ./build_lib_nonpublic.sh

# 3. explicit flag
./build_lib_nonpublic.sh --repo=ssh://…/ts-tr01-lib-nonpublic.git
```

Internal Tropic Square developers can get the URL from the project maintainers
or from the GitLab project CI/CD variables page. CI pipelines pick it up
automatically from the `TS_LIB_NONPUBLIC_REPO_URL` CI/CD variable on this
project.

## How the source commit is chosen

The source commit is pinned in `bin/lib-nonpublic.manifest`. Which commit the
script actually builds depends on whether `--ref` is given and whether the
manifest already exists:

| `--ref` given | Manifest present | What the script does                                                                            |
|---------------|------------------|-------------------------------------------------------------------------------------------------|
| yes           | —                | clones the given commit/branch; manifest is rewritten with that SHA                             |
| no            | yes              | reads the SHA from the manifest and clones that commit — reproduces the committed binaries      |
| no            | no               | clones remote HEAD and writes a fresh manifest (bootstrap)                                      |

So: re-running with no flags reproduces what's checked in; bumping the pin to
a newer source commit is an explicit `--ref` override; the very first run
(when no manifest exists yet) doesn't need any flags.

Other flag:

- `--yes` / `-y` — non-interactive; delete an existing `./lib-nonpublic/` clone
  without prompting.

Commit the manifest together with the rebuilt binaries in the same SDK commit
so the link from each binary to its source commit is recoverable. The
`verify_lib_nonpublic_binaries` CI job re-runs the build at the SHA recorded
in the manifest and fails the pipeline if the committed binaries don't match.
