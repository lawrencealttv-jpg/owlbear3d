#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VTTPlayerController.generated.h"

class AVTTBoard;
class AVTTCharacterPiece;

UCLASS()
class OWLBEAR3D_API AVTTPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AVTTPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    UPROPERTY()
    TObjectPtr<AVTTBoard> Board;

    UPROPERTY()
    TObjectPtr<AVTTCharacterPiece> SelectedPiece;

    bool bBuildMode = false;
    int32 HeroCount = 1;
    int32 MonsterCount = 1;

    void PrimaryClick();
    void SpawnHero();
    void SpawnMonster();
    void SpawnPieceAtCursor(const FString& PieceName, int32 MaxHP, const FLinearColor& Colour);
    void DamageSelected();
    void HealSelected();
    void DeleteSelected();
    void ToggleBuildMode();
    bool GetCursorGrid(FIntPoint& OutGrid, FVector& OutWorld) const;
    void SelectPiece(AVTTCharacterPiece* NewSelection);
    void ShowControls() const;
};

