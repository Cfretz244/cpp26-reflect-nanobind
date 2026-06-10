# LIB-0004 — taskflow v4.0.0 uses std::bit_ceil/bit_width without including <bit>

- **Status:** RECORDED (library-side; consumer-side `#include <bit>` first, exactly like
  LIB-0001's <cstdlib> workaround for fmt).
- **Found via:** corpus/runs/taskflow (wave 2; dedup key `taskflow-v4-missing-bit-include`).
  `taskflow/core/wsq.hpp` (std::bit_ceil) and `core/executor.hpp` (std::bit_width) compile
  elsewhere only via transitive includes; this repo's from-source libc++ does not provide
  them transitively. Candidate for an upstream taskflow patch.
