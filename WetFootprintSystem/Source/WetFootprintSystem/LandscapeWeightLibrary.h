#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LandscapeWeightLibrary.generated.h"

UCLASS()
class WETFOOTPRINTSYSTEM_API ULandscapeWeightLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Landscape")
    static float GetLandscapeLayerWeightAtLocation(
        ALandscapeProxy* Landscape,
        FVector Location,
        FName LayerName);
};