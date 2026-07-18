---
name: enforce-pixeltagger-mvvm
description: Enforce PixelTagger's project-specific strict MVVM architecture while reviewing, planning, implementing, refactoring, testing, or documenting C++17/Qt Widgets code. Use for changes involving PixelTagger App, Common, Model, ViewModel, View, Repository, Processor, Exporter, signal-slot wiring, ownership, shared presentation contracts, OpenCV processing, JSON persistence, YOLO export, or architecture-boundary audits. Also use when a request mentions the PixelTagger MVVM guideline, asks whether a change conforms to the framework, or limits work to particular layers such as Model/ViewModel.
---

# Enforce PixelTagger MVVM

Treat PixelTagger's MVVM rules as executable engineering constraints, not merely folder naming conventions.

## Establish the source of truth

1. Locate the repository root and read `PixelTagger_MVVM_Architecture_Guideline.local.md` completely when it exists.
2. Read the files directly involved in the request plus composition and build files that determine actual ownership and dependencies.
3. Read `references/mvvm-architecture-guideline.md` completely when the repository guideline is absent. It is the bundled team copy of the project's architecture rules.
4. Prefer the repository guideline when it differs from the bundled baseline because the project document may have evolved.
5. Follow an explicit user scope restriction such as "only Model/ViewModel". Do not fill in adjacent App or View integration unless requested.

## Classify the change before editing

Assign every new responsibility to exactly one primary layer:

- App: object construction, lifetime, dependency injection, signal-slot binding.
- Common: stable data contracts genuinely shared across layers.
- Model: authoritative business state, identities, invariants, and domain relations.
- ViewModel: UI-facing state, business operations, orchestration, presentation conversion, and notifications.
- View: widgets, dialogs, input, drawing, transient interaction state, and coordinate mapping.
- Repository: project persistence and serialization.
- Processor: OpenCV algorithms only.
- Exporter: external dataset/output formatting only.

Before adding a file or abstraction, answer:

1. Who owns it?
2. Which layer may depend on it?
3. Is it authoritative state, presentation state, or transient View state?
4. Does an existing layer already own this responsibility?
5. Would the change create a second source of truth or a reverse dependency?

Do not add `Service`, generalized `utils`, `FunctionModel`, or `Command` merely to make the tree look layered. Add a standalone Command module only when undo/redo is an actual requirement and restrict it to undoable business mutations.

## Plan an end-to-end flow

Describe the requested flow before implementation:

```text
View user intent
-> ViewModel operation
-> Model / Repository / Processor / Exporter
-> ViewModel presentation data and signal
-> View rendering
```

Name the owner of each state transition and identify the Common contract crossing a layer boundary. Keep file dialogs and message boxes in View. Keep algorithms and serialization out of ViewModel.

When multiple ViewModels share data, let them observe the one `ProjectModel` and synchronize through explicit signals. Do not make ViewModels hold pointers to one another.

## Implement within strict boundaries

- Keep `ProjectModel` as the single source of truth for project, image, label, and annotation business data.
- Store annotation geometry in original image coordinates and categories by stable `LabelId`.
- Keep selected IDs, tool mode, availability flags, titles, and processing parameters in ViewModel when they have UI semantics.
- Keep mouse positions, drag previews, zoom, pan, hover handles, painting, and widget/image mapping in View.
- Pass stable IDs and presentation contracts across boundaries instead of exposing Model objects.
- Let View emit user-intent signals. Bind those signals to ViewModel operations in `Application`.
- Let ViewModel publish dedicated change/result/error signals and expose getters or presentation values for View refresh.
- Return structured results from Repository, Processor, and Exporter; never let those modules open dialogs or control widgets.
- Use references for required borrowed dependencies and make ownership explicit in `Application`.
- Preserve unrelated user changes and avoid broad renames unless the requested change requires them.

## Review Common rigorously

Place a type in Common only when at least two layers need a stable contract and the type has no business behavior or ownership. Prefer neutral names such as `AnnotationRenderItem` and `LabelPresentationData`; do not name a shared contract after a single consumer when several layers use it.

Reject Common additions that:

- hold Model, ViewModel, or View instances;
- store current selections or other live application state;
- execute JSON, OpenCV, export, or business rules;
- exist only to avoid choosing the correct owning layer.

## Verify before claiming completion

Run checks proportionate to the change:

1. Inspect `git diff` and confirm only intended files changed.
2. Search View headers and sources for direct includes or references to Model, Repository, Processor, Exporter, and concrete ViewModel types.
3. Search Model/Common for Qt Widget or View dependencies.
4. Confirm `Application` contains composition and binding, not business mutation.
5. Confirm there is one authoritative copy of each business datum.
6. Build the active CMake preset and run relevant tests.
7. For behavioral changes, state a short manual test path covering success, failure, and state synchronization.

Treat a successful build as necessary but insufficient: explicitly report any boundary issue that remains. If verification cannot run, state the exact blocker and do not claim success.

## Report decisions

Lead with whether the result conforms to the project architecture. Summarize:

- responsibility placement;
- dependency and ownership changes;
- signals/contracts added or changed;
- build/test evidence;
- integration work intentionally left to another layer or teammate.
