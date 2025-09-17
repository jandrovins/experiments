# Acq and release example on assembly level

This example shows how acquire and release semantics are implemented on assembly level.

Compile with:

```bash
clang++  -S -O3 -std=c++11 -masm=intel -c acq_rel.cpp -o acq_rel.s
```