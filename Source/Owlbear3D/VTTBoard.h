#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VTTBoard.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;

UCLASS()
class OWLBEAR3D_API AVTTBoard : public AActor
{
    GENERATED_BODY()

public:
    AVTTBoard();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VTT|Grid")
    int32 GridWidth = 16;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VTT|Grid")
    int32 GridHeight = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VTT|Grid")
    float TileSize = 100.0f;

    UFUNCTION(BlueprintCallable, Category="VTT|Grid")
    bool WorldToGrid(const FVector& WorldLocation, FIntPoint& OutGrid) const;

    UFUNCTION(BlueprintCallable, Category="VTT|Grid")
    FVector GridToWorld(const FIntPoint& Grid) const;

    UFUNCTION(BlueprintCallable, Category="VTT|Grid")
    bool IsCellActive(const FIntPoint& Grid) const;

    UFUNCTION(BlueprintCallable, Category="VTT|Grid")
    void ToggleCell(const FIntPoint& Grid);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> GridSurface;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> FloorInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> WallInstances;

    TSet<FIntPoint> ActiveCells;

    void BuildInitialRoom();
    void RebuildFloorInstances();
    void RebuildBoundaryWalls();
};
