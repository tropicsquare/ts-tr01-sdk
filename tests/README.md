# Unit Tests

Host-native unit tests for selected SDK modules. Tests run on a standard
Linux/macOS host using a native C toolchain — no embedded hardware required.

## Framework

[Ceedling](https://github.com/ThrowTheSwitch/Ceedling) 1.0.1 with
[Unity](https://github.com/ThrowTheSwitch/Unity) and
[CMock](https://github.com/ThrowTheSwitch/CMock). Ceedling auto-generates
test runners and mock headers from the source — no manual `main()` functions
or mock boilerplate.

## Structure

```
tests/
├── project.yml                # Ceedling configuration
├── .gitignore                 # Ignores vendor/ and build/
├── test/
│   ├── test_ct_memcmp.c       # Tests for ct_memcmp()  — common/util.c
│   ├── test_crc16.c           # Tests for crc16()      — hal/crc16.c
│   ├── test_scramble.c        # Tests for scramble_*() — drv/scramble.c
│   └── support/
│       └── stubs/
│           ├── common.h       # Host stub shadowing HW-specific common.h
│           └── os.h           # Host stub shadowing HW-specific os.h
└── README.md
```

## Test suites

### `test_ct_memcmp` — constant-time memory comparison

Covers `ct_memcmp()` from `common/util.c`, which is used to compare
cryptographic material (keys, MACs) without leaking timing information.

### `test_crc16` — CRC-16/BUYPASS checksum

Covers `crc16()` and `crc16_byte()` from `hal/crc16.c`.

Parameters: polynomial `0x8005`, initial value `0x0000`, no reflection, no
final XOR. Verified against the standard check value `0xFEE8` for the string
`"123456789"`.

### `test_scramble` — address scrambling for OTP and Flash

Covers `scramble_init()`, `scramble_shuffle()`, `scramble_value()`, and
`scramble_value_reversed()` from `drv/scramble.c`.

Key properties verified:

- **`scramble_value()`** — LSB-first packing (seq[0] → bits 3:0). Used by
  Flash sector and page scrambling.
- **`scramble_value_reversed()`** — MSB-first packing (seq[0] → MSB nibble).
  Used by OTP scrambling for ACAB backward compatibility.
- **Flash PAGE scrambling** — verifies the ACAB-compatible init pattern:
  `scramble_init(seq, 8)` + `scramble_shuffle(seq, 7, seed)` leaves seq[7]=7
  (identity) in the MSB nibble position.
- **OTP scrambling** — documents the full-shuffle + split-register pattern.

## Building and running

### Prerequisites

Ruby >= 2.6 and Ceedling:

```sh
gem install ceedling --no-document
```

### Run all tests

```sh
cd tests
ceedling test:all
```

### Run a single suite

```sh
ceedling test:test_scramble
```

### Adding a new test

1. Create `tests/test/test_<module>.c` with `setUp()`, `tearDown()`, and
   `test_*()` functions.
2. `#include "unity.h"` and the header(s) under test.
3. If the module has HW dependencies, add mock includes (`#include "mock_<dep>.h"`)
   — CMock generates them automatically from the source headers.
4. Run `ceedling test:all` — Ceedling discovers and builds new test files
   automatically.
