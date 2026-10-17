# Review: camera framing

## Must fix
- Visual placement in the captured frame still disagrees with projected model bounds; do not treat the numeric NDC bounds as proof of correct visible framing.
- Camera obstruction from `lamp_wooden_lantern` retracts the arm to 3.300 m. The prop fade path is implemented, but the capture indicates the framing issue persists.

## Recommended
- Trace the glTF node/root transform and the precise actor rendered in the capture before further offset tuning.

## Result
- Build, CTest, and 1080p smoke pass. Visual acceptance remains unresolved.
