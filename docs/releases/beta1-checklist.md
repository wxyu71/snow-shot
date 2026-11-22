# Snow Shot Community v1.5.8-beta.1 acceptance

**Status: BLOCKED.** This is a release gate, not a statement that the
candidate is approved or that a Beta1 release exists.

## Required evidence for the final reviewed PR head

- [ ] Exact SHA, source ancestry, changes and protected Community paths reviewed.
- [ ] actionlint, zizmor, PSScriptAnalyzer, Lychee and Git lifecycle tests pass.
- [ ] Compiled C++ Qt/CTest and Rust OCR protocol/process tests pass.
- [ ] Full Windows x64 MSVC + audited static Qt production build passes.
- [ ] Local OCR real-model behavior and direct cloud OCR responses, failures,
      timeouts and credentials/privacy checks pass.
- [ ] Online/offline NSIS packages, manifests and SHA-256 validate.
- [ ] Real installed upgrade from official 1.5.7 preserves user data,
      shortcuts, local OCR and privacy choices.
- [ ] Fresh install, offline start, uninstall and native Windows UI smoke pass.
- [ ] Third-party notices, release notes, product identity and security
      warnings are reviewed and signed off.
- [ ] No unresolved P0/P1 regressions or unexplained warnings.

## Evidence ledger

| Check | Commit / workflow run / artifact | Result |
| --- | --- | --- |
| Final PR #2 head | Pending | Unverified |
| Security/preflight + compiled native | Pending | Unverified |
| Full Windows x64 build | Pending | Unverified |
| NSIS installed upgrade + offline/online | Pending | Unverified |
| Actual OCR and privacy acceptance | Pending | Unverified |
| Human product approval | Pending | Unverified |

Keep PR #2 as Draft until these checks pass. No merge or automated
prerelease publication is authorized by a documentation-only pass.
Publish Community Beta1 only from current reviewed `main`, with the
Community identity, independently managed Windows updates and no
upstream commercial/account service.

macOS Sparkle, Homebrew and Windows ARM64 are separate acceptance
projects; do not advertise their release packages as covered by the
Windows x64 Beta1 gate.

See [the Windows runbook](../community-windows.md),
[CI design](../community-ci-architecture.md) and
[architecture overview](../architecture/overview.md).
