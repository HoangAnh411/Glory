# Review: neutral color

## Must fix
- None found in code review; glTF linear factors now remain unchanged, and the added regression fixture catches reintroducing an sRGB transfer.

## Recommended
- Compare the Neutral and Sunset presets in a fixed camera screenshot; atmospheric look is subjective and cannot be graded by smoke output.

## Suggestions
- Regenerate the environment cubemap per preset so indirect reflections match the selected sky.

## Result
- Build, 37 CTest cases, and 1080p smoke pass. No gameplay, hitbox, or combo code changed.
