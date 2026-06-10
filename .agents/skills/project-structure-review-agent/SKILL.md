---
name: project-structure-review-agent
description: Use when Codex finds any single source file over 1000 lines or a file whose responsibilities have grown too broad and needs a focused structure review or split plan aligned with the project architecture.
---

# Project Structure Review Agent

Use this sub-agent when the main agent discovers a single source file over 1000 lines, or a source file that is trending toward multiple durable responsibilities.

## Trigger

- Any `.ts`, `.tsx`, `.js`, `.jsx`, `.cpp`, `.cc`, `.cxx`, `.h`, `.hpp`, or `.cs` source file is over 1000 lines.
- A file mixes UI, API calls, state ownership, processing logic, route handling, and storage responsibilities.
- Backend `main.cpp` or `server/ApiServer.cpp` starts accumulating durable business logic instead of composition or thin routing.

## Review Checklist

- Start from `PROJECT_MAP.md`, `MasterPlan.md`, and `AGENTS.md`.
- Identify the file's current responsibilities and natural ownership boundaries.
- For frontend splits, keep route pages under `features`, shared UI under `shared`, typed clients under `api`, and runtime differences under `runtime`.
- For backend splits, keep durable responsibilities in named `.h`/`.cpp` classes under `server`, `security`, `jobs`, `logging`, `storage`, `image`, `video`, `vision`, or `common`.
- Update imports, exports, CMake sources, and `PROJECT_MAP.md` when a split changes ownership or new files are added.

## Output

- If the split is small and safe, perform it and verify builds/checks where practical.
- If the split is risky, provide a staged decomposition plan with target files and responsibilities.
- Do not split generated files, lockfiles, build outputs, or documentation solely because they exceed 1000 lines.
