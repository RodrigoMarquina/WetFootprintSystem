### **World Position UV**

- `Absolute World Position` XY , Subtract A
- `CharacterPos_X` (MPC) , Append A
- `CharacterPos_Y` (MPC) , Append B
- Append , Subtract B
- Subtract , Divide A
- `LocalRTSize` (MPC) , Divide B
- Divide , Add A, B = 0.5
- Add , Texture Sample UVs , output R and G channels
![[Pasted image 20260504181925.png]]
---

### **Wetness Mask**

- Texture Sample R (from Section 1) , Add A
- `Wet` (Landscape Layer Sample) , Add B
- Add , Saturate , **Wetness Mask** named output
![[Pasted image 20260504181940.png]]
---

### **Normal Blend**

- `LLB_Normal` (Landscape Layer Blend) , BlendAngleCorrectedNormals BaseNormal
- Constant (0,0,1) , BlendAngleCorrectedNormals AdditionalNormal
- BlendAngleCorrectedNormals Result , Lerp A
- Constant (0,0,1) , Lerp B
- `Wet` (Landscape Layer Sample) , Lerp Alpha
- Lerp , Mask(R) , **goes to Image 4 top Add**
- Lerp , Mask(G) , **goes to Image 4 bottom Add**
- Lerp , Mask(B) , **goes to Image 4 Append**
![[Pasted image 20260504181951.png]]

---

### **Normal from Wetness RT**

- Texture Sample (from Section 1) , DDX Value and DDY Value
- `WetnessNormalStrength` (MPC) , Multiply B (both top and bottom)
- DDX , Multiply A (top) , Add A (top)
- DDY , Multiply A (bottom) , Add A (bottom)
- Mask(R) from Section 3 , Add B (top)
- Mask(G) from Section 3 , Add B (bottom)
- Add (top) , Append A
- Add (bottom) , Append B
- Mask(B) from Section 3 , second Append B
- Append , second Append A
- second Append , Normalize VectorInput
- Normalize , **NormalFootprint** named output
![[Pasted image 20260504182004.png]]
---

### **Physical Material Output**

- `Dirt` (Landscape Layer Sample) , PM_Dirt
- `Grass` (Landscape Layer Sample) , PM_Grass
- `Moss` (Landscape Layer Sample) , PM_Moss
- `Rock` (Landscape Layer Sample) , PM_Rock
- All four , **Landscape Physical Material Output**
![[Pasted image 20260504182016.png]]
---
### **Final Material Output**

- `LLB_Albedo` (Landscape Layer Blend) , Lerp A
- `LLB_Albedo` , Multiply A
- `Wet Darkness` (Param, default 0.7) , Multiply B
- Multiply , Lerp B
- `Wetness Mask` (from Section 2) , Lerp Alpha
- Lerp , **M_Shader Base Color**
- `LLB_ORM` , Mask(G) , Lerp A (roughness)
- `Wet Roughness` (Param, default 0.05) , Lerp B
- `Wetness Mask` , Lerp Alpha
- Lerp , **M_Shader Roughness**
- `LLB_ORM` , Mask(R) , **M_Shader Ambient Occlusion**
- `NormalFootprint` (from Section 4) , **M_Shader Normal**
- `Landscape Coords` , Divide A
- `Tiling Scale` (Param, default 1.0) , Divide B
- Divide , **UV Scale** named output
- `Wet Darkness` (Param) , **Wet Darkness** named output
- `Wet Roughness` (Param) , **Wet Roughness** named output
- M_Shader Metallic = 0.0, Specular = 0.5, Tangent = (1.0, 0.0, 0.0)
![[Pasted image 20260504182027.png]]