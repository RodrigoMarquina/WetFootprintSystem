**Purpose:** Renders a single footprint stamp into the render target at the correct world-relative UV position, oriented to the character's walking direction, with independent control over wetness intensity and normal perturbation strength.

**Material Settings:**

```
Domain     → Surface
Blend Mode → Additive
Shading    → Unlit
```

---

**Section 1 , UV Rotation**

Takes the raw UV coordinate of the render target pixel, centers it on the foot's UV position, then rotates it to match the character's walking direction.

The foot position in UV space comes from `FootprintUV_X` and `FootprintUV_Y` (MPC), which are set in Blueprint from the foot's world position relative to the character. The UV is centered by subtracting this position from `TexCoord[0]`.

The rotation is applied using a standard 2D rotation matrix driven by `FootprintCos` and `FootprintSin` (MPC), which are pre-calculated in Blueprint from the character's yaw angle. This ensures footprints always face the direction of movement regardless of which way the character is walking.

The output is a rotated UV offset centered at the origin, passed to Section 2.
![[Pasted image 20260504175931.png]]

---

**Section 2 , UV Scale and Footprint Sampling**

Scales the rotated UV to the correct footprint size and samples the foot shape texture.

The rotated UV from Section 1 is divided by `FootprintSize` (MPC) to control how large the footprint appears in world space. Adding 0.5 recenters the UV for texture sampling.

Left/right foot flipping is handled by a Lerp between the normal U coordinate and its mirror (`1 - U`), driven by `FootprintFlip` (MPC). This allows a single texture to serve both feet without needing two separate assets.

The resulting UV samples the footprint texture, producing the foot shape as an alpha mask.
![[Pasted image 20260504180000.png]]

---

**Section 3 , Radial Opacity Mask**

Generates a circular falloff mask centered on the foot position. This prevents the footprint texture from tiling or bleeding outside the intended stamp area.

The distance from the current pixel to the foot UV position is calculated using Length on the centered UV offset. This distance is normalized by `FootprintSize` and inverted to produce a value of 1 at the center and 0 at the edges.

This mask is multiplied with the texture sample from Section 2, cleanly bounding the footprint shape.
![[Pasted image 20260504180129.png]]

---

**Section 4 , Channel Output and Wetness Encoding**

Encodes the footprint stamp into two separate channels of the render target, each carrying different information:

**R channel , Wetness intensity** The masked footprint shape is multiplied by `WetnessLevel` (MPC). This channel drives the darkening and roughness changes in the landscape material. When the character is dry, R = 0 and no wetness appears visually.

**G channel , Normal perturbation intensity** The masked footprint shape is multiplied by a value that lerps between `DryNormalStrength` and `WetnessNormalStrength` (MPC) based on `WetnessLevel`. This channel drives the normal indent effect in the landscape material independently of wetness. Even on dry ground, footprints can perturb the surface normal, giving a subtle physical impression.

Both channels are combined into the final RGB output via AppendMany and written to the render target as Emissive Color.
![[Pasted image 20260504180420.png]]