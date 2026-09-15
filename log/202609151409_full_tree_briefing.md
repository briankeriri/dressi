# Full-tree specialist briefing

Date: 2026-09-15

## Goal

Fill context a sampled reading of `CLAUDE.md` + the Sep 14 C-feasibility
log does **not** contain. Eight extra-high specialists each read **every**
file in their slice (not a sample). This log is the in-repo synthesis.
Linear: [Full-tree specialist briefing](https://linear.app/sourmist/document/full-tree-specialist-briefing-ee5103a95170).
Tickets ENG-33/35/36/37/38/39/25 carry ticket-specific landmines as comments.

## Method

| Specialist | Slice (read in full) |
|---|---|
| Notes historian | `CLAUDE.md`, `README*`, `docs/*`, all 20 `log/*.md` (26 markdown files; no other `.md` at root/`docs/`) |
| Graph / AD | `include/dressi/{dressi,dressi_ad,variable,function,f,types,mesh_utils}.h`, `src/core/{node,infer,op_builder,build_backward,cpu_eval}.*`, `src/dressi_ad.cpp` |
| Ops / raster | `f_ops.cpp` (3479 lines), `cpu_raster.*`, `mesh_utils.*`, `f.h` |
| Pack / codegen | `src/pack/*`, `src/codegen/glsl_codegen.*` |
| Vulkan / transfer | `src/vk/{context,executor,transfer}.*` (executor 1030 lines) |
| IBL / PBR | `ibl.h`, `ibl.cpp`, `ibl_math.h`, `pbr_graph.*`, three `pbr_*` examples, `test_ibl_gpu.cpp`, three IBL logs |
| Tests | every file under `tests/` (8 CPU TUs, 8 GPU, 3 headers, 7 Python) |
| Hosts | all examples, `python/`, `android/`, CMake, `scripts/*`, `tools/downsample_exr` |

`AGENTS.md` is only a pointer to `CLAUDE.md`. `third_party/VulkanWrapper`
is **not checked out** in this workspace (`vkw.h` missing); Vulkan
behavior is from dressi call sites + CMake.

## Verdict on coverage

The Sep 14 feasibility log answers **only** “must we stay C++?” (~40
lines). `CLAUDE.md` is the C++ operator’s manual. Together they still
omit lifetime traps, rebuild-threshold equality, alias-unsplit UB,
comment-vs-code stubs (`SetFragDepth`), two HSR backward paths, and
which tests are pixels vs pointer-shape. A C port that read only those
two files would drop HostCached, WIDE, remat guards, Intel tiling,
Adreno queue bits, and face-id `+1`.

Implementation still starts at **ENG-35** (CPU Add/Mul reverse-mode).
This briefing does not start that work.

Linear IDs in `log/202609140430_c_core_feasibility.md` still say
`SOU-*`; live tickets are `ENG-*`. Corrected in this log.

## What sampled summaries miss (the point of this pass)

### Graph / AD

- Default `Variable()` is a **live** FLOAT `{1,1}` leaf. Null is only
  `Variable(nullptr)`. Start C ids at **1**; id `0` is the null sentinel
  (`std::less` / hash treat null and the first node as equivalent).
- `operator==` is **pointer identity**. Two `F::Float(1)` nodes are
  different. `BuildBackward` seeds a **fresh** `F::Float(1)` every call.
- One **process-global** id space for Functions **and** Variables.
- `setRequiresGradRecursively` is **downstream-only**. Call after the
  graph exists. Marking a leaf first → empty VJP.
- `SumContribs` / `SumPixelWise` arity is **exactly 4** (Vulkan
  guaranteed input-attachment floor), even if the device allows more.
- `build_backward.h` still claims loss must be FLOAT `{1,1}`. The cpp
  seeds every pixel of any float image. Follow the cpp + tests.
- Scalar→vector `gy` materialization only handles 2/3/4 components.
  **MAT3/MAT4 fall through to Vec4** (latent if a matrix has `req_grad`).
- Rebuild thresholds are `==`, not `>=`. One FAST (`cnt==2` → STAGE) and
  one FULL (`cnt==8` → SUBSTAGE) per stable dirty-id set, then never
  again until the **set** changes. `if / else if`: if fast==full, only
  STAGE fires. `graph_static_cnt` does **not** reset after a rebuild.
- Dirty-set change without `pruning_active` leaves status FINISHED
  (correct: nothing was dropped).
- Optimizer **params** and `addUpdate` inputs are **always dynamic**,
  even if not dirty. That is why IBL precompute prunes while Adam runs.
- `addUpdate` is **append-only**, no dedup, no clear. Guard with a
  once-flag. Re-entering BACKWARD after wiring desyncs `extra_updates`.
- `addUpdate` during BACKWARD does **not** lower status (BACKWARD ≯
  OPTIMIZER); extras merge in the same OPTIMIZER block after the lambda.
- Identity `return xs` → `CopyImage(img,img)` Vulkan UB. Tests use
  `xs + 0*g` + `markOutput`, or `setGradOutputsEnabled`.
- Constants are **computed zero-input ops** with `const_val` on `y`, not
  leaves. Dropping `IsInlineConst` puts them in `leaf_vars` and expects
  `sendImg`.
- `entry_status == FINISHED` fused-upload gate is captured **before**
  the reactive check. Comment says “no rebuild”; a FAST/FULL frame that
  **started** FINISHED still takes the fused path. Pick one rule in C.
- `Broadcast` is TexelFetch of `{1,1}` (cannot fuse as input
  attachment). Elementwise `{1,1}` Add/Mul inputs **are** SamePixel.
  Two broadcast mechanisms.
- `PeelDepth` has **no CPU kernel**. CPU eval throws.
- `ThrowDressiError` lives in `node.cpp`, not `types.cpp`.
- `PackingMode` is duplicated: public `DressiAD::PackingMode` and
  `dressi::PackingMode` in `substage.h`.
- No `virtual` / `override` in `src/`. C needs RC + four callbacks, not
  a vtable.

### Ops / raster

- **Two HSR backward paths.** Tests: live `RasterizeSoft` → WIDE
  `GatherDistGrad` (COMP) + `ColSum`. Production silhouette:
  `StopGradient(RasterizeSoft)` then rebuild dist via `FaceFetch` +
  `LookupFaces` + `ScreenCoord` (Alg.1-style stochastic). `--technique=aa`
  is a third path (hard raster + FaceId + AntiAlias).
- Face-id landmine: Soft ch1 = **raw `f`**; FaceId / FaceFetch / AA =
  **`f+1`**. Production does `(GetY(rs)+1)*cov`. Mixing without `+1`
  silently gathers the wrong face.
- There is **no** `F::Interpolate`. Python interpolate = LookupFaces +
  FaceFetch `n_samples=0` + bary from rast.
- `SetFragDepth` comment claims codegen writes `gl_FragDepth`. **It does
  not.** Only `__rasterize_soft__` writes depth. Unused except Python
  bind. Do not port as a real depth write.
- Production peeling is `RasterizeSoftPeel` (`peel=1`), not `F::PeelDepth`.
  Peel bias `1e-4` is load-bearing. `PeelDepth` unused in examples/tests.
- `F::Texture` (nearest) has **no CPU kernel**. Nearest gather
  (`__gather_inv_uv__`) also has no CPU kernel. Bilinear is the
  CPU-tested texture path.
- `inv_uv` is a **bwd closure**, not a forward input. C userdata must
  retain it. Dummy inv_uv in PBR viewers is legal because forward ignores it.
- Gather windows: 9×9 dilation for chart-boundary texels, then 7×7
  screen search. Nearest = first-match; bilinear = accumulate tent.
- SoftClip = centroid scale, cap **8**, `d_min` floor `1e-3`. **Not**
  Alg.2 obtuse AABB. `radius_px==0` is identity unwind.
- AA owner preference `{tri(n), tri(self)}`, asymmetric `r`. Stochastic
  AA vtx offset is **`±2` px hardcoded**, not `radius_px`. FaceFetch
  stochastic is `±radius_px`. Hash salts: FaceFetch 0/1, AA 2/3.
- WIDE: GatherDistGrad + AntiAliasBwdVtx use `{V,max_deg}` + `ColSum`.
  Equirect bwd uses `{map_w, map_h*K}` + `TileSum` (no `wide=1` marker).
  `{V,1}` / unsplit map is the 14 ms / 45 ms trap.
- Only COMP Function in `f_ops.cpp`: internal `GatherDistGrad`. Hot
  gathers stay FRAG (measured). RasterizeSoft bwd **always** `wide=true`.
- Operator overloads wrap Add/Sub/Mul/Div/Neg **and** all four
  comparisons, not just Add/Mul/Less.
- `Fract` backward is 1 everywhere. `Sign`/`Floor`/`Step`/comparisons
  are NullBwd. `Clamp`/`SmoothStep` drop bound/edge grads.
- CPU GatherDistGrad scans **all** covered pixels (ignores radius). GPU
  rebuilds soft bbox from hard clip + centroid scale + marker radius.
- `softras_scene.h` test helper uses a clip-space scale shortcut
  (uniform `w`), not the pixel `d_min` formula. Production uses GPU
  `SoftClip`. SoftRas FD **freezes** enlarged soft tris while probing
  hard clip (isolates GatherDistGrad, not SoftClip).
- FaceNeighborsTex **drops** extra neighbors if valence > 3.
- CPU raster later-face-wins ties (`z > zb` continue). No near-plane
  clip (`w<0` skips).

### Pack / codegen

- Remat is RSP-only. **No dedicated remat unit test.** Collision guards
  were found by running an example.
- `IsCloneSafe`: `__` prefix → only `__face_fetch__` / `__screen_coord__`;
  any generic snippet (no `__`) is clone-safe. That includes `PeelDepth`
  (`discard`) and identity `SetFragDepth` — cloning discard ≠ reading 1.0.
- A.8 OmitConstant / OmitDuplicated **strings do not exist**. Partial
  OmitConstant = `IsInlineConst`. Duplicate `Sin(x)` stays duplicate.
- COMP descriptor budgets are **wrong**: COMP `inp_vars` are samplers
  but counted as input attachments, **not** toward `max_sampled_images`.
  Reciprocal: COMP fusion can be rejected by `max_input_attachments`
  even though COMP has none. Storage-image limit **unchecked** (Intel 16).
- Raster-headed fusion: RASTER into FRAG iff `y` is the **sole** inp and
  `tex_vars` empty. After join, `shader_type=RASTER` so nothing else
  pushes in front.
- RASTER packing puts SamePixel inputs in `vtx_vars`, not `inp_vars`.
  Classify by shader type first, not `InputAccess` alone.
- `CountEdges` ignores `tex_vars` and `vtx_vars`.
- Live DressiAD limits: input attachments `min(16, device)`, color 8,
  sampled 32, **stage attachments 64**. Header defaults 8/8/32/16 are
  what packing **unit tests** use.
- `is_special` prefix list matches every `__…__` in `f_ops.cpp`. Keep
  `__prefilter_conv_norm__` / `_bwd__` **before** `__prefilter_conv__`.
- `kTileAwareMarkers` is exact-match and **misses** parameterized
  `__antialias_bwd_vtx__ n=…`. Bunny 30338 faces vs Intel 16384:
  only some markers fold `{N,1}`.
- `WrapSubStagesIntoStages` (Naive) omits `uif_vars`.
- Funcs stored consumer-first; codegen emits **creation-id** order
  (`v0..vN`). FloatLit `%.9g` + `.0` if no exponent.
- HSR NVIDIA packing is **not** bit-invariant after SumPixelWise trees
  (greedy unlock order + FMA). AA is. Not worth pack-time splitting.

### Vulkan / transfer

- Persist **four** maps; they **never shrink**. Pruned vars keep GPU
  memory for `has_cached`. Dropping `vtx_bufs`/`textures` was commit
  `36d7a6e` (degenerate raster, fake loss=0).
- Aliasing is a **pointer assign** (`plan.imgs[upd] = plan.imgs[inp]`).
  Nothing splits them if the next rebuild makes aliasing illegal →
  `CopyImage` onto self (Vulkan UB) **and** DontCare/Undefined on the
  leaf. C **must allocate a new image for `upd`** on that flip.
  `ensure_texture` will not rebuild a sampler after retarget.
- Stale comment at `executor.cpp:53-56` still teaches “never alias.”
- Recv staging **must** request HostCached. First HostVisible|HostCoherent
  on NVIDIA is write-combined; memcpy **reads** ~100 MB/s (4 MB recv
  was 23 ms). Fallback exists if the device has no cached type.
- UBO UniformRead barrier dest is **FragmentShader only**. COMP + uif
  would race. GatherDistGrad currently does not bind uif.
- FRAG/RASTER output visibility dst is FragmentShader only; a later
  COMP that samples a FRAG color is the same class of hole.
- Queue: GRAPHICS|COMPUTE only. **Do not require TRANSFER** (Adreno).
- First physical device. No discrete-over-iGPU scoring.
- Clip VB binding 0 is **always** RGBA32F stride 16. GPU-generated
  FLOAT/VEC2 passes the padding check but is only valid as **attrib**.
  GPU-generated VEC3 VB is forbidden (RGBA32F image ≠ tight VB).
- Leaf VEC3 attrib uses RGB32F vertex format; the dual image is RGBA32F.
- Fused upload is FINISHED-entry only (new images are Undefined).
  Fused **download** is not gated that way.
- COMP `Undefined→General` every recorded frame is intentional discard
  (plan CB recorded once). Do not “fix” to ShaderReadOnlyOptimal.
- `CreateVarImage` always sets `eStorage`. Intel RGBA32F storage was OK;
  the risk is descriptor **count**.
- GeometryShader comes from enabling the **entire** `GetPhysicalFeatures2`
  blob, not a documented subset.
- Tiled `recvImg` of `{N,1}` returns **physical** size, not `{N,1}`.
- Dual resources: a leaf used as geometry **and** texelFetch gets both
  VB and image; one `sendImg` updates both. Faces stay CPU leaves.

### IBL / PBR

- Python `_C` has **no IBL bindings** (no Equirect/Irradiance/Prefilter/
  BrdfLut/TextureBilinear/AvgPool2x2, no `BuildPbrIblMaps`).
- Env opt **must** use `PrefilterConv` on **both** GT and pred.
  Mixing Sample vs Conv leaks formulation into the recovered env.
- Roughness floor 0.045: shading fetch + PrefilterConv α. **Not** in
  PrefilterEnv or BrdfIntegrationLut. Viewer r=0 ≠ env-opt α=0.045².
- `PrefilterConvNorm` and `BrdfIntegrationLut` are **zero-input** static
  ops so they prune while env is dirty. Do not pass env into the norm.
- Equirect WIDE K: `while (map_w*map_h*k < 65536 && k < dir_h) k *= 2`.
  Forgetting TileSum = 45 ms trap. Do not WIDE-split irradiance/prefilter
  bwd (they loop a small output).
- Specular IBL uses `F0 * A + B`, **not** `f_ibl * A + B`. `f_ibl` is
  only for `kD_ibl`.
- `SafeNormalize` (`v * inversesqrt(dot+1e-12)`) stops background NaN.
- Loss is Reinhard **before** gamma (`Pow` at 0 diverges).
- `--fg-only=1` default: 18.11 vs 18.26 dB; ms/iter **unchanged**
  (bg EquirectSample + transpose still run; only the loss mask changes).
  `docs/benchmarks.md` rows are still bg-inclusive 18.23–18.26.
- `--env-reg=0.05` is a 2×2 block-variance prior. Darkening is null
  space, not a bwd bug. Uncapped negatives: render matches, env PSNR −28 dB.
- `LoadGltfScene`: no V-flip + `floor(min)` (DamagedHelmet V∈[1,2]).
  Integer-spanning UV tris unsupported. `LoadGltfMesh` keeps `1-v`.
  Silhouette `--mesh=` uses LoadGltfMesh (flip). PBR uses LoadGltfScene.
- `f.h` comment “precompute ops are forward-only” is **stale**
  (IrradianceConv / PrefilterConv have exact transposes).
- Hammersley is **manual bit reverse**, not `bitfieldReverse`.
- ENG-39 copies working IBL (equirect deviations included). ENG-21 is a
  later A.3 **name façade** (`BuildBasicRenderGraph` etc. are not in
  source). Do not introduce cubemaps on the C port.

### Tests (ENG-25)

Must stay green: FD `CheckGrad` family; CPU forward oracle; GPU vs CPU
with SoftRas/AA **≤8 pixel** fill-rule slack; seed-1 image loss ≡
reduced sum; surrogate VJP; AA back-vertex mag == 0; convergence gates;
UBO uniform update; reactive prune numeric match.

Do **not** copy as the C oracle: `test_graph.cpp` pointer/lifetime
shape; `test_infer.cpp` throw-only; packing IR handle equality;
codegen exact GLSL `v0..vN` strings (unless a C codegen hook);
`test_vk_smoke` (vkw); Python `len(ctx._engines)`.

Gaps (do not invent on the C port unless the P ticket lands):

- No remat unit test.
- No Alg.1 scatter; stochastic FaceFetch n=8 is GPU vs CPU **same seed**,
  not vs FD.
- No obtuse AABB fixture.
- `PeelDepth` untested; C++ SoftRas tests are K=1; Python peels use
  `RasterizeSoftPeel`, not `PeelDepth`.
- AA stochastic n=8 vs FD: not done (Python Adam only; C++ FD uses n=0).
- No tests: `VertexNeighborMean`, `NormalConsistency*`, `SumAll`,
  `PixelFetch`, `TileFetch`, `SetFragDepth`.
- `ExecutorRsp` header overclaims naive-vs-RSP image compare (absent).
- Reactive tests use rebuild counts 2 and **6**, not production 2 and 8.

Typical CheckGrad: `h=1e-3`, `rel=2e-2`, `abs=2e-3` (tol = abs +
rel·|FD|). GPU SoftRas/AA forward slack **≤8**. Identity optimizer
avoided via `xs+0*g` or `setGradOutputsEnabled`.

### Hosts / bench

- Seven examples. `geometryShader` required: silhouette + shape_texture.
  `shape_texture` has **no** `bench.json` (two-phase).
- Viewer-off: `--view-interval=0` everywhere; silhouette (and
  shape_texture) also `--no-view`. image_fitting viewer **pollutes**
  execStep median from **outside** the timed section (0.063 vs ~0.21 ms).
- Skip ≥20 warmup, **median** of the rest (even-N = upper middle, not
  mean of two mids). Python silhouette: 10-iter CUDA sync **blocks**.
- `dressi_native_bench.py` uses **mean** of 200 after 50 warmup — not
  the README protocol. `dressi_torch_bench.py` mean, no CUDA sync.
  Python texture example averages **all** iters including warmup.
- Native-fused Python and C++ must land within noise (~0.22 vs ~0.26 ms
  @512² Avocado). A gap is a measurement bug.
- `set_rebuild_counts(0,0)` on every eager torch Engine. Dirty-skip
  must **pin** source tensors (address reuse → zero texture grads).
- Android: `CMAKE_BUILD_TYPE=Release` even on the debuggable bench APK.
  One SurfaceView, CPU blit, no EGL. Synthetic `"all"` stream. Share
  viewers across shape_texture phases (unregister does not exist).
  Stop/Back needed systemBars padding (were under the nav bar).
- Kotlin block comments nest. Queue: no TRANSFER. NDK 28.2 / AGP 8.13.2 /
  minSdk 29 / arm64 only.
- ENG-40: keep `Run<Name>(args, ExampleHost&)`. C mains **or** C++
  `run.cpp` calling C ABI. Any example-loop change must re-verify
  **desktop** medians. Android JNI wrapping C is out of scope.
- GIF pipeline: `--snapshot=1 --view-interval=0`, **log-spaced** frames.
- `docs/algorithm.md` Milestone 2 still describes Sobol as **UV** jitter.
  Implementation is **camera/projection** jitter (UV jitter was
  chart-level wobble). Do not re-implement UV Sobol from algorithm.md.
- Python texture example uses **Halton** camera jitter; C++ uses Sobol.
- Hardcoded `E:\Dev\dressi2\data\...` in some Python bench scripts.
- C++ regularizer weights do not transfer 1:1 to `dressi.torch`
  (aa 0.5/0.5 vs 0.2/0.2).
- `--single-view` sine-hash resonates with Adam (~−0.03 IoU). Opt-in only.
- In-graph unweld via `SoftClip(r=0)` **passed unit tests**, AA IoU
  0.97→0.94, reverted. Keep CPU-side unweld.

## Do not re-propose (failed / reverted)

UV sampling jitter; GPU-generated `{1,1}` → mid-frame UBO; COMP for hot
gathers; fp16; in-graph unweld; exact `{V,1}` / unsplit-map as production
path; combined upload+render+download as eager silver bullet; identity
optimizer for grad export; scalar `{1,1}` loss requirement; CPU vertex
update loop as architecture; isolating every RASTER (no raster-headed
fusion); special-cased HSR distance bwd instead of AD over FaceFetch;
`--single-view` as default; HSR lap/normal 0.5/0.5; importing torch CUDA
allocs into Vulkan; hardware `texture()` / cubemaps / HW mips;
PrefilterEnv on opt paths; mixing PrefilterEnv vs Conv; 128×64 pref of
64×32 env; env recovery without `--env-reg`; treating env darkening as
a bwd bug; background in default env-opt loss; direct UV-sphere cage;
per-phase Android viewers; vkw TRANSFER bit; `add_subdirectory`
VulkanWrapper; mechanical `.cpp`→`.c` / vtable IR; reactive prune on
eager torch engines; data-ptr cache without pin; shared AA jitter seed;
Alg.2 completeness claims; consecutive COMP batching as a current win.

Author feedback that locked the design: image-valued losses; paper
transfer model; stochastic FaceFetch not exact vertex gather;
raster-headed fusion; remat second GLSL-collision guard; PrefilterConv
transpose; fg-only env loss.

## Doc drift

- `CLAUDE.md` never mentions that a C core is **decided** (ENG-42) or
  that implementation starts at ENG-35. It remains the operational spec
  for behavior. This log + Linear briefing cover the C-port traps.
- `docs/algorithm.md` UV Sobol vs camera jitter (stale).
- `docs/benchmarks.md` envmap rows are `--fg-only=0`.
- ~8 week gap Jul 18 → Sep 14 with no work logs.
- `DRESSI_DUMP_STAGES=1` exists; not in CLAUDE.md.
- Zero-copy CUDA interop (Vulkan→CUDA only) designed, never built.
- `IdxCoord` not extended to RasterizeSoft / LookupFaces / WIDE gathers.

## Open (not started this session)

- ENG-35 CPU Add/Mul reverse-mode gate — wait for an explicit ask.
- Alias-unsplit is a **latent C++ bug**; fix it when porting ENG-38,
  do not “fix C++ first” unless asked.
- Remat unit test still missing in C++ (ENG-25 / ENG-37).
