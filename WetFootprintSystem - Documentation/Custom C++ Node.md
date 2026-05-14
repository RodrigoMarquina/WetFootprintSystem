**GetLandscapeLayerWeightAtLocation**

_Purpose:_ Read the exact paint weight (0.0 to 1.0) of a specific landscape layer at any world position at runtime.

_Why C++:_ UE5 does not expose landscape weightmap data to Blueprint natively. The weightmap is stored as a GPU texture — accessing it requires C++ to lock the mip data and read raw pixel values.

_Parameters:_

- `Landscape` — reference to the ALandscapeProxy actor in the level
- `Location` — world position to sample (typically the foot hit location)
- `LayerName` — name of the landscape layer to read (e.g. "Wet")

_Returns:_ Float 0.0 to 1.0 — the paint weight of the specified layer at the given location.

### LandscapeWeightLibrary.cpp

``` C++
#include "LandscapeWeightLibrary.h"
#include "LandscapeProxy.h"
#include "LandscapeComponent.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"

float ULandscapeWeightLibrary::GetLandscapeLayerWeightAtLocation(
    ALandscapeProxy* Landscape,
    FVector Location,
    FName LayerName)
{
    if (!Landscape) return 0.0f;
    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    if (!LandscapeInfo) return 0.0f;
    // Find component at location
    ULandscapeComponent* FoundComponent = nullptr;
    
    for (auto& Pair : LandscapeInfo->XYtoComponentMap)
    {
        ULandscapeComponent* Component = Pair.Value;
        if (Component && Component->Bounds.GetBox().IsInsideOrOn(Location))
        {
            FoundComponent = Component;
            break;
        }
    }
    if (!FoundComponent) return 0.0f;
    // Get runtime weightmap allocations
    const TArray<FWeightmapLayerAllocationInfo>& Allocs =
        FoundComponent->GetCurrentRuntimeWeightmapLayerAllocations();

    for (const FWeightmapLayerAllocationInfo& Alloc : Allocs)
    {
        ULandscapeLayerInfoObject* LayerInfoPtr = Alloc.LayerInfo.Get();
        if (LayerInfoPtr)
        {
            UE_LOG(LogTemp, Warning, TEXT("Layer: %s"), *LayerInfoPtr->GetLayerName().ToString());
        }
    }

    // Get weightmap textures
    const TArray<UTexture2D*>& Textures =
        FoundComponent->GetWeightmapTextures(false);
    // Convert world location to local component coordinates

    FVector LocalPos = FoundComponent->GetComponentTransform()
        .InverseTransformPosition(Location);
    int32 SizeX = FoundComponent->ComponentSizeQuads + 1;
    int32 X = FMath::Clamp(FMath::RoundToInt(LocalPos.X), 0, SizeX - 1);
    int32 Y = FMath::Clamp(FMath::RoundToInt(LocalPos.Y), 0, SizeX - 1);

    // Find the layer and sample its weightmap
    for (const FWeightmapLayerAllocationInfo& Alloc : Allocs)
    {
        ULandscapeLayerInfoObject* LayerInfoPtr = Alloc.LayerInfo.Get();
        if (!LayerInfoPtr || LayerInfoPtr->GetLayerName() != LayerName) continue;
        if (!Textures.IsValidIndex(Alloc.WeightmapTextureIndex)) continue;
        UTexture2D* WeightmapTex = Textures[Alloc.WeightmapTextureIndex];
        if (!WeightmapTex) return 0.0f;
        FTexture2DMipMap& Mip = WeightmapTex->GetPlatformData()->Mips[0];
        FColor* Data = (FColor*)Mip.BulkData.Lock(LOCK_READ_ONLY);
        if (!Data) { Mip.BulkData.Unlock(); return 0.0f; }
        int32 TexSize = WeightmapTex->GetSizeX();
        FColor Pixel = Data[Y * TexSize + X];
        Mip.BulkData.Unlock();
        uint8 Weight = Alloc.WeightmapTextureChannel == 0 ? Pixel.R :
                       Alloc.WeightmapTextureChannel == 1 ? Pixel.G :
                       Alloc.WeightmapTextureChannel == 2 ? Pixel.B : Pixel.A;
        return Weight / 255.0f;
    }
    return 0.0f;
}
```

### LandscapeWeightLibrary.h

``` C++
#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LandscapeWeightLibrary.generated.h"
UCLASS()

class PHYSICALMATERIALTEST_API ULandscapeWeightLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Landscape")
    static float GetLandscapeLayerWeightAtLocation(
        ALandscapeProxy* Landscape,
        FVector Location,
        FName LayerName);
};
```


Function signature
``` C++
float ULandscapeWeightLibrary::GetLandscapeLayerWeightAtLocation( ALandscapeProxy* Landscape, FVector Location, FName LayerName)
```
Takes three inputs — the landscape actor, a world position, and the name of the layer to sample. Returns a float 0.0 to 1.0.

```cpp
if (!Landscape) return 0.0f;
ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
if (!LandscapeInfo) return 0.0f;
```
Null checks before doing anything. `LandscapeInfo` is a metadata object that UE5 uses to track all landscape components, layers and their relationships. If either is invalid, return 0 safely.

```cpp
for (auto& Pair : LandscapeInfo->XYtoComponentMap)
{
    ULandscapeComponent* Component = Pair.Value;
    if (Component && Component->Bounds.GetBox().IsInsideOrOn(Location))
    {
        FoundComponent = Component;
        break;
    }
}
```
A landscape is divided into a grid of components, think of them as tiles. `XYtoComponentMap` is a public map of all components indexed by their grid position. This loop finds which tile the foot is standing on by checking if the world position falls inside each component's bounding box. Once found it breaks, no need to check the rest.

```cpp
const TArray<FWeightmapLayerAllocationInfo>& Allocs =
    FoundComponent->GetCurrentRuntimeWeightmapLayerAllocations();
```
Each landscape component stores its painted layer data in weightmap textures. Multiple layers are packed into a single RGBA texture — each layer occupies one channel (R, G, B, or A). `GetCurrentRuntimeWeightmapLayerAllocations()` returns the list of all layers painted on this component, including which texture and which channel each layer uses.

```cpp
FVector LocalPos = FoundComponent->GetComponentTransform()
    .InverseTransformPosition(Location);
int32 SizeX = FoundComponent->ComponentSizeQuads + 1;
int32 X = FMath::Clamp(FMath::RoundToInt(LocalPos.X), 0, SizeX - 1);
int32 Y = FMath::Clamp(FMath::RoundToInt(LocalPos.Y), 0, SizeX - 1);
```
The weightmap texture stores one value per vertex. To read the correct pixel, the world position needs to be converted into local component space, then mapped to a pixel coordinate. `InverseTransformPosition` transforms from world to local. `ComponentSizeQuads + 1` gives the number of vertices (quads + 1 because vertices include both endpoints). The result is clamped to stay within texture bounds.

```cpp
for (const FWeightmapLayerAllocationInfo& Alloc : Allocs)
{
    ULandscapeLayerInfoObject* LayerInfoPtr = Alloc.LayerInfo.Get();
    if (!LayerInfoPtr || LayerInfoPtr->GetLayerName() != LayerName) continue;
    if (!Textures.IsValidIndex(Alloc.WeightmapTextureIndex)) continue;

    UTexture2D* WeightmapTex = Textures[Alloc.WeightmapTextureIndex];
    FTexture2DMipMap& Mip = WeightmapTex->GetPlatformData()->Mips[0];
    FColor* Data = (FColor*)Mip.BulkData.Lock(LOCK_READ_ONLY);
```
Iterates through all layer allocations looking for the one matching `LayerName`. Once found, gets the weightmap texture and locks its first mip level for CPU reading. `BulkData.Lock` gives direct access to the raw pixel data in CPU memory.

```cpp
FColor Pixel = Data[Y * TexSize + X];
Mip.BulkData.Unlock();

uint8 Weight = Alloc.WeightmapTextureChannel == 0 ? Pixel.R :
               Alloc.WeightmapTextureChannel == 1 ? Pixel.G :
               Alloc.WeightmapTextureChannel == 2 ? Pixel.B : Pixel.A;

return Weight / 255.0f;
```
Reads the pixel at the calculated coordinate. Since multiple layers share one texture, each layer is assigned a specific channel (R, G, B, or A). `WeightmapTextureChannel` tells us which one. The raw value is a byte (0-255), dividing by 255 converts it to a normalized float (0.0 to 1.0).