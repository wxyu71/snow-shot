# Snow Shot Community documentation

This is the documentation entry point, **not** a second release runbook.
Start with the root README for the project's user-facing scope.

The navigation separates task-oriented guides, references, explanations and
release evidence, borrowing from [Diátaxis](https://diataxis.fr/). These are
project-level conventions, not an official OpenAI or Anthropic folder mandate.

## Guides: build, use and contribute

- [Windows x64 beta build, OCR setup and acceptance](community-windows.md)
- [macOS build, packaging and installation](platforms/macos-build.md)
- [Upstream sync and conflict review](community-upstream-sync.md)
- [Developer testing: real behavior and tools](development/testing.md)

## Reference and policy

- [Privacy, network and stored credentials](community-privacy.md)
- [Release notes template used by the publisher](community-release-notes.md)
- [Community changelog](community-changelog.md)
- [OCR worker protocol](../snow-crates/crates/snow-ocr-process/PROTOCOL.md)
- [Contributor and agent instructions](../AGENTS.md)

## Architecture and design decisions

- [Modules, ownership and trust boundaries](architecture/overview.md)
- [Community CI architecture](community-ci-architecture.md)
- [Upstream merge architecture review](community-upstream-architecture-review.md)

## Release evidence

- [Beta1 release checklist](releases/beta1-checklist.md)
- [Windows installer and upgrade runbook](community-windows.md)

Keep code-specific tests in their owning module, CMake logic in cmake/,
release automation in scripts/, and human-maintained guides under docs/.
Historical docs/community-* filenames remain stable for release scripts and
external links. New material should follow this navigation rather than
creating unrelated top-level Markdown files.
