# Submission review format

Write a readable Markdown report at `submissions/<student-name>/submission-review.md`. Use the sections below, adapting their length to the evidence. No empty template fields. Use relative links to student source, report, and `review-evidence/`; include source line numbers in text. Verify HTML anchors actually exist (normally `report.html#scenario-G01`).

## Title and assessment

Title: `# MemTacToe submission review: <student-name>`.

Open with a specific assessment of game correctness and assignment compliance: consequential confirmed defects, genuine versus harness-related observations, and material uncertainties. No unsupported blanket correctness verdict or numerical grade.

## Evidence and execution context

Record date, host OS/architecture, report path/timestamp/platform, source/binary paths and hashes, launch commands/cwd, input conventions, roles/marks, creator, topology, known build flags, and report/current-binary provenance limits. Identify portability analysis consulted and available x86 execution. Distinguish supplied binaries from diagnostic builds.

## Assignment coverage

| ID | Requirement and assignment locator | Conclusion | Evidence and explanation | Rubric relevance / findings |
|---|---|---|---|---|

Cover every normative assignment requirement, including those outside the rubric. Use A IDs and `Met`, `Partially met`, `Not met`, or `Not established`. Split independent behaviors or explain mixed conclusions. Each row needs specific report, experiment, or source evidence. Map violations to findings. Use `Assignment requirement; no separate rubric item` when no category mapping is defensible.

## Rubric assessment

| Category | Weight / alternative | Assessment and evidence | Finding IDs |
|---|---|---|---|

Include R1 error handling (2), R2 printing results (2), and R3 process implementation (5 for fork parent/child OR 6 for separate processes). Explain actual architecture, the stated fork-route 9/10 ceiling when relevant, and different-programs ambiguities. Process alternatives are not additive. Do not award points/deductions without a separate grading request. Explain mappings rather than automatically placing every game defect in a rubric category.

## Findings

Order by impact with `F01`-style IDs. Each finding includes:

- **Title, impact, classification:** confirmed submission defect, harness/profile incompatibility, environment limitation, or unresolved; mark source-only risks explicitly.
- **Assignment/rubric:** affected A IDs and explicit **R1 — Error handling**, **R2 — Printing results**, and/or **R3 — Process implementation** labels when applicable. Otherwise mark assignment-only or additional issue. Explain when an additional robustness concern informs a broad rubric category without violating an explicit requirement.
- **Reproduction:** actual trigger/setup/inputs, E IDs, report cases, expected versus observed behavior, and practical consequence.
- **Logical cause:** trigger → implementation behavior → consequence, with source paths/lines and relevant trace evidence. Cause confidence `Confirmed`, `Likely`, or `Unresolved`; corrected code is unnecessary.
- **Platform assessment:** ARM observation and platform-independent, architecture-sensitive non-portability, environment/dependency limitation, or unresolved classification. For sensitive issues give x86 support marked `Observed`, `Inferred`, or `Unknown`; identify native/emulated execution or diagnostic flags.

Group shared-cause symptoms without hiding distinct mechanisms. If no confirmed defects are found, state that within the tested scope and still document anomalies and limitations.

## Harness reconciliation and scenario ledger

| Report scenario | Evidence / scope | Reviewer conclusion | Independent probe or blocker | Requirements / findings |
|---|---|---|---|---|

Account for every report scenario, including normal completions. Grouped rows may be compact only if individual case IDs/conclusions remain identifiable. Every error indication needs an independent probe or blocker; distinguish defects from profile/input/pacing/capture/cleanup issues. Note unexecuted actions and missing coverage. Completion alone is not correctness evidence.

## Independent experiments

| ID | Purpose and replay instructions | Expected versus observed | Result, limits and evidence |
|---|---|---|---|

List E IDs and link scripts/logs. Include successful controls and recovery checks. Identify natural termination, reviewer intervention, IPC leftovers, diagnostic-build differences, and timing that affects interpretation.

## Portability and remaining uncertainty

Consolidate ARM/x86 conclusions, qualifying prior portability claims and property-isolation tests. State whether each sensitive failure persists on x86 and the evidence strength. Identify untested requirements, failed reproduction attempts, unavailable tooling, attribution/provenance limits, and other uncertainties. Keep additional bugs and non-rubric specification violations visible rather than burying them in generic limitations.
