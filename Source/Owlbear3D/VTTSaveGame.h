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

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY()
    int32 SizeSquares = 1;

    UPROPERTY()
    FString ConditionText;

    UPROPERTY()
    bool bHiddenFromPlayers = false;
};

USTRUCT(BlueprintType)
struct FVTTPropSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    uint8 PropType = 0;

    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FVTTMapSnapshot
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FIntPoint> ActiveCells;

    UPROPERTY()
    TArray<FIntVector> WallEdges;

    UPROPERTY()
    TArray<FIntVector> DoorEdges;

    UPROPERTY()
    TArray<FIntPoint> FoggedCells;

    UPROPERTY()
    TArray<FVTTPieceSaveData> Pieces;

    UPROPERTY()
    TArray<FVTTPropSaveData> Props;
};

UCLASS()
class OWLBEAR3D_API UVTTSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FVTTMapSnapshot Map;
};
