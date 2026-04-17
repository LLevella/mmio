# Contributing

Thank you for improving `mmio`.

## Local Checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Sanitizer checks:

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DMMIO_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --config Debug
ctest --test-dir build-sanitize -C Debug --output-on-failure
```

## Style

- Keep the library header-only and C++17-compatible.
- Keep public headers self-contained.
- Add tests for every API or parser behavior change.
- Prefer small, explicit APIs over broad implicit behavior.

## Releases

Releases use semantic versioning. Tags matching `v*` trigger GitHub Release
creation with packaged archives and checksums.
