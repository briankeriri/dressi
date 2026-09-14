# C core feasibility (Linear plan lock)

Date: 2026-09-14

## Goal

Answer whether dressi's **core** must stay C++, or can be rewritten in C.
The working hypothesis was that operator overloading is unnecessary (paper
Appendix A is factories). If C is possible, complete the incomplete Linear
C-rewrite plan.

## Verdict

**Possible.** Nothing in the engine requires C++ as a language. Operators
are one-line wrappers around `F::Add` / `F::Mul` / `F::Less`
(`src/core/f_ops.cpp`). There are no virtual graph nodes (`rg` over
`src/` for `virtual`/`override` is empty), no expression templates, no
RTTI IR.

C++ was providing:

1. `shared_ptr` / `weak_ptr` handle-body graph (strong back, weak forward)
   in `src/core/node.h`
2. `std::function` capturing lambdas for bwd / cpu / infer / optimizer
3. VulkanWrapper + `vulkan.hpp` RAII; `BuildGpuPlan` moves persistent
   `plan.imgs` / vtx / textures / uif across rebuilds (`src/vk/executor.cpp`)

Those map to retain-or-arena, `fn + userdata`, and `vulkan.h` plus an
explicit persistent GPU table. glslang stays a C++ archive behind
`glslang_c_interface.h`.

This is not a mechanical `.cpp` → `.c` rename.

## Linear (archived the goal)

- Doc: https://linear.app/sourmist/document/c-rewrite-feasibility-82dfb8ec14dc
- Plan: https://linear.app/sourmist/document/how-we-finish-dressi-c0c25a049664
- Decision (Done): https://linear.app/sourmist/issue/SOU-42/decision-c-core-is-possible
- Parent work: https://linear.app/sourmist/issue/SOU-33/convert-dressi-claude-c20-engine-to-c
- C mappings added on SOU-35/36/37/38/39/40/25/24; SOU-35 retitled
  (callbacks, not vtable)

## Open issues

None for the language question. Implementation starts at SOU-35
(CPU Add/Mul reverse-mode gate).
