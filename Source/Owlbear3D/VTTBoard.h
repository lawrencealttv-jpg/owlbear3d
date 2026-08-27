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
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

    void SetCellActive(const FIntPoint& Grid, bool bActive);

    UFUNCTION(BlueprintCallable, Category="VTT|Grid")
    bool ToggleWallAtWorldLocation(const FVector& WorldLocation);

    bool FindNearestEdge(const FVector& WorldLocation, FIntVector& OutEdge) const;
    bool HasWall(const FIntVector& Edge) const;
    bool HasDoor(const FIntVector& Edge) const;
    void SetWall(const FIntVector& Edge, bool bActive);
    void SetDoor(const FIntVector& Edge, bool bActive);
    void SetFogged(const FIntPoint& Grid, bool bFogged);
    bool IsFogged(const FIntPoint& Grid) const;

    TArray<FIntPoint> GetActiveCells() const;
    TArray<FIntVector> GetWallEdges() const;
    TArray<FIntVector> GetDoorEdges() const;
    TArray<FIntPoint> GetFoggedCells() const;
    void ApplyLayout(const TArray<FIntPoint>& NewActiveCells, const TArray<FIntVector>& NewWallEdges,
        const TArray<FIntVector>& NewDoorEdges, const TArray<FIntPoint>& NewFoggedCells);

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

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> DoorInstances;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UInstancedStaticMeshComponent> FogInstances;

    TSet<FIntPoint> ActiveCells;
    TSet<FIntVector> WallEdges;
    TSet<FIntVector> DoorEdges;
    TSet<FIntPoint> FoggedCells;

    UPROPERTY(ReplicatedUsing=OnRep_Layout)
    TArray<FIntPoint> RepActiveCells;

    UPROPERTY(ReplicatedUsing=OnRep_Layout)
    TArray<FIntVector> RepWallEdges;

    UPROPERTY(ReplicatedUsing=OnRep_Layout)
    TArray<FIntVector> RepDoorEdges;

    UPROPERTY(ReplicatedUsing=OnRep_Layout)
    TArray<FIntPoint> RepFoggedCells;

    void BuildInitialRoom();
    void RebuildFloorInstances();
    void RebuildWallInstances();
    void RebuildDoorInstances();
    void RebuildFogInstances();
    bool IsValidEdge(const FIntVector& Edge) const;
    void SyncReplicatedLayout();

    UFUNCTION()
    void OnRep_Layout();
};
