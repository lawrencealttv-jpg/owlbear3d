#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VTTProp.h"
#include "VTTSaveGame.h"
#include "VTTPlayerController.generated.h"

class AVTTBoard;
class AVTTCharacterPiece;
class AVTTProp;
class SWidget;

enum class EVTTBuildTool : uint8
{
    Floor,
    Wall,
    Door,
    Prop,
    Fog
};

struct FVTTInitiativeEntry
{
    TWeakObjectPtr<AVTTCharacterPiece> Piece;
    int32 Roll = 0;
};

UCLASS()
class OWLBEAR3D_API AVTTPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AVTTPlayerController();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY()
    TObjectPtr<AVTTBoard> Board;

    UPROPERTY()
    TObjectPtr<AVTTCharacterPiece> SelectedPiece;

    UPROPERTY()
    TObjectPtr<AVTTProp> SelectedProp;

    bool bBuildMode = false;
    EVTTBuildTool BuildTool = EVTTBuildTool::Floor;
    EVTTPropType CurrentPropType = EVTTPropType::Crate;
    int32 HeroCount = 1;
    int32 MonsterCount = 1;
    bool bPainting = false;
    bool bPaintAdd = true;
    bool bHasLastPaintTarget = false;
    FIntPoint LastPaintCell = FIntPoint(-1, -1);
    FIntVector LastPaintEdge = FIntVector(-1, -1, -1);
    bool bMeasureMode = false;
    bool bHasMeasureStart = false;
    FVector MeasureStart = FVector::ZeroVector;
    TArray<FVTTMapSnapshot> UndoStack;
    TArray<FVTTMapSnapshot> RedoStack;
    TArray<FVTTInitiativeEntry> Initiative;
    int32 InitiativeIndex = 0;
    bool bAutosavePending = false;
    float AutosaveAtTime = 0.0f;
    int32 CurrentSceneIndex = 1;
    FVTTMapSnapshot DefaultSceneSnapshot;
    FString JoinAddress = TEXT("127.0.0.1");
    TSharedPtr<SWidget> ToolbarWidget;

    void PrimaryClick();
    void PrimaryRelease();
    void ApplyBuildAtCursor(bool bStartingStroke);

    UFUNCTION(Server, Reliable)
    void ServerMoveActor(AActor* Actor, FVector NewLocation);
    void SpawnHero();
    void SpawnMonster();
    void SpawnPieceAtCursor(const FString& PieceName, int32 MaxHP, const FLinearColor& Colour);
    void DamageSelected();
    void HealSelected();
    void DeleteSelected();
    void RotateSelected();
    void DuplicateSelected();
    void ToggleBuildMode();
    void CycleBuildTool();
    void CycleProp();
    void CycleSelectedSize();
    void CycleSelectedCondition();
    void ToggleSelectedHidden();
    void ToggleMeasure();
    void RollD20();
    void AddSelectedToInitiative();
    void NextInitiativeTurn();
    void ShowInitiative();
    void HandleMeasureClick(const FVector& WorldLocation);
    void SaveMap();
    void WriteSaveSlot(bool bShowMessage);
    void LoadMap();
    void PreviousScene();
    void NextScene();
    void SwitchScene(int32 NewSceneIndex);
    void HostSession();
    void JoinSession();
    void Undo();
    void Redo();
    bool GetCursorGrid(FIntPoint& OutGrid, FVector& OutWorld) const;
    void SelectPiece(AVTTCharacterPiece* NewSelection);
    void SelectProp(AVTTProp* NewSelection);
    void SpawnPropAt(const FVector& WorldLocation, EVTTPropType PropType, const FRotator& Rotation = FRotator::ZeroRotator);
    FVTTMapSnapshot CaptureSnapshot() const;
    void ApplySnapshot(const FVTTMapSnapshot& Snapshot);
    void PushUndoState();
    FString GetBuildToolName() const;
    FString GetToolbarStatus() const;
    FString GetSaveSlotName() const;
    void SetBuildTool(EVTTBuildTool NewTool);
    void BuildToolbar();
    void RemoveToolbar();
    void ShowControls() const;
};
