# PuzzlesAndGames

Offline-first, cross-platform **puzzles and single-player games**. No ads, no telemetry, **no network** in shipped builds. Built for users who want a small, auditable native app and developers who want a **C/C++** codebase with **bundled** dependencies.

## Docs

| File | Purpose |
|------|---------|
| [PRD.md](PRD.md) | Product vision, requirements, personas, roadmap |
| [PLAN.md](PLAN.md) | Engineering stack, architecture, CI guardrails, checklist |

## Principles

- **Trust**: ship what you can explain; minimize dependencies; document licenses.
- **Portable**: Windows, Linux, macOS as first-class targets.
- **Reuse**: integrate permissively licensed upstreams (for example Simon Tatham-style MIT C puzzle cores) with clear `third_party/` attribution.

## Status

Greenfield: no game code in this folder yet. Follow [PLAN.md](PLAN.md) for the first vertical slice.

## License

TBD (intent: permissive for the hub; see PRD for GPL boundaries if bundling strong chess engines).
