# Architecture

The Community fork retains upstream C++/Qt UI, Rust components, OCR runtime and platform abstractions. Community-specific boundaries are direct PaddleOCR configuration, local privacy defaults, disabled account/paid features, independent update origin and Windows x64 release policy.

The upstream-sync workflow preserves Git ancestry. Production compilation, native acceptance and packaging are handled by Community Windows x64. See [sync](../community-upstream-sync.md) and [build](../community-windows.md).
