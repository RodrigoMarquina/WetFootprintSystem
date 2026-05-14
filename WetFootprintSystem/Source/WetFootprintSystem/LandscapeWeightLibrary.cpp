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