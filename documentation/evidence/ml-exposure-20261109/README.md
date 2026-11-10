# AI/ML exposure evidence (2026-11-09)

Exposes the trained HGB model through application settings and detection service defaults.

## Changes covered
- `DetectionSettingsStore` persists `useRuleEngine`/`useMlModel` via `QSettings` (defaults preserved).
- `DetectionService()` loads stored detection settings; explicit-settings constructor unchanged.
- `SettingsPage` now provides checkboxes to enable rule engine / ML model and saves settings.
- `DetectionPage` shows active detection engine(s) on refresh/run.
- `main.cpp` sets Qt organization/application name for stable settings storage.

## Tests
- `MlExposureTest.DetectionServiceUsesStoredMlSetting` — stored `useMlModel=true` causes `ml_model` findings.
- `MlExposureTest.DefaultDetectionServiceRemainsRuleBased` — defaults still produce rule findings, no `ml_model`.

## Evidence
- `ctest-full.log` — 160/160 passed after exposure/fixes.
- `ctest-ml-exposure-verbose.log` — verbose ML/exposure/config tests.
- `gui-smoke.log` — headless `QT_QPA_PLATFORM=offscreen` startup smoke.

## Note
An intermediate exposure-test approach polluted global `~/.config/FTAD/FTAD.conf` (ML enabled), causing temporary regressions in pipeline defaults; store isolation and config reset fixed this. Evidence reflects final passing state.
