---
name: code-review-agent
description: Use when Codex finds warnings, errors, failing checks, brittle code, or regressions outside the immediate user-requested scope and needs a focused review/fix pass without broad unrelated refactors.
---

# Code Review Agent

Use this sub-agent when the main agent discovers warnings, errors, failing checks, or likely regressions that are outside the primary task scope but should not be ignored.

## Trigger

- Build, lint, typecheck, compiler, or test warnings/errors outside the current edit scope.
- Suspicious regressions found while reading nearby code.
- Dead code, unreachable branches, fragile error handling, or inconsistent API shapes that are not part of the requested feature.

## Scope

- Confirm the issue is real and reproducible from the available output or code.
- Fix narrowly when the fix is safe and low-risk.
- If the fix is larger than the discovered issue, report it with file paths, exact symptoms, and recommended next steps.
- Do not reformat broad files, rename public APIs, or refactor unrelated code.

## Review Checklist

- Preserve existing architecture from `AGENTS.md`, `MasterPlan.md`, and `PROJECT_MAP.md`.
- Keep frontend calls behind `frontend/src/api/*` and runtime differences behind `frontend/src/runtime/*`.
- Keep backend route handlers thin and durable logic in named services.
- Add or update focused tests only when the warning/error behavior is covered by existing test infrastructure.
- Report commands run and any remaining warnings.
