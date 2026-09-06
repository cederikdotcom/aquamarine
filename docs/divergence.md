# Aquamarine CPU fork divergence

This is the allocation/output fork used by [Omarchy32 CPU](https://github.com/cederikdotcom/omarchy32cpu) together with [Hyprland's CPU renderer](https://github.com/cederikdotcom/Hyprland/blob/pixman-renderer/docs/divergence.md). Work lives on [`cpu-backend`](https://github.com/cederikdotcom/aquamarine/tree/cpu-backend), not the repository's default `main`. Aquamarine allocates/presents buffers; Hyprland composites pixels; Omarchy configures and packages the desktop. These are separate ownership boundaries.

## Two different measurements

Measured on 2026-09-06 before adding this accounting documentation:

| Measure | Result | Meaning |
|---|---|---|
| Custom patch baseline | upstream v0.15.0, `783bfd9ae441d1d0519b979ac68b73ddd6e81df0` | Exact parent of the first CPU patch |
| Runtime/source snapshot | fork `60a765d3bb01a975bc193f216b5d9a5e9aaca3d7` | Three custom commits beyond the baseline |
| Custom patch delta | **6 files, +335 / −18 lines (353 churn)** | SHM allocator, nested Wayland and CPU DRM integration |
| Current upstream main | `36b66db4ddd708ad19f5db850af6a478d8a19b2a` | Four upstream commits not integrated |
| Direct current-main tree difference | **9 files, +348 / −34 lines** | Custom work plus missing upstream changes |
| Commit-graph distance | **4 upstream-only / 3 fork-side commits** | Not the number of bugs or unresolved decisions |

The README, this document and reporting machinery add separate `fork-accounting` weight. The six-file runtime snapshot excludes those additions. Neither this implementation nor the Hyprland renderer is included in [Omarchy's file totals](https://github.com/cederikdotcom/omarchy32cpu/blob/main/docs/divergence.md).

## Custom patch ownership

| Entry | Source scope | Files / churn at runtime snapshot | Tracking |
|---|---|---:|---|
| CPU SHM allocator | `Allocator.hpp`, `Shm.hpp`, `src/allocator/Shm.cpp` | 3 / 217 | [#1](https://github.com/cederikdotcom/aquamarine/issues/1) |
| Nested SHM Wayland | `src/backend/Wayland.cpp` | 1 / 75 | [#2](https://github.com/cederikdotcom/aquamarine/issues/2) |
| CPU allocation / DRM | `src/backend/Backend.cpp`, `src/backend/drm/DRM.cpp` | 2 / 61 | [#3](https://github.com/cederikdotcom/aquamarine/issues/3) |
| Documentation/accounting | README, this document, `.github/divergence/`, reporting workflow | Measured separately | [#4](https://github.com/cederikdotcom/aquamarine/issues/4) |

The SHM allocator provides memfd/mmap-backed buffers and data-pointer access. The nested backend wraps these in wl_shm buffers, handles hosts without dmabuf, advertises baseline formats and preserves the size from an early configure event. Generic configure/host fixes are candidates for separation from CPU allocator integration.

`AQ_FORCE_ALLOCATOR=shm` selects SHM; `AQ_FORCE_ALLOCATOR=dumb` selects DRM dumb buffers, opening the primary card node rather than a render node. With no usable backend DRM fd, allocation falls back to SHM. DRM skips unnecessary GL renderer initialization for a CPU primary allocator. This supplies scanout without EGL on the Mac; it is not a pixel compositor or a replacement for Hyprland.

## Upstream backlog and convergence order

The four missing upstream commits at measurement time are:

- `ffbf3a9`: do not terminate shared EGL displays in CDRMRenderer destruction.
- `4ea1633`: reserve CRTCs used by redundant tiles.
- `f844ce1`: fix shader version pragma placement.
- `36b66db`: fix libinput missing new events on resume.

The resume fix is particularly relevant to the outstanding suspend/input acceptance work, but its title does not prove it fixes this Mac's intermittent appletouch initialization. Review and test it; do not conflate those symptoms. No upstream merge or target binary replacement was performed by this documentation task.

1. Review the four commits against CPU and ordinary GBM paths, then build a matching dependency set before deploying.
2. Split generic nested-host/configure robustness changes into independent upstream candidates.
3. Validate SHM allocation bounds, format/stride arithmetic, failed allocation cleanup, buffer lifetime/release, configure/frame timing and cursors.
4. Validate primary-node dumb scanout, suspend/resume and the unforced GBM path. Remove the fork only when upstream can supply equivalent behavior.

Historical nested and VM DRM evidence is in [Omarchy's renderer progress](https://github.com/cederikdotcom/omarchy32cpu/blob/main/docs/pixman-renderer/PROGRESS.md); physical Mac evidence and remaining gates are in the [convergence report](https://github.com/cederikdotcom/omarchy32cpu/blob/main/docs/history/macbook-convergence-20260905.md). Old chronological notes saying `60a765d` was not pushed describe that earlier moment: this run verified it is published on `cpu-backend`. Those historical results are not a fresh resume, input or GPU-path test.

## Reproduce and maintain the accounting

The source of truth is [the registry](../.github/divergence/registry.json). Every custom path belongs to exactly one entry. Unknown or overlapping ownership fails the report; unmatched pathspecs are surfaced for review. Each entry has a local GitHub issue with handling, rationale and exit conditions.

```bash
git fetch upstream main
python3 .github/divergence/test_report.py
python3 .github/divergence/snapshot.py
# Before committing, include tracked edits and untracked files:
python3 .github/divergence/snapshot.py --worktree
```

Run these on the documented working branch. The pinned baseline is intentionally not advanced automatically: change it only after integrating and validating upstream, then review every ownership change. The current-main distance is unclassified raw tree distance; it is never silently folded into the classified custom-patch table. Commit counts describe ancestry, not patch equivalence. Binary files count as files but contribute no textual line count.

The [divergence workflow](../.github/workflows/divergence.yml) still runs on work-branch pushes and manual dispatch. The [daily scheduler](https://github.com/cederikdotcom/aquamarine/blob/main/.github/workflows/upstream-sync.yml) runs from default `main` at 06:37 UTC (GitHub can delay it), explicitly checks out the CPU work branch, and runs both the merge queue and divergence accounting. Its work-branch copy is retained for review; edits must also reach default main to affect scheduling.

### One issue per pending merge batch

The daily monitor checks upstream `main`. When commits are missing, it creates one open issue for that channel with exact source/target SHAs, up to 60 incoming commit subjects and a `git merge-tree` conflict check. Subsequent runs extend/update that pending batch instead of creating duplicates. Generated blocks preserve human notes outside the markers. Once the recorded target is an ancestor of the work branch, the batch closes; a subsequent batch gets a new issue. A manually closed issue for the same target stays closed.

The merge issue is published before classification, so accounting failures cannot hide incoming work. Failed issue writes fail the job. The monitor does not merge, build or install anything, and automatic closure proves source ancestry only—not runtime or hardware acceptance. Use comments for validation evidence. The branch SHA must match its configured remote work branch before the monitor can publish.

`python3 .github/divergence/test_monitor.py` tests the queue lifecycle. `python3 .github/divergence/monitor.py` inspects without issue writes; add `--publish` only to update the queue. Configuration is in `monitor.json`. Both scheduler and push workflow refresh the semantic divergence issue blocks separately from merge tasks.

The accounting engine is copied from [Omarchy32 CPU at 4b3278af](https://github.com/cederikdotcom/omarchy32cpu/blob/4b3278af/.github/divergence/report.py), with descriptive text adjusted for this workflow. Keep those copies aligned when fixing accounting behavior. The small local regression suite covers unique ownership, missing/duplicate ownership failures, binary weights and preservation of human issue text.
