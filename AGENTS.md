# 32blit-sdk: agent instructions

The chili-chip fork of the 32blit SDK, with the VGC Zero (RP2350) port. See [README.md](README.md) and [docs/](docs/).

These rules apply to AI coding agents (Claude Code, Codex, Copilot and others) and to people.

## Test-driven development (required)

Write the tests before the code they test. This applies to every change in behaviour: new features, bug fixes, and refactors (there the tests prove nothing changed).

1. **Red.** Write the tests first: unit tests for the logic, integration tests where the change crosses a boundary (HTTP API, database, payment provider, file format, engine/host interface), and an end-to-end test for a user-visible flow when this repo has an E2E harness. Run them and watch them fail for the reason you expect.
2. **Green.** Write the smallest change that makes them pass.
3. **Refactor.** Clean up with everything green, then run the whole suite before you push.

- A bug fix starts with a test that reproduces the bug.
- When a kind of test is not possible here yet (no harness for that layer, or behaviour that only shows on the console), say so in the PR description and list the manual steps you ran instead.
- Never skip, disable or weaken an existing test to get to green.
- Name the tests you wrote first in the PR description.
- **Code coverage: we aim for 80% of lines.** It is a target, not a gate: a pull request below 80% is not refused. Do not lower coverage without a reason in the PR.
- This repo has no coverage report yet. The change that adds the first tests also adds an informational coverage step to CI, like chili-platform's `coverage` job.

## Tests in this repo

- CI builds the SDK and `32blit-examples` for Linux/SDL, Windows, macOS, Emscripten and the pico targets. A change must keep every build green.
- There is no host unit test harness yet. For logic that does not need hardware (maths, text, file formats, asset tools), add a host-only test first (CTest, or `pytest` for tools in `tools/`) with a CI step that runs it.
- On-device checks are `utilities/system-test` and `utilities/hardware-test`. Behaviour that only shows on the VGC Zero needs a hardware test on the console; say so in the PR.
