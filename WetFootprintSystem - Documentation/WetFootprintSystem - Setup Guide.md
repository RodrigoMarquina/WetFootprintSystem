### 1. Installation

Copy the `WetFootprintSystem` folder into your project's `Plugins` directory:

```
YourProject/
  Plugins/
    WetFootprintSystem/
```

Open your project in UE5.7. A popup will appear asking to compile the plugin — click **Yes**. Once compiled, go to **Edit → Plugins** → search `WetFootprintSystem` → verify it is enabled.

In the Content Browser, click the settings icon at the bottom right and enable **Show Plugin Content**. You should now see a `WetFootprintSystem Content` folder containing all plugin assets.
![[Pasted image 20260505163055.png]]

---
### 2. Project Settings

Go to **Edit → Project Settings → Engine → Physics → Physical Surface** and add the following surface types in order:

```
SurfaceType1 → Dirt
SurfaceType2 → Rock
SurfaceType3 → Grass
SurfaceType4 → Moss
```

These names must match the row names in `DT_FootprintData` exactly. If you want to add your own surface types, add them here and create matching rows in the Data Table.

![[Pasted image 20260505163030.png]]

---

### 3. Landscape Setup

#### 3a. Apply the Landscape Material

Assign `M_Shader` from the plugin content to your landscape. This material handles all wetness darkening, roughness changes, normal flattening, and footprint visualization automatically.

If you want to integrate the wetness system into your own landscape material instead, refer to the M_Shader documentation for the node breakdown.

![[Pasted image 20260505163256.png]]
![[Pasted image 20260505163330.png]]
#### 3b. Create Layer Info Assets

In the Landscape Paint mode, create a Layer Info asset for each layer. The blend mode for each must be set correctly:

```
Dirt  → Weight Blend (Legacy)
Rock  → Weight Blend (Legacy)
Grass → Weight Blend (Legacy)
Moss  → Weight Blend (Legacy)
Wet   → No Weight Blending
```

The `Wet` layer uses No Weight Blending so it acts as an independent overlay on top of all other layers. The plugin includes pre-configured Layer Info assets in `WetFootprintSystem Content/PhysicalMaterials/` that you can use directly.

![[Pasted image 20260505163428.png]]

#### 3c. Assign Physical Materials

Open each Layer Info asset → Details panel → assign the corresponding Physical Material:

```
Dirt_LayerInfo  → PM_Dirt
Rock_LayerInfo  → PM_Rock
Grass_LayerInfo → PM_Grass
Moss_LayerInfo  → PM_Moss
Wet_LayerInfo   → None (no physical material needed)
```

![[Pasted image 20260505163541.png]]

#### 3d. Paint Your Landscape

Paint your surface layers as normal. For wet areas, paint the `Wet` layer on top of any surface — the system reads this layer to determine ground wetness level at runtime.

![[Pasted image 20260505163632.png]]

---

### 4. Character Setup

#### 4a. Add the Footprint Component

Open your Character Blueprint → **Add Component** → search `BP_FootprintComponent` → add it.

![[Pasted image 20260505163737.png]]

#### 4b. Set Up Foot Sockets

Open your Skeletal Mesh asset → **Skeleton** tab.

Right click the left foot bone → **Add Socket** → name it exactly `foot_l`. Right click the right foot bone → **Add Socket** → name it exactly `foot_r`.

The line trace will fire downward from this position to detect the ground surface.

![[Pasted image 20260505164713.png]]

#### 4c. Add Foot notifies

We need to add notifies in the Animation Sequence when each foot touches the ground.

Start by creating a new **Notify Track**.

![[Pasted image 20260505165223.png]]

Find the exact moment the character's foot lands on the ground and add a notify. Name the new notify `foot_l` and `foot_r`.
![[Pasted image 20260505165311.png]]
![[Pasted image 20260505165355.png]]
![[Pasted image 20260505165537.png]]

#### 4d. Set Up the Animation Blueprint

Open your Animation Blueprint → **Event Graph**.

First in Begin Play set the Footprint Component reference.

```
Get Character
        ↓
Get Component by Class (BP_FootprintComponent)
        ↓
SET FootprintComponent variable
```

![[Pasted image 20260505164556.png]]

Now, add two notify handlers, one for each foot:

**Left foot:**

```
AnimNotify_l_foot_plant
        ↓
Get Socket Location (Target = Mesh, Socket = "foot_l") → Line Trace Start
        ↓
Line Trace By Channel
  Start = above
  End = Start - (X=0, Y=0, Z=50)
  Channel = Visibility
  Trace Complex = true
        ↓
Branch (Return Value = true)
  True → Handle Footstep (BPI_SurfaceResponse)
    Target = FootprintComponent
    Is Left Foot = true
    Hit Result = Out Hit
```

**Right foot:** identical but with socket `foot_r` and `Is Left Foot = false`.

![[Pasted image 20260505170358.png]]

---

### 5. Tuning

#### Data Table

Open `DT_FootprintData` → adjust values per surface type:

```
DryNormalStrength     → how deep footprint indents look on dry ground (0.0 to 3.0)
WetnessNormalStrength → how deep footprint indents look on wet ground (0.0 to 5.0)
NiagaraSystem_Dry     → particle effect for dry footsteps
NiagaraSystem_Wet     → particle effect for wet footsteps
```

#### Global Parameters

Select your character in the level → Details panel → find the `BP_FootprintComponent` section:

```
GlobalFadeSpeed  → how fast footprints fade from the landscape (default 0.001)
GlobalDryingRate → how fast feet dry when walking on dry ground (default 0.15)
LocalRTSize      → world units covered by the render target (default 2000)
RTSize           → render target resolution in pixels (default 2048)
```

---

### 6. Testing

Press Play → walk over a painted wet area → you should see:

- Feet get wet based on the wetness layer paint strength
- Wet footprints appear on the landscape surface
- Footprints persist and scroll with the character
- Footprints fade gradually over time
- Particle effects spawn on each footstep
- Feet dry gradually when walking on dry ground

**Important: After painting the Wet layer on new landscape components, you need to Save All → restart the editor for weightmap data to be properly written.**

![[Pasted image 20260505170711.png]]