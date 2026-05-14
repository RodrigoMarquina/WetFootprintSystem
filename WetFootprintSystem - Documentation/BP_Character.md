# Begin Play
**Purpose:** Initializes all systems before gameplay begins. Clears the render targets, caches references, sets MPC parameters to their starting values, and creates the Dynamic Material Instance used for the ping pong scroll.
![[Pasted image 20260504182637.png]]
![[Pasted image 20260504182642.png]]

---
### **Clearing the Render Targets**
Both render targets (RT_FootprintBuffer_A and RT_FootprintBuffer_B) are cleared to black at the start of play. This ensures no leftover data from a previous session appears on the landscape when the game begins. Both must be cleared because the ping pong system alternates between them and either could contain stale data.

---
### **Caching Character Location**
`Get Actor Location` is called once and stored in two variables, `CharacterLocation` and `LastCharacterPosition`. Both are initialized to the same spawn position. `LastCharacterPosition` is used in Event Tick to calculate the movement delta each frame — initializing it at Begin Play prevents a large false delta on the first tick.

---
### **Caching the Landscape Reference**
`Get Actor Of Class` finds the landscape proxy in the level and stores it as the `Landscape` variable. This reference is used every footstep to call the C++ layer weight function. Caching it here avoids searching the level every step.

---
### **Initializing MPC Parameters**
`LocalRTSize` is set on `MPC_FootprintData` from the character's `LocalRTSize` variable. This ensures the landscape material and the Blueprint UV calculations use the same value. Setting it here rather than relying on the MPC default prevents mismatches if the value is tuned on the character instance.

---
### **Creating the Scroll DMI**
A Dynamic Material Instance is created from `M_FootprintUpdate` and stored as `ScrollDMI`. A DMI is required because `M_FootprintUpdate` needs its `PreviousRT` texture parameter changed every frame at runtime. Static material assets cannot have parameters changed at runtime, so a DMI is created once here and reused every tick.

---
### **Setting Initial CharacterPos in MPC**
`Get Actor Location` is called again and its X and Y components are set on `MPC_FootprintData` as `CharacterPos_X` and `CharacterPos_Y`. The landscape material uses these values to calculate the world to RT UV mapping. Setting them at Begin Play ensures the material is correctly aligned from the first frame before Event Tick begins updating them.

# Event Tick

**Purpose:** Runs every frame. Handles time-based wetness decay, calculates the pixel-snapped scroll offset for the render target, executes the ping pong RT update, and keeps the landscape material's character position in sync.

---

**Section 1 , Wetness and Character Location Update**

At the start of each tick, `WetnessLevel` is decremented by a small time-based amount (`DryingRate × 0.05 × DeltaSeconds`) and clamped to 0.0 minimum. This produces a gradual time-based drying effect even when the character is standing still, independent of the per-step drying that happens in HandleFootstep. The 0.05 multiplier keeps tick-based drying slower than step-based drying for a natural feel.

`CharacterLocation` is updated with the current actor position. This variable is used in Section 2 to calculate the movement delta.
![[Pasted image 20260504182842.png]]

---

**Section 2 , Scroll Offset Calculation**

Calculates how far the character moved this frame and converts it into a pixel-snapped UV offset for the render target scroll.

`CharacterLocation` minus `LastCharacterPosition` gives the raw world-space delta for this frame. This is divided by `LocalRTSize` to convert from world units to UV space.

The result is then pixel-snapped using this sequence: multiply by `RTSize` (converts UV to pixel count), Round (snaps to nearest whole pixel), divide by `RTSize` (converts back to UV). This ensures the scroll always moves by exact pixel increments, preventing bilinear filtering blur that would accumulate over many frames.

The snapped values are set directly on `MPC_FootprintData` as `ScrollOffset_X` and `ScrollOffset_Y`. These are read by `M_FootprintUpdate` in the same frame to shift the RT contents.
![[Pasted image 20260504182848.png]]

---

**Section 3 , RT Ping Pong**

Executes the render target update for this frame.

First, `ScrollDMI`'s `PreviousRT` texture parameter is set to the current `PreviousRT` variable. This tells the scroll material which RT to read from this frame.

`Draw Material to Render Target` then executes `ScrollDMI` into `CurrentRT`. The material reads from `PreviousRT`, shifts the content by the scroll offset, applies the edge fade, and writes the result into `CurrentRT`. This is the core of the ping pong, where the previous frame's result becomes the input for the current frame's output.

After drawing, the RT references are swapped using a temporary variable. `CurrentRT` becomes `PreviousRT` and vice versa, so next frame reads from what was just written.
![[Pasted image 20260504182853.png]]

---

**Section 4 , Character Position MPC Update**

`LastCharacterPosition` is updated to the current actor location, ready for next frame's delta calculation. This must happen after the scroll offset has been calculated in Section 2, not before.

The current world position X and Y are then set on `MPC_FootprintData` as `CharacterPos_X` and `CharacterPos_Y`. The landscape material uses these values every pixel to compute the world-to-RT UV mapping, ensuring the RT is always sampled relative to the character's current position.
![[Pasted image 20260504182857.png]]

# Even HandleFootstep

**Purpose:** The core per-footstep logic. Fires on every foot plant via the Animation Notify system. Reads surface data, updates wetness state, spawns particle effects, and stamps the footprint into the render target.

---

**Section 1 , Hit Data Extraction**

Receives two inputs from the Blueprint Interface: `IsLeftFoot` (boolean) and `HitResult` (struct). These are passed from the Animation Blueprint via `Try Get Pawn Owner` when the foot plant Animation Notify fires.

The Hit Result is broken apart to extract the three values used throughout the rest of the function: `HitLocation` (world position of the foot contact), `HitNormal` (surface direction at contact), and the Physical Material which is immediately converted to a `SurfaceType` enum via `Get Surface Type`.

The Surface Type is converted to a name and used as the Row Name for an immediate Data Table lookup on `DT_FootprintData`. The resulting row struct flows into Section 2.
![[Pasted image 20260504183823.png]]

---

**Section 2 , Footprint Data and Landscape Query**

Two operations run in parallel from the Data Table result.

The `DryNormalStrength` and `WetnessNormalStrength` values from the row are immediately pushed into `MPC_FootprintData`. The landscape material reads these values to know how strongly to perturb the surface normal for this surface type. Setting them here means the normal response changes as soon as the foot lands on a new material.

Simultaneously, `GetLandscapeLayerWeightAtLocation` is called using the cached `Landscape` reference and `HitLocation`. This returns the exact wetness layer paint weight at the foot's world position, stored as `GroundWetness`.
![[Pasted image 20260504183830.png]]

---

**Section 3 , Wetness Branch Logic**

Updates `WetnessLevel` based on the relationship between ground wetness and the character's current wetness state.

If `GroundWetness > WetnessLevel`, the feet are stepping onto a wetter surface than they currently are. `WetnessLevel` is set directly to `GroundWetness` — feet get wet instantly when contacting a wetter surface.

If `GroundWetness <= WetnessLevel`, the surface is drier than the current wetness state. `WetnessLevel` lerps toward `GroundWetness` using `DryingRate` as the alpha. This creates a gradual drying curve rather than an instant drop, giving the feeling of feet slowly drying out as they walk on drier terrain.

A second branch checks if `GroundWetness < 0.01` to determine whether to spawn a wet or dry Niagara effect.
![[Pasted image 20260504183836.png]]

---

**Section 4 , Wet Niagara Spawn**

Fires when `GroundWetness >= 0.01` (stepping on a wet area).

The wet Niagara system from the Data Table row is spawned at `HitLocation`, oriented upward from the surface using `Make Rot from Z` on the `HitNormal`. Scale is set to (2.0, 2.0, 2.0) with auto destroy enabled.

After spawning, `GroundWetness` is passed into the Niagara system as a User Parameter named `WetnessIntensity`. Artists can use this value inside the Niagara asset to scale particle count, size, or velocity, making the wet effect visually stronger on fully saturated surfaces than on barely damp ones.
![[Pasted image 20260504183840.png]]

---

**Section 5 , Dry Niagara Spawn**

Fires when `GroundWetness < 0.01` (stepping on a dry surface).

The dry Niagara system from the Data Table row is spawned identically to Section 4 but without a wetness intensity parameter, since dry effects are fixed in intensity regardless of character state.
![[Pasted image 20260504183847.png]]

---

**Section 6 , MPC Footprint UV and Wetness Update**

Updates all MPC values needed by `M_FootprintBrush` before the brush is drawn.

`WetnessLevel` is set on `MPC_FootprintData` so the brush material knows how strongly to write the wetness channel (R) this frame.

The `Calculate UV` macro converts `HitLocation` into render target UV space and sets `FootprintUV_X` and `FootprintUV_Y` on the MPC. The brush material reads these to know where in the RT to stamp the footprint.
![[Pasted image 20260504183854.png]]

---

**Section 7 , Rotation to Cos/Sin**

Converts the character's current yaw rotation into the cos and sin values used by the brush material's UV rotation math.

The actor's Yaw is retrieved, offset by 90 degrees to align the footprint texture with the forward direction, and converted from degrees to radians using `D2R`. The result is passed through both `COS` and `SIN` nodes. The sin value is negated to correct for UE5's left-handed coordinate system. Both values are set on `MPC_FootprintData` as `FootprintCos` and `FootprintSin`.
![[Pasted image 20260504183859.png]]

---

**Section 8 , Footprint Brush Draw**

The final step. Stamps the footprint into both render targets.

`IsLeftFoot` is converted to a float via Select (True = 0.0, False = 1.0) and set as `FootprintFlip` on the MPC, telling the brush material which foot orientation to use for texture flipping.

`Draw Material to Render Target` is called twice, once for `CurrentRT` and once for `PreviousRT`. Drawing to both ensures that whichever RT becomes current on the next frame already contains the footprint. Without this, footprints would flicker every other frame as the ping pong alternates between RTs.
![[Pasted image 20260504183903.png]]

# Macro Calculate UVs

**Purpose:** Converts a world-space foot position into a normalized UV coordinate (0.0 to 1.0) for use in the render target. This macro is the single source of truth for the world-to-RT coordinate mapping, used both when stamping footprints into the RT and when setting MPC values for the landscape material to read.

The foot's world position has the character's current position subtracted, giving a position relative to the character. This is divided by `LocalRTSize` to normalize it into a proportional range, then 0.5 is added to center the coordinate system so the character's exact position maps to UV (0.5, 0.5), the center of the render target.

The X and Y components are individually clamped to 0.0 to 1.0 to ensure no footprint is ever stamped outside the RT bounds.
![[Pasted image 20260504184340.png]]

#### Render Target Assets (RT_FootprintBuffer_A and RT_FootprintBuffer_B)
Two identical render target assets used as a ping pong pair by the scroll system.

```
Size          → 2048 × 2048
Format        → RTF RGBA32f
Address X     → Clamp
Address Y     → Clamp
```

**Size:** 2048 × 2048 provides enough resolution for footprints to remain sharp at typical `LocalRTSize` values. Higher resolution reduces the world units per pixel ratio, giving finer detail.

**Format:** `RTF RGBA32f` uses 32-bit floating point per channel. This high precision format is critical for the ping pong system — lower precision formats (RGBA8, RGBA16f) accumulate rounding errors each frame during the scroll copy, causing footprints to visually degrade over time. 32-bit eliminates this degradation.

**Address mode Clamp:** Prevents the RT from tiling when sampled at UV coordinates near or beyond the 0 to 1 boundary. The edge fade in `M_FootprintUpdate` handles the visual transition before content reaches the edge.

**R channel:** Wetness intensity. Drives albedo darkening and roughness in the landscape material.

**G channel:** Footprint presence for normal perturbation. Independent of wetness — always written on every footstep regardless of `WetnessLevel`.

