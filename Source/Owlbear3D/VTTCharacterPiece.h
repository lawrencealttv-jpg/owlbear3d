#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VTTCharacterPiece.generated.h"

class UCapsuleComponent;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EVTTVisualType : uint8
{
    Adventurer,
    Paladin,
    Mage,
    Goblin,
    Orc,
    Skeleton,
    Wolf,
    Spider,
    Slime,
    Warforged,
    Drake
};

UCLASS()
class OWLBEAR3D_API AVTTCharacterPiece : public AActor
{
    GENERATED_BODY()

public:
    AVTTCharacterPiece();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category="VTT|Piece")
    void InitialisePiece(const FString& NewName, int32 NewMaxHP, const FLinearColor& NewColour);

    UFUNCTION(BlueprintCallable, Category="VTT|Piece")
    void SetSelected(bool bNewSelected);

    UFUNCTION(BlueprintCallable, Category="VTT|Piece")
    void MoveToGridLocation(const FVector& NewLocation);

    UFUNCTION(BlueprintCallable, Category="VTT|Piece")
    void ChangeHP(int32 Delta);

    void CycleSize();
    void CycleCondition();
    void ToggleHiddenFromPlayers();
    void SetSizeSquares(int32 NewSize);
    void CycleVisualType();
    void SetVisualType(EVTTVisualType NewVisualType);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    FString DisplayName = TEXT("Adventurer");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    int32 MaxHP = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    int32 CurrentHP = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    FLinearColor PieceColour = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    int32 SizeSquares = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    FString ConditionText;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    bool bHiddenFromPlayers = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PieceData, Category="VTT|Piece")
    EVTTVisualType VisualType = EVTTVisualType::Adventurer;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCapsuleComponent> HitCapsule;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> BaseMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> HeadMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> SelectionMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> ImportedVisualMesh;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> NameText;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> HPText;

    FVector TargetLocation;
    bool bMoving = false;

    void UpdateLabels();
    void ApplyColour(const FLinearColor& NewColour);

    UFUNCTION()
    void OnRep_PieceData();
};
