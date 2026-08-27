#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VTTProp.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EVTTPropType : uint8
{
    Crate,
    Table,
    Column,
    Statue,
    Chest
};

UCLASS()
class OWLBEAR3D_API AVTTProp : public AActor
{
    GENERATED_BODY()

public:
    AVTTProp();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="VTT|Prop")
    EVTTPropType PropType = EVTTPropType::Crate;

    void InitialiseProp(EVTTPropType NewType);
    void SetSelected(bool bNewSelected);
    void RotateClockwise();
    FString GetPropName() const;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> HitBox;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> PropMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> SelectionMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeAsset;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderAsset;

    UPROPERTY()
    TObjectPtr<UStaticMesh> ConeAsset;
};
