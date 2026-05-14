**Purpose:** Each frame, reads the previous render target, shifts its contents by the character's movement delta, applies an edge fade to prevent artifacts at RT boundaries, and fades all content slightly toward black. This is one half of the ping pong system that keeps footprints anchored in world space as the character moves.

**Material Settings:**

```
Domain     → Surface
Blend Mode → Opaque
Shading    → Unlit
```

---

**Section 1 , Edge Fade Mask**

Generates a mask that is 1.0 at the center of the render target and fades to 0.0 near the edges. This prevents footprints from reaching the RT boundary and creating sharp cutoffs or tiling artifacts.

`TexCoord[0]` is centered by subtracting 0.5, then made absolute so all values are positive (0 at center, 0.5 at edges). The larger of the X and Y components is taken via Max, giving the distance from center along the dominant axis.

A threshold of 0.4 defines the safe zone. Anything within 0.4 UV units of center has full opacity. Beyond 0.4, a 0.1 unit wide fade zone (controlled by the Divide B = 0.1) rapidly brings the mask to 0. The result is inverted so the center reads 1.0 and edges read 0.0.

This mask is held and multiplied with the RT sample in Section 2.
![[Pasted image 20260504180902.png]]

---

**Section 2 , Scrolling Previous RT Sample**

Reads from the previous render target with a UV offset that compensates for the character's movement this frame, making footprints appear to stay fixed in world space.

`ScrollOffset_X` and `ScrollOffset_Y` (MPC) are set each frame in Blueprint from the pixel-snapped movement delta. Adding this offset to `TexCoord[0]` shifts where the RT is sampled from. The result is that existing footprint content appears to move in the opposite direction of the character, giving the illusion of world-anchored persistence.

The `PreviousRT` texture parameter is set at runtime via a Dynamic Material Instance in Blueprint, alternating between RT_A and RT_B each frame as part of the ping pong.

The sampled RGBA result is multiplied by the edge fade mask from Section 1.
![[Pasted image 20260504180910.png]]

---

**Section 3 , Fade Output**

Applies a subtle global fade to all RT content each frame, so footprints gradually disappear over time.

The edge-masked RT sample is multiplied by `FadeSpeed` (a material parameter, default 0.0) and subtracted from itself. The result is clamped via Saturate to prevent negative values. As FadeSpeed increases, content dims faster each frame.

`FadeSpeed` is intentionally a material parameter rather than an MPC value, since it is a fixed global setting that does not need to change per frame.

The output is written to Emissive Color, which the ping pong draws into the current RT each frame.
![[Pasted image 20260504180916.png]]