#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "VTTSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FVTTPieceSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FString DisplayName;

    UPROPERTY()
    int32 MaxHP = 1;

    UPROPERTY()
    int32 CurrentHP = 1;

    UPROPERTY()
    FLinearColor Colour = FLinearColor::White;

    UPROPERTY()
    FVector Location = FVector::ZeroVector;
};

UCLASS()
class OWLBEAR3D_API UVTTSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<FIntPoint> ActiveCells;

    UPROPERTY()
    TArray<FIntVector> WallEdges;

    UPROPERTY()
    TArray<FVTTPieceSaveData> Pieces;
};

