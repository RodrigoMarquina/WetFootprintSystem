### **FS_FootprintData — Struct**

![[Pasted image 20260504153649.png]]
The struct defines the schema for each row in the Data Table. Every surface type has one row containing all the artistic parameters that control how that surface behaves when interacted with.

Fields:
`DryNormalStrength : Float` Controls how strongly the footprint indents the surface normal when the character's feet are dry. Higher values create more visible impressions. Set to 0 for hard surfaces like rock where dry footprints leave no mark.

`WetnessNormalStrength : Float` Same as above but for wet feet. This value should typically be higher than DryNormalStrength — wet mud deforms more visibly than dry dirt. The system lerps between these two values based on current WetnessLevel.

`NiagaraSystem_Dry : Niagara System` The particle effect that spawns when the character steps on this surface with dry feet. Examples: dust puff for dirt, rock chip for stone, grass rustle for grass.

`NiagaraSystem_Wet : Niagara System` The particle effect for wet footsteps. The WetnessLevel value is passed directly into this system via a User Parameter called `WetnessIntensity`, artists can use it to scale particle count, size, or velocity inside the Niagara asset.

---
### **DT_FootprintData — Data Table**

![[Pasted image 20260504153641.png]]
One row per physical surface type. Row names must match the Surface Type names defined in Project Settings → Physics → Physical Surface exactly, the system uses these names for the Data Table lookup at runtime.

Example row naming:
```
SurfaceType1 → Dirt
SurfaceType2 → Mud  
SurfaceType3 → Rock
SurfaceType4 → Grass
```

Global parameters that affect all surfaces equally, `GlobalFadeSpeed` and `GlobalDryingRate`, live as Instance Editable variables directly on the character Blueprint, not in the Data Table. This is intentional: since the render target is shared across all surfaces, per-surface fade speed would cause inconsistent behavior.