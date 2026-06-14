# Security Audit — stereotool-ladspa

**Date:** 2026-06-14
**Scope:** Full source tree of `caveman99/stereotool-ladspa`
(`st_wrapper.cpp`, `stereotool_ladspa.c`, `Makefile`, `.github/workflows/build.yml`).
**Out of scope:** The proprietary `libStereoTool` DSP library (closed-source binary;
behaviour treated as a trusted-but-opaque dependency).

> **Remediation status (2026-06-14):** Findings #2, #3, #4, #5, #6, #7, #9 and #10 have
> been fixed in this branch. #1 has the verification *mechanism* in place but still
> requires the maintainer to pin a SHA-256 (repo variable `STEREOTOOL_ZIP_SHA256`).
> #8 is a documentation/permissions recommendation. SHA-pinning of the GitHub Actions
> themselves (part of #6) was intentionally left to the maintainer to avoid guessing
> commit hashes. See "Remediation applied" at the end.

## Summary

This is a small LADSPA plugin that wraps Thimeo Stereo Tool. The attack surface is
narrow: the code runs in-process inside a LADSPA host, processes audio buffers
supplied by that host, and at instantiation loads a `settings.sts` preset from the
plugin's own install directory. There are no network listeners, no privilege
boundaries crossed at runtime, and no parsing of untrusted wire data in this
codebase itself.

The most material issues are in the **CI/release supply chain** (a third-party binary
`.so` is downloaded and shipped to end users with no integrity verification) and a set
of **memory-safety / robustness defects** (unchecked allocations, an integer-overflow
in a heap allocation, use of uninitialised pointers if a host misbehaves, and C++
exceptions able to unwind across a C ABI boundary).

| # | Severity | Issue | Location |
|---|----------|-------|----------|
| 1 | Medium | Third-party binary downloaded and released without integrity/version pinning | `build.yml:26-29` |
| 2 | Medium | Unchecked `malloc` → NULL-pointer dereference (DoS) | `stereotool_ladspa.c:53`, `:127` |
| 3 | Medium | Integer overflow in heap array allocation | `st_wrapper.cpp:34` |
| 4 | Low/Med | C++ exceptions can unwind across the C ABI boundary (UB) | `st_wrapper.cpp:34`, `:41` |
| 5 | Low | Uninitialised port pointers used if host calls `run()` before `connect_port()` | `stereotool_ladspa.c:53,88` |
| 6 | Low | GitHub Actions not pinned to commit SHAs; no least-privilege `permissions:` | `build.yml` |
| 7 | Low | `unzip ... | true` masks extraction failures | `build.yml:29` |
| 8 | Low | Preset auto-loaded from install dir (untrusted-search-path style exposure) | `stereotool_ladspa.c:57-72` |
| 9 | Info | Real-time allocation in `run()` despite `HARD_RT_CAPABLE` | `st_wrapper.cpp:34,48` |
| 10 | Info | `st_handle == NULL` from `StereoTool_Create` not guarded before processing | `stereotool_ladspa.c:55,88` |

---

## Findings

### 1. Supply chain: third-party binary shipped without integrity verification — Medium
**CWE-494 (Download of Code Without Integrity Check), CWE-1357 (Reliance on Insufficiently Trustworthy Component)**
`.github/workflows/build.yml:26-29`

```yaml
curl -L -o Stereo_Tool_Generic_plugin.zip https://download.thimeo.com/Stereo_Tool_Generic_plugin.zip
unzip Stereo_Tool_Generic_plugin.zip | true
```

The release pipeline downloads the proprietary `libStereoTool*.so` at build time and
bundles it directly into the published GitHub Release archive that users install. The
download:

* is **not pinned to a version** (always "current" — releases are not reproducible and
  can silently change behaviour between builds), and
* has **no checksum or signature verification**.

The transport is HTTPS, so a passive network MITM is mitigated by TLS. The residual
risk is an **upstream/origin compromise or artifact rotation**: whatever bytes
`download.thimeo.com` serves at build time are signed into your release and trusted by
every downstream user. Because the shipped `.so` is loaded into the host process, a
compromised blob is arbitrary code execution on end-user machines, attributed to this
project.

**Recommendation:**
* Pin the expected upstream version and verify a known SHA-256 before use:
  ```bash
  echo "<known-sha256>  Stereo_Tool_Generic_plugin.zip" | sha256sum -c -
  ```
  Fail the build on mismatch. Update the pinned hash deliberately when bumping the
  vendored version.
* Consider documenting the exact upstream version each release was built against.

---

### 2. Unchecked `malloc` → NULL dereference (DoS) — Medium
**CWE-252 (Unchecked Return Value), CWE-476 (NULL Pointer Dereference)**
`stereotool_ladspa.c:53` and `stereotool_ladspa.c:127`

```c
StereoToolHandle *handle = malloc(sizeof(StereoToolHandle));
handle->sample_rate = (int)s_rate;        // <-- deref before NULL check
```

```c
descriptor = (LADSPA_Descriptor *)malloc(sizeof(LADSPA_Descriptor));
descriptor->UniqueID = STEREO_TOOL_UNIQUE_ID;   // <-- same pattern in constructor
```

Neither allocation is checked. Under memory pressure (or RLIMIT exhaustion), this is an
immediate NULL-pointer dereference and process crash. The `init()` constructor case is
worse: it runs at library-load time, so a failure crashes the host as soon as the
plugin is `dlopen`'d.

**Recommendation:** Check both allocations and fail gracefully — return `NULL` from
`instantiate` (LADSPA contract for failure), and in the constructor leave `descriptor`
NULL so `ladspa_descriptor()` reports no plugins rather than crashing.

---

### 3. Integer overflow in heap array allocation — Medium
**CWE-190 (Integer Overflow) → CWE-787 (Out-of-bounds Write)**
`st_wrapper.cpp:34`

```c
float* buffer = new float[count * channels];   // count: unsigned long, channels: 2
...
for (unsigned long i = 0; i < count; ++i) {
    buffer[i * 2 + 0] = inL[i];
    buffer[i * 2 + 1] = inR[i];                // OOB write if allocation under-sized
}
```

`count * channels` is computed in `unsigned long` arithmetic **before** being handed to
`new[]`. C++'s built-in `new[]` overflow guard only checks the final
`elements * sizeof(float)` product, not this earlier multiplication — so if `count`
exceeds `ULONG_MAX/2`, the element count wraps to a small value, the buffer is
under-allocated, and the copy loops write out of bounds (heap corruption).

In practice `count` is a host-supplied audio frame count and is small, so exploitability
is low; but the value is host-controlled and the guard is absent. Defensive code should
not trust it.

**Recommendation:** Validate `count` against a sane maximum block size and/or compute
the size with an explicit overflow check (e.g. `if (count > SIZE_MAX/ (channels*sizeof(float))) ...`)
before allocating. See also findings #4 and #9 — the allocation itself is better avoided.

---

### 4. C++ exceptions unwinding across the C ABI boundary — Low/Medium
**CWE-248 (Uncaught Exception)**
`st_wrapper.cpp:34,41`

`StereoTool_ProcessFloat` is declared `extern "C"` and is called from the C translation
unit (`run()` in `stereotool_ladspa.c`). Inside it, `new[]` can throw `std::bad_alloc`
(or `std::bad_array_new_length` for the overflow case in #3), and `stereoTool_Process`
may also throw. Propagating a C++ exception through a C stack frame is undefined
behaviour (typically `std::terminate`/abort, possibly worse). This converts an OOM
condition into a hard crash with no chance for the host to recover.

**Recommendation:** Wrap the body in `try { ... } catch (...) { /* output silence, return */ }`
so no exception can escape the `extern "C"` function. Combine with the no-allocation
approach (#9).

---

### 5. Uninitialised port pointers if `run()` precedes `connect_port()` — Low
**CWE-457 (Use of Uninitialized Variable)**
`stereotool_ladspa.c:53` (alloc), `:88` (use)

`instantiate` uses `malloc` (not `calloc`), leaving `input_l`, `input_r`, `output_l`,
`output_r` indeterminate. A well-behaved LADSPA host connects every port before calling
`run`, but a buggy or hostile host that calls `run` first will dereference wild pointers
inside `StereoTool_ProcessFloat`. Cheap to harden.

**Recommendation:** Use `calloc` (or zero the struct) so unconnected ports are NULL, and
optionally NULL-check the four buffers and `st_handle` at the top of `run`.

---

### 6. CI: unpinned actions and no least-privilege token — Low
**CWE-829 (Inclusion of Functionality from Untrusted Control Sphere)**
`.github/workflows/build.yml`

* Actions are pinned only to mutable major tags (`actions/checkout@v4`,
  `actions/upload-artifact@v4`, `actions/download-artifact@v4`,
  `softprops/action-gh-release@v2`). A compromised or moved tag executes attacker code
  in a job that holds `GITHUB_TOKEN`. Pin to full commit SHAs.
* No top-level `permissions:` block, so the default token scope applies. The build jobs
  need none, and only the release job needs `contents: write`.

**Recommendation:**
```yaml
permissions:
  contents: read        # top level
# release job:
  permissions:
    contents: write
```
and pin third-party (and ideally first-party) actions to `@<sha>`.

---

### 7. CI: `unzip ... | true` masks extraction failures — Low
`.github/workflows/build.yml:29`

```yaml
unzip Stereo_Tool_Generic_plugin.zip | true
```

`| true` pipes `unzip`'s stdout into `true` and the step's exit status becomes `true`'s
(always success) — extraction errors (corrupt/empty download, partial archive) are
swallowed. This is almost certainly a typo for `|| true`, but even `|| true` is the
wrong intent here: a failed download/extract should fail the release, not proceed with
missing files. Pair this with the checksum check from #1.

---

### 8. Preset auto-loaded from the plugin's install directory — Low
**CWE-426 (Untrusted Search Path) — conditional**
`stereotool_ladspa.c:57-72`, README note

At instantiation the plugin derives its own directory from `dladdr()` and auto-loads
`<dir>/settings.sts` into Stereo Tool. The path is taken from the loaded library's path
(not CWD or an env var), which is the safe choice. Residual risk: if the install
directory is writable by a lower-privileged user (e.g. a shared/system LADSPA path with
loose permissions), an attacker can drop a `settings.sts` whose contents are then parsed
by the closed-source DSP library — surfacing any parser bugs there. The `.sts` parser is
out of scope, so this is flagged as exposure, not a confirmed vulnerability.

**Recommendation:** Document that the plugin directory must not be group/other-writable;
optionally `stat()` the preset and refuse to load if it is writable by anyone but the
owner. The truncation handling here (`strncpy` + explicit NUL, bounded `snprintf`) is
correct — no path-handling bug found.

---

### 9. Real-time allocation in `run()` despite `HARD_RT_CAPABLE` — Info / robustness
`st_wrapper.cpp:34,48`; descriptor flag at `stereotool_ladspa.c:130`

The descriptor advertises `LADSPA_PROPERTY_HARD_RT_CAPABLE`, which promises bounded,
allocation-free, lock-free processing. `StereoTool_ProcessFloat` calls `new[]`/`delete[]`
on every `run()`. Beyond the RT-correctness violation (page faults / heap lock under the
audio thread → xruns), per-call allocation is the source of findings #3 and #4.

**Recommendation:** Allocate the interleave buffer once per instance (sized to a max
block in `instantiate`, grown rarely outside the RT path) and reuse it in `run`, or
process without interleaving if the DSP API allows planar buffers. This removes the RT
violation and the throw/overflow surface at once.

---

### 10. `StereoTool_Create` NULL result not guarded before processing — Info
`stereotool_ladspa.c:55,88`; `st_wrapper.cpp:14`

`instantiate` stores `st_handle` and proceeds even when `StereoTool_Create` returns NULL
(the preset load is guarded, but `instantiate` still returns a usable handle). A later
`run` then forwards NULL into `stereoTool_Process`, with crash behaviour determined by
the closed library. Note also that `StereoTool_Create(int sampleRate)` ignores its
`sampleRate` argument and passes `nullptr` to `stereoTool_Create` — harmless but
surprising.

**Recommendation:** If `st_handle` is NULL, fail instantiation (return NULL) rather than
return a half-initialised instance.

---

## What was checked and found clean

* **Path handling** in `instantiate` — `strncpy` is followed by an explicit NUL
  terminator and `snprintf` is correctly bounded; no overflow or truncation-injection.
* **Format strings** — all `fprintf` calls use fixed literal formats; no format-string
  injection.
* **Thread safety** — the global `descriptor` is written only by the load-time
  constructor and read-only thereafter; per-instance state is isolated. No shared mutable
  global on the processing path.
* **`connect_port`** — bounds-safe (switch over a fixed enum; out-of-range ports ignored).
* **`cleanup`/`fini`** — no double-free or use-after-free observed in this code.

## Suggested remediation priority

1. Add checksum verification + version pinning to the release download (#1) and fail on
   extract errors (#7).
2. Check allocations (#2) and eliminate the per-`run` allocation, which closes #3, #4,
   and #9 together.
3. Harden the host-contract assumptions: `calloc` + NULL guards (#5, #10).
4. Lock down CI (#6) and document install-directory permissions (#8).

---

## Remediation applied

The following fixes were committed to the `audit` branch alongside this report:

* **#2** — `instantiate` now uses `calloc` and returns `NULL` on allocation failure;
  `init()` returns early (leaving `descriptor` NULL) if its `malloc` fails.
* **#3 / #9** — The per-`run()` `new[]`/`delete[]` is gone. The interleave buffer now
  lives in the instance (`scratch`/`scratch_frames`) and is grown via `realloc` only when
  the block size increases, with an explicit overflow check before the
  `sample_count * 2 * sizeof(float)` multiply. Steady-state `run()` is allocation-free.
* **#4** — `StereoTool_ProcessFloat` body is wrapped in `try { … } catch (...)`, emitting
  silence so no C++ exception can cross the `extern "C"` boundary.
* **#5 / #10** — `calloc` zero-inits port pointers; `run()` returns early if any port is
  unconnected; `instantiate` fails if `StereoTool_Create` returns NULL.
* **#6** — Top-level `permissions: contents: read` added; the `release` job opts into
  `contents: write`. All GitHub Actions are pinned to the commit SHA of their latest
  major release (with the version in a trailing comment):
  `actions/checkout` v6.0.3, `actions/upload-artifact` v7.0.1,
  `actions/download-artifact` v8.0.1, `softprops/action-gh-release` v3.0.0.
* **#7** — `unzip … | true` replaced with `set -euo pipefail`, `curl -fL --retry 3`, and
  a failing `unzip -o`.
* **#1** — Integrity-check step added: verifies the download against the
  `STEREOTOOL_ZIP_SHA256` repo variable when set, otherwise prints the observed hash and
  a warning. **Action required:** pin the hash to complete this fix (the audit
  environment's egress allowlist blocks `download.thimeo.com`, so the hash must be
  pinned from a host that can reach it).

Both translation units were syntax-checked (`-Wall -Wextra`, clean apart from
pre-existing unused-parameter warnings) and the workflow YAML was validated.
