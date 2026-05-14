### **1. Surface Detection Layer**
The foundation of the system. Two parallel detection mechanisms run on every footstep:

- **Physical Material** — a line trace from the foot socket reads the Physical Material at the hit location, returning a Surface Type enum. This drives the Data Table lookup, determining which Niagara systems and normal values to use.
- **C++ Layer Weight Function** — a custom Blueprint Function Library reads the landscape weightmap texture directly at runtime, returning the exact wetness layer weight (0.0 to 1.0) at the foot's world position. This was implemented in C++ because UE5 does not expose landscape layer weights to Blueprint natively.

---
### **2. Wetness State Layer**
Lives on the character. Maintains a single `WetnessLevel` float (0.0 to 1.0) that represents how wet the character's feet currently are:

- If ground wetness > current WetnessLevel → WetnessLevel instantly updates to ground wetness
- If ground wetness < current WetnessLevel → WetnessLevel lerps toward ground wetness using the surface DryingRate
- Time-based decay runs in Event Tick — feet dry gradually even when standing still

---
### **3. Render Target Layer**

A ping pong render target system that stores footprint data in world space:

- Two render targets (A and B) alternate each frame
- `M_FootprintUpdate` reads from the previous RT, shifts content by the character's frame delta (pixel-snapped to avoid bilinear blur), and applies a global fade
- `M_FootprintBrush` stamps a foot-shaped mark at the correct UV position on each footstep
- The RT coverage area follows the character — when content reaches the edge it fades out cleanly

The UV coordinate system maps world position relative to the character's current position into 0-1 RT space, keeping footprints correctly anchored in the world.

---
### **4. Landscape Material Layer**

`M_Shader` reads from the render target and the landscape wet layer simultaneously to produce the final visual result:

- **Wetness mask** — combines RT R channel (footprint wetness) and Landscape Layer Sample "Wet" (painted wet areas)
- **Albedo darkening** — Lerp between base color and darkened version driven by wetness mask
- **Roughness** — Lerp toward 0.05 (very shiny) on wet areas
- **Normal flattening** — full normal vector Lerps toward (0,0,1) based on painted wet layer
- **Footprint normal perturbation** — DDX/DDY of RT G channel drives normal indent, intensity controlled per surface via Data Table

---
### **5. Data Table Layer**

`DT_FootprintData` with struct `FS_FootprintData` — the artist-facing control panel:

```
DryNormalStrength     → normal perturbation intensity on dry ground
WetnessNormalStrength → normal perturbation intensity on wet ground
NiagaraSystem_Dry     → particle effect for dry footstep impact
NiagaraSystem_Wet     → particle effect for wet footstep impact
```

Global parameters (FadeSpeed, DryingRate) live on the character as Instance Editable variables — tunable per character without opening Blueprints.