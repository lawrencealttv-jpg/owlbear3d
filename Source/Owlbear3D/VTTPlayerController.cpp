#include "VTTPlayerController.h"

#include "VTTBoard.h"
#include "VTTCharacterPiece.h"
#include "VTTProp.h"
#include "VTTSaveGame.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
    const FString SaveSlotPrefix = TEXT("Owlbear3D_Scene_");
}

AVTTPlayerController::AVTTPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AVTTPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    for (TActorIterator<AVTTBoard> It(GetWorld()); It; ++It)
    {
        Board = *It;
        break;
    }

    DefaultSceneSnapshot = CaptureSnapshot();
    if (HasAuthority() && UGameplayStatics::DoesSaveGameExist(GetSaveSlotName(), 0))
    {
        if (UVTTSaveGame* SaveData = Cast<UVTTSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSaveSlotName(), 0)))
        {
            ApplySnapshot(SaveData->Map);
        }
    }

    ShowControls();
    BuildToolbar();
}

void AVTTPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction(TEXT("PrimaryClick"), IE_Pressed, this, &AVTTPlayerController::PrimaryClick);
    InputComponent->BindAction(TEXT("PrimaryClick"), IE_Released, this, &AVTTPlayerController::PrimaryRelease);
    InputComponent->BindAction(TEXT("SpawnHero"), IE_Pressed, this, &AVTTPlayerController::SpawnHero);
    InputComponent->BindAction(TEXT("SpawnMonster"), IE_Pressed, this, &AVTTPlayerController::SpawnMonster);
    InputComponent->BindAction(TEXT("DamagePiece"), IE_Pressed, this, &AVTTPlayerController::DamageSelected);
    InputComponent->BindAction(TEXT("HealPiece"), IE_Pressed, this, &AVTTPlayerController::HealSelected);
    InputComponent->BindAction(TEXT("DeletePiece"), IE_Pressed, this, &AVTTPlayerController::DeleteSelected);
    InputComponent->BindAction(TEXT("ToggleBuildMode"), IE_Pressed, this, &AVTTPlayerController::ToggleBuildMode);
    InputComponent->BindAction(TEXT("CycleBuildTool"), IE_Pressed, this, &AVTTPlayerController::CycleBuildTool);
    InputComponent->BindAction(TEXT("SaveMap"), IE_Pressed, this, &AVTTPlayerController::SaveMap);
    InputComponent->BindAction(TEXT("LoadMap"), IE_Pressed, this, &AVTTPlayerController::LoadMap);
    InputComponent->BindAction(TEXT("RotateSelection"), IE_Pressed, this, &AVTTPlayerController::RotateSelected);
    InputComponent->BindAction(TEXT("DuplicateSelection"), IE_Pressed, this, &AVTTPlayerController::DuplicateSelected);
    InputComponent->BindAction(TEXT("CycleProp"), IE_Pressed, this, &AVTTPlayerController::CycleProp);
    InputComponent->BindAction(TEXT("Undo"), IE_Pressed, this, &AVTTPlayerController::Undo);
    InputComponent->BindAction(TEXT("Redo"), IE_Pressed, this, &AVTTPlayerController::Redo);
    InputComponent->BindAction(TEXT("CycleSize"), IE_Pressed, this, &AVTTPlayerController::CycleSelectedSize);
    InputComponent->BindAction(TEXT("CycleCondition"), IE_Pressed, this, &AVTTPlayerController::CycleSelectedCondition);
    InputComponent->BindAction(TEXT("ToggleHidden"), IE_Pressed, this, &AVTTPlayerController::ToggleSelectedHidden);
    InputComponent->BindAction(TEXT("ToggleMeasure"), IE_Pressed, this, &AVTTPlayerController::ToggleMeasure);
    InputComponent->BindAction(TEXT("RollD20"), IE_Pressed, this, &AVTTPlayerController::RollD20);
    InputComponent->BindAction(TEXT("AddInitiative"), IE_Pressed, this, &AVTTPlayerController::AddSelectedToInitiative);
    InputComponent->BindAction(TEXT("NextTurn"), IE_Pressed, this, &AVTTPlayerController::NextInitiativeTurn);
    InputComponent->BindAction(TEXT("CycleVisual"), IE_Pressed, this, &AVTTPlayerController::CycleSelectedVisual);
}

void AVTTPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bPainting && bBuildMode)
    {
        ApplyBuildAtCursor(false);
    }
    if (bAutosavePending && GetWorld() && GetWorld()->GetTimeSeconds() >= AutosaveAtTime)
    {
        WriteSaveSlot(false);
        bAutosavePending = false;
    }
}

void AVTTPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RemoveToolbar();
    Super::EndPlay(EndPlayReason);
}

void AVTTPlayerController::PrimaryClick()
{
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit))
    {
        return;
    }

    if (bMeasureMode)
    {
        HandleMeasureClick(Hit.ImpactPoint);
        return;
    }

    if (bBuildMode)
    {
        PushUndoState();
        bPainting = BuildTool != EVTTBuildTool::Prop;
        bHasLastPaintTarget = false;
        ApplyBuildAtCursor(true);
        return;
    }

    if (AVTTCharacterPiece* ClickedPiece = Cast<AVTTCharacterPiece>(Hit.GetActor()))
    {
        SelectPiece(ClickedPiece);
        return;
    }
    if (AVTTProp* ClickedProp = Cast<AVTTProp>(Hit.GetActor()))
    {
        SelectProp(ClickedProp);
        return;
    }

    FIntPoint Grid;
    FVector GridWorld;
    if (!GetCursorGrid(Grid, GridWorld))
    {
        return;
    }

    if (SelectedPiece && Board->IsCellActive(Grid))
    {
        PushUndoState();
        if (HasAuthority()) SelectedPiece->MoveToGridLocation(GridWorld);
        else ServerMoveActor(SelectedPiece, GridWorld);
    }
    else if (SelectedProp && Board->IsCellActive(Grid))
    {
        PushUndoState();
        if (HasAuthority()) SelectedProp->SetActorLocation(GridWorld);
        else ServerMoveActor(SelectedProp, GridWorld);
    }
}

void AVTTPlayerController::ServerMoveActor_Implementation(AActor* Actor, FVector NewLocation)
{
    if (AVTTCharacterPiece* Piece = Cast<AVTTCharacterPiece>(Actor))
    {
        Piece->MoveToGridLocation(NewLocation);
    }
    else if (AVTTProp* Prop = Cast<AVTTProp>(Actor))
    {
        Prop->SetActorLocation(NewLocation);
    }
}

void AVTTPlayerController::PrimaryRelease()
{
    bPainting = false;
    bHasLastPaintTarget = false;
}

void AVTTPlayerController::ApplyBuildAtCursor(bool bStartingStroke)
{
    if (!HasAuthority() || !Board)
    {
        return;
    }

    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit))
    {
        return;
    }

    FIntPoint Grid;
    if (!Board->WorldToGrid(Hit.ImpactPoint, Grid))
    {
        return;
    }

    if (BuildTool == EVTTBuildTool::Floor || BuildTool == EVTTBuildTool::Fog)
    {
        if (bHasLastPaintTarget && Grid == LastPaintCell)
        {
            return;
        }
        if (bStartingStroke)
        {
            bPaintAdd = BuildTool == EVTTBuildTool::Floor ? !Board->IsCellActive(Grid) : !Board->IsFogged(Grid);
        }
        if (BuildTool == EVTTBuildTool::Floor)
        {
            Board->SetCellActive(Grid, bPaintAdd);
        }
        else
        {
            Board->SetFogged(Grid, bPaintAdd);
        }
        LastPaintCell = Grid;
        bHasLastPaintTarget = true;
        return;
    }

    if (BuildTool == EVTTBuildTool::Wall || BuildTool == EVTTBuildTool::Door)
    {
        FIntVector Edge;
        if (!Board->FindNearestEdge(Hit.ImpactPoint, Edge) || (bHasLastPaintTarget && Edge == LastPaintEdge))
        {
            return;
        }
        if (bStartingStroke)
        {
            bPaintAdd = BuildTool == EVTTBuildTool::Wall ? !Board->HasWall(Edge) : !Board->HasDoor(Edge);
        }
        if (BuildTool == EVTTBuildTool::Wall)
        {
            Board->SetWall(Edge, bPaintAdd);
        }
        else
        {
            Board->SetDoor(Edge, bPaintAdd);
        }
        LastPaintEdge = Edge;
        bHasLastPaintTarget = true;
        return;
    }

    if (BuildTool == EVTTBuildTool::Prop && Board->IsCellActive(Grid))
    {
        SpawnPropAt(Board->GridToWorld(Grid), CurrentPropType);
    }
}

void AVTTPlayerController::SpawnHero()
{
    SpawnPieceAtCursor(FString::Printf(TEXT("Hero %d"), HeroCount++), 24, FLinearColor(0.12f, 0.48f, 1.0f));
}

void AVTTPlayerController::SpawnMonster()
{
    SpawnPieceAtCursor(FString::Printf(TEXT("Goblin %d"), MonsterCount++), 12, FLinearColor(0.20f, 0.75f, 0.18f));
}

void AVTTPlayerController::SpawnPieceAtCursor(const FString& PieceName, int32 MaxHP, const FLinearColor& Colour)
{
    if (!HasAuthority()) return;
    FIntPoint Grid;
    FVector GridWorld;
    if (!GetCursorGrid(Grid, GridWorld) || !Board->IsCellActive(Grid))
    {
        return;
    }

    FActorSpawnParameters Params;
    PushUndoState();
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AVTTCharacterPiece* Piece = GetWorld()->SpawnActor<AVTTCharacterPiece>(AVTTCharacterPiece::StaticClass(), GridWorld, FRotator::ZeroRotator, Params);
    if (Piece)
    {
        Piece->InitialisePiece(PieceName, MaxHP, Colour);
        SelectPiece(Piece);
    }
}

void AVTTPlayerController::DamageSelected()
{
    if (!HasAuthority()) return;
    if (SelectedPiece)
    {
        PushUndoState();
        SelectedPiece->ChangeHP(-1);
    }
}

void AVTTPlayerController::HealSelected()
{
    if (!HasAuthority()) return;
    if (SelectedPiece)
    {
        PushUndoState();
        SelectedPiece->ChangeHP(1);
    }
}

void AVTTPlayerController::DeleteSelected()
{
    if (!HasAuthority()) return;
    if (SelectedPiece)
    {
        PushUndoState();
        AVTTCharacterPiece* PieceToDelete = SelectedPiece;
        SelectedPiece = nullptr;
        PieceToDelete->Destroy();
    }
    else if (SelectedProp)
    {
        PushUndoState();
        AVTTProp* PropToDelete = SelectedProp;
        SelectedProp = nullptr;
        PropToDelete->Destroy();
    }
}

void AVTTPlayerController::RotateSelected()
{
    if (!HasAuthority()) return;
    if (!SelectedPiece && !SelectedProp)
    {
        return;
    }
    PushUndoState();
    if (SelectedProp)
    {
        SelectedProp->RotateClockwise();
    }
    else
    {
        SelectedPiece->AddActorWorldRotation(FRotator(0.0f, 45.0f, 0.0f));
    }
}

void AVTTPlayerController::DuplicateSelected()
{
    if (!HasAuthority()) return;
    if (!SelectedPiece && !SelectedProp)
    {
        return;
    }
    PushUndoState();
    if (SelectedProp)
    {
        SpawnPropAt(SelectedProp->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f), SelectedProp->PropType, SelectedProp->GetActorRotation());
    }
    else
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AVTTCharacterPiece* Piece = GetWorld()->SpawnActor<AVTTCharacterPiece>(AVTTCharacterPiece::StaticClass(),
            SelectedPiece->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f), SelectedPiece->GetActorRotation(), Params);
        if (Piece)
        {
            Piece->InitialisePiece(SelectedPiece->DisplayName + TEXT(" Copy"), SelectedPiece->MaxHP, SelectedPiece->PieceColour);
            Piece->ChangeHP(SelectedPiece->CurrentHP - SelectedPiece->MaxHP);
            Piece->SetSizeSquares(SelectedPiece->SizeSquares);
            Piece->ConditionText = SelectedPiece->ConditionText;
            Piece->bHiddenFromPlayers = SelectedPiece->bHiddenFromPlayers;
            Piece->SetVisualType(SelectedPiece->VisualType);
            Piece->ChangeHP(0);
            SelectPiece(Piece);
        }
    }
}

void AVTTPlayerController::ToggleBuildMode()
{
    if (!HasAuthority()) return;
    bBuildMode = !bBuildMode;
    if (GEngine)
    {
        const FString ModeText = GetBuildToolName();
        GEngine->AddOnScreenDebugMessage(21, 3.0f, bBuildMode ? FColor::Cyan : FColor::White,
            bBuildMode ? FString::Printf(TEXT("BUILD MODE - %s TOOL: click to add/remove, T changes tool"), *ModeText)
                       : TEXT("PLAY MODE: click pieces, then click a tile to move"));
    }
}

void AVTTPlayerController::CycleBuildTool()
{
    const int32 NextTool = (static_cast<int32>(BuildTool) + 1) % 5;
    BuildTool = static_cast<EVTTBuildTool>(NextTool);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(22, 3.0f, FColor::Cyan,
            FString::Printf(TEXT("BUILD TOOL: %s"), *GetBuildToolName()));
    }
}

void AVTTPlayerController::CycleProp()
{
    const int32 NextProp = (static_cast<int32>(CurrentPropType) + 1) % 5;
    CurrentPropType = static_cast<EVTTPropType>(NextProp);
    if (GEngine)
    {
        const UEnum* PropEnum = StaticEnum<EVTTPropType>();
        const FString PropName = PropEnum ? PropEnum->GetNameStringByValue(static_cast<int64>(CurrentPropType)) : TEXT("Prop");
        GEngine->AddOnScreenDebugMessage(26, 3.0f, FColor::Cyan, FString::Printf(TEXT("PROP: %s"), *PropName));
    }
}

void AVTTPlayerController::CycleSelectedSize()
{
    if (!HasAuthority()) return;
    if (SelectedPiece)
    {
        PushUndoState();
        SelectedPiece->CycleSize();
    }
}

void AVTTPlayerController::CycleSelectedCondition()
{
    if (!HasAuthority()) return;
    if (SelectedPiece)
    {
        PushUndoState();
        SelectedPiece->CycleCondition();
    }
}

void AVTTPlayerController::ToggleSelectedHidden()
{
    if (!HasAuthority()) return;
    if (SelectedPiece)
    {
        PushUndoState();
        SelectedPiece->ToggleHiddenFromPlayers();
    }
}

void AVTTPlayerController::CycleSelectedVisual()
{
    if (!HasAuthority()) return;
    if (SelectedPiece)
    {
        PushUndoState();
        SelectedPiece->CycleVisualType();
    }
}

void AVTTPlayerController::ToggleMeasure()
{
    bMeasureMode = !bMeasureMode;
    bHasMeasureStart = false;
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(29, 3.0f, FColor::Cyan,
            bMeasureMode ? TEXT("RULER: click start and end points") : TEXT("RULER OFF"));
    }
}

void AVTTPlayerController::HandleMeasureClick(const FVector& WorldLocation)
{
    if (!bHasMeasureStart)
    {
        MeasureStart = WorldLocation;
        bHasMeasureStart = true;
        return;
    }

    const FVector End = WorldLocation;
    const float TileSize = Board ? Board->TileSize : 100.0f;
    const float DistanceFeet = FVector::Dist2D(MeasureStart, End) / TileSize * 5.0f;
    DrawDebugLine(GetWorld(), MeasureStart + FVector(0.0f, 0.0f, 12.0f), End + FVector(0.0f, 0.0f, 12.0f),
        FColor::Cyan, false, 8.0f, 0, 5.0f);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(30, 8.0f, FColor::Cyan,
            FString::Printf(TEXT("DISTANCE: %.1f ft"), DistanceFeet));
    }
    MeasureStart = End;
}

void AVTTPlayerController::RollD20()
{
    const int32 Roll = FMath::RandRange(1, 20);
    if (GEngine)
    {
        const FColor Colour = Roll == 20 ? FColor::Green : Roll == 1 ? FColor::Red : FColor::White;
        GEngine->AddOnScreenDebugMessage(31, 5.0f, Colour, FString::Printf(TEXT("D20: %d"), Roll));
    }
}

void AVTTPlayerController::AddSelectedToInitiative()
{
    if (!SelectedPiece)
    {
        return;
    }

    Initiative.RemoveAll([this](const FVTTInitiativeEntry& Entry)
    {
        return !Entry.Piece.IsValid() || Entry.Piece.Get() == SelectedPiece;
    });
    FVTTInitiativeEntry Entry;
    Entry.Piece = SelectedPiece;
    Entry.Roll = FMath::RandRange(1, 20);
    Initiative.Add(Entry);
    Initiative.Sort([](const FVTTInitiativeEntry& A, const FVTTInitiativeEntry& B)
    {
        return A.Roll > B.Roll;
    });
    InitiativeIndex = 0;
    ShowInitiative();
}

void AVTTPlayerController::NextInitiativeTurn()
{
    Initiative.RemoveAll([](const FVTTInitiativeEntry& Entry){ return !Entry.Piece.IsValid(); });
    if (Initiative.IsEmpty())
    {
        return;
    }
    InitiativeIndex = (InitiativeIndex + 1) % Initiative.Num();
    ShowInitiative();
}

void AVTTPlayerController::ShowInitiative()
{
    if (!GEngine || Initiative.IsEmpty())
    {
        return;
    }
    FString Display = TEXT("INITIATIVE\n");
    for (int32 Index = 0; Index < Initiative.Num(); ++Index)
    {
        if (const AVTTCharacterPiece* Piece = Initiative[Index].Piece.Get())
        {
            Display += FString::Printf(TEXT("%s %s — %d\n"), Index == InitiativeIndex ? TEXT(">") : TEXT(" "),
                *Piece->DisplayName, Initiative[Index].Roll);
        }
    }
    GEngine->AddOnScreenDebugMessage(32, 10.0f, FColor(255, 210, 80), Display);
}

void AVTTPlayerController::SaveMap()
{
    WriteSaveSlot(true);
}

void AVTTPlayerController::WriteSaveSlot(bool bShowMessage)
{
    if (!HasAuthority() || !Board)
    {
        return;
    }

    UVTTSaveGame* SaveData = Cast<UVTTSaveGame>(UGameplayStatics::CreateSaveGameObject(UVTTSaveGame::StaticClass()));
    if (!SaveData)
    {
        return;
    }

    SaveData->Map = CaptureSnapshot();

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveData, GetSaveSlotName(), 0);
    if (bShowMessage && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(23, 3.0f, bSaved ? FColor::Green : FColor::Red,
            bSaved ? TEXT("MAP SAVED") : TEXT("MAP SAVE FAILED"));
    }
}

void AVTTPlayerController::LoadMap()
{
    if (!HasAuthority()) return;
    UVTTSaveGame* SaveData = Cast<UVTTSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSaveSlotName(), 0));
    if (!Board || !SaveData)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(24, 3.0f, FColor::Yellow, TEXT("NO SAVED MAP FOUND"));
        }
        return;
    }

    PushUndoState();
    ApplySnapshot(SaveData->Map);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(25, 3.0f, FColor::Green, TEXT("MAP LOADED"));
    }
}

void AVTTPlayerController::PreviousScene()
{
    SwitchScene(CurrentSceneIndex <= 1 ? 9 : CurrentSceneIndex - 1);
}

void AVTTPlayerController::NextScene()
{
    SwitchScene(CurrentSceneIndex >= 9 ? 1 : CurrentSceneIndex + 1);
}

void AVTTPlayerController::SwitchScene(int32 NewSceneIndex)
{
    if (!HasAuthority()) return;
    WriteSaveSlot(false);
    CurrentSceneIndex = FMath::Clamp(NewSceneIndex, 1, 9);
    UndoStack.Empty();
    RedoStack.Empty();
    Initiative.Empty();
    InitiativeIndex = 0;
    bAutosavePending = false;

    if (UGameplayStatics::DoesSaveGameExist(GetSaveSlotName(), 0))
    {
        if (UVTTSaveGame* SaveData = Cast<UVTTSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSaveSlotName(), 0)))
        {
            ApplySnapshot(SaveData->Map);
        }
    }
    else
    {
        ApplySnapshot(DefaultSceneSnapshot);
    }
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(33, 3.0f, FColor::Green,
            FString::Printf(TEXT("SCENE %d"), CurrentSceneIndex));
    }
}

void AVTTPlayerController::HostSession()
{
    WriteSaveSlot(false);
    UGameplayStatics::OpenLevel(this, FName(TEXT("/Engine/Maps/Entry")), true, TEXT("listen"));
}

void AVTTPlayerController::JoinSession()
{
    if (!JoinAddress.IsEmpty())
    {
        ClientTravel(JoinAddress, TRAVEL_Absolute);
    }
}

void AVTTPlayerController::Undo()
{
    if (!HasAuthority()) return;
    if (UndoStack.IsEmpty())
    {
        return;
    }
    RedoStack.Add(CaptureSnapshot());
    const FVTTMapSnapshot Snapshot = UndoStack.Last();
    UndoStack.RemoveAt(UndoStack.Num() - 1);
    ApplySnapshot(Snapshot);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(27, 2.0f, FColor::Cyan, TEXT("UNDO"));
    }
}

void AVTTPlayerController::Redo()
{
    if (!HasAuthority()) return;
    if (RedoStack.IsEmpty())
    {
        return;
    }
    UndoStack.Add(CaptureSnapshot());
    const FVTTMapSnapshot Snapshot = RedoStack.Last();
    RedoStack.RemoveAt(RedoStack.Num() - 1);
    ApplySnapshot(Snapshot);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(28, 2.0f, FColor::Cyan, TEXT("REDO"));
    }
}

bool AVTTPlayerController::GetCursorGrid(FIntPoint& OutGrid, FVector& OutWorld) const
{
    if (!Board)
    {
        return false;
    }

    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit) || !Board->WorldToGrid(Hit.ImpactPoint, OutGrid))
    {
        return false;
    }

    OutWorld = Board->GridToWorld(OutGrid);
    return true;
}

void AVTTPlayerController::SelectPiece(AVTTCharacterPiece* NewSelection)
{
    if (SelectedPiece)
    {
        SelectedPiece->SetSelected(false);
    }

    SelectedPiece = NewSelection;
    if (SelectedPiece)
    {
        if (SelectedProp)
        {
            SelectedProp->SetSelected(false);
            SelectedProp = nullptr;
        }
        SelectedPiece->SetSelected(true);
    }
}

void AVTTPlayerController::SelectProp(AVTTProp* NewSelection)
{
    if (SelectedProp)
    {
        SelectedProp->SetSelected(false);
    }
    SelectedProp = NewSelection;
    if (SelectedProp)
    {
        if (SelectedPiece)
        {
            SelectedPiece->SetSelected(false);
            SelectedPiece = nullptr;
        }
        SelectedProp->SetSelected(true);
    }
}

void AVTTPlayerController::SpawnPropAt(const FVector& WorldLocation, EVTTPropType PropType, const FRotator& Rotation)
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AVTTProp* Prop = GetWorld()->SpawnActor<AVTTProp>(AVTTProp::StaticClass(), WorldLocation, Rotation, Params);
    if (Prop)
    {
        Prop->InitialiseProp(PropType);
        SelectProp(Prop);
    }
}

FVTTMapSnapshot AVTTPlayerController::CaptureSnapshot() const
{
    FVTTMapSnapshot Snapshot;
    if (!Board)
    {
        return Snapshot;
    }

    Snapshot.ActiveCells = Board->GetActiveCells();
    Snapshot.WallEdges = Board->GetWallEdges();
    Snapshot.DoorEdges = Board->GetDoorEdges();
    Snapshot.FoggedCells = Board->GetFoggedCells();

    for (TActorIterator<AVTTCharacterPiece> It(GetWorld()); It; ++It)
    {
        FVTTPieceSaveData PieceData;
        PieceData.DisplayName = It->DisplayName;
        PieceData.MaxHP = It->MaxHP;
        PieceData.CurrentHP = It->CurrentHP;
        PieceData.Colour = It->PieceColour;
        PieceData.Location = It->GetActorLocation();
        PieceData.Rotation = It->GetActorRotation();
        PieceData.SizeSquares = It->SizeSquares;
        PieceData.ConditionText = It->ConditionText;
        PieceData.bHiddenFromPlayers = It->bHiddenFromPlayers;
        PieceData.VisualType = static_cast<uint8>(It->VisualType);
        Snapshot.Pieces.Add(PieceData);
    }

    for (TActorIterator<AVTTProp> It(GetWorld()); It; ++It)
    {
        FVTTPropSaveData PropData;
        PropData.PropType = static_cast<uint8>(It->PropType);
        PropData.Location = It->GetActorLocation();
        PropData.Rotation = It->GetActorRotation();
        Snapshot.Props.Add(PropData);
    }
    return Snapshot;
}

void AVTTPlayerController::ApplySnapshot(const FVTTMapSnapshot& Snapshot)
{
    if (!Board)
    {
        return;
    }

    SelectPiece(nullptr);
    SelectProp(nullptr);
    Initiative.Empty();
    InitiativeIndex = 0;
    Board->ApplyLayout(Snapshot.ActiveCells, Snapshot.WallEdges, Snapshot.DoorEdges, Snapshot.FoggedCells);

    TArray<AActor*> ExistingActors;
    for (TActorIterator<AVTTCharacterPiece> It(GetWorld()); It; ++It)
    {
        ExistingActors.Add(*It);
    }
    for (TActorIterator<AVTTProp> It(GetWorld()); It; ++It)
    {
        ExistingActors.Add(*It);
    }
    for (AActor* Actor : ExistingActors)
    {
        Actor->Destroy();
    }

    for (const FVTTPieceSaveData& PieceData : Snapshot.Pieces)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AVTTCharacterPiece* Piece = GetWorld()->SpawnActor<AVTTCharacterPiece>(AVTTCharacterPiece::StaticClass(), PieceData.Location, PieceData.Rotation, Params);
        if (Piece)
        {
            Piece->InitialisePiece(PieceData.DisplayName, PieceData.MaxHP, PieceData.Colour);
            Piece->ChangeHP(PieceData.CurrentHP - PieceData.MaxHP);
            Piece->SetSizeSquares(PieceData.SizeSquares);
            Piece->ConditionText = PieceData.ConditionText;
            Piece->bHiddenFromPlayers = PieceData.bHiddenFromPlayers;
            Piece->SetVisualType(static_cast<EVTTVisualType>(FMath::Min<uint8>(PieceData.VisualType, 10)));
            Piece->ChangeHP(0);
        }
    }

    for (const FVTTPropSaveData& PropData : Snapshot.Props)
    {
        const uint8 SafeType = FMath::Min<uint8>(PropData.PropType, 4);
        SpawnPropAt(PropData.Location, static_cast<EVTTPropType>(SafeType), PropData.Rotation);
    }
    SelectProp(nullptr);
}

void AVTTPlayerController::PushUndoState()
{
    if (!HasAuthority()) return;
    UndoStack.Add(CaptureSnapshot());
    if (UndoStack.Num() > 50)
    {
        UndoStack.RemoveAt(0);
    }
    RedoStack.Empty();
    if (GetWorld())
    {
        bAutosavePending = true;
        AutosaveAtTime = GetWorld()->GetTimeSeconds() + 2.0f;
    }
}

FString AVTTPlayerController::GetBuildToolName() const
{
    switch (BuildTool)
    {
    case EVTTBuildTool::Wall: return TEXT("WALL");
    case EVTTBuildTool::Door: return TEXT("DOOR");
    case EVTTBuildTool::Prop: return TEXT("PROP");
    case EVTTBuildTool::Fog: return TEXT("FOG");
    default: return TEXT("FLOOR");
    }
}

FString AVTTPlayerController::GetToolbarStatus() const
{
    FString Selection = TEXT("Nothing selected");
    if (SelectedPiece)
    {
        Selection = FString::Printf(TEXT("%s | %d/%d HP | Size %d"), *SelectedPiece->DisplayName,
            SelectedPiece->CurrentHP, SelectedPiece->MaxHP, SelectedPiece->SizeSquares);
    }
    else if (SelectedProp)
    {
        Selection = SelectedProp->GetPropName();
    }

    if (!bBuildMode)
    {
        return FString::Printf(TEXT("SCENE %d  |  PLAY MODE  |  %s"), CurrentSceneIndex, *Selection);
    }

    FString Extra;
    if (BuildTool == EVTTBuildTool::Prop)
    {
        if (const UEnum* PropEnum = StaticEnum<EVTTPropType>())
        {
            Extra = TEXT("  |  ") + PropEnum->GetNameStringByValue(static_cast<int64>(CurrentPropType));
        }
    }
    return FString::Printf(TEXT("SCENE %d  |  BUILD: %s%s  |  %s"), CurrentSceneIndex, *GetBuildToolName(), *Extra, *Selection);
}

FString AVTTPlayerController::GetSaveSlotName() const
{
    return SaveSlotPrefix + FString::FromInt(CurrentSceneIndex);
}

void AVTTPlayerController::SetBuildTool(EVTTBuildTool NewTool)
{
    if (!HasAuthority()) return;
    BuildTool = NewTool;
    bBuildMode = true;
    bPainting = false;
    bHasLastPaintTarget = false;
}

void AVTTPlayerController::BuildToolbar()
{
    if (!GEngine || !GEngine->GameViewport || ToolbarWidget.IsValid())
    {
        return;
    }

    TWeakObjectPtr<AVTTPlayerController> WeakThis(this);
    auto MakeButton = [WeakThis](const FString& Label, TFunction<void(AVTTPlayerController*)> Callback) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .IsFocusable(false)
            .ContentPadding(FMargin(8.0f, 4.0f))
            .OnClicked_Lambda([WeakThis, Callback]()
            {
                if (AVTTPlayerController* Controller = WeakThis.Get())
                {
                    Callback(Controller);
                }
                return FReply::Handled();
            })
            [
                SNew(STextBlock).Text(FText::FromString(Label))
            ];
    };

    ToolbarWidget = SNew(SBox)
        .Visibility(EVisibility::SelfHitTestInvisible)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Top)
        .Padding(FMargin(0.0f, 12.0f))
        [
            SNew(SBorder)
            .BorderBackgroundColor(FLinearColor(0.018f, 0.022f, 0.03f, 0.96f))
            .Padding(FMargin(8.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4.0f)
                [
                    SNew(STextBlock)
                    .ColorAndOpacity(FLinearColor(0.82f, 0.88f, 1.0f))
                    .Text_Lambda([WeakThis]()
                    {
                        const AVTTPlayerController* Controller = WeakThis.Get();
                        return FText::FromString(Controller ? Controller->GetToolbarStatus() : TEXT("Owlbear 3D"));
                    })
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SUniformGridPanel).SlotPadding(FMargin(2.0f))
                    + SUniformGridPanel::Slot(0, 0)[MakeButton(TEXT("PLAY / BUILD"), [](AVTTPlayerController* C){ C->ToggleBuildMode(); })]
                    + SUniformGridPanel::Slot(1, 0)[MakeButton(TEXT("FLOOR"), [](AVTTPlayerController* C){ C->SetBuildTool(EVTTBuildTool::Floor); })]
                    + SUniformGridPanel::Slot(2, 0)[MakeButton(TEXT("WALL"), [](AVTTPlayerController* C){ C->SetBuildTool(EVTTBuildTool::Wall); })]
                    + SUniformGridPanel::Slot(3, 0)[MakeButton(TEXT("DOOR"), [](AVTTPlayerController* C){ C->SetBuildTool(EVTTBuildTool::Door); })]
                    + SUniformGridPanel::Slot(4, 0)[MakeButton(TEXT("PROP"), [](AVTTPlayerController* C){ C->SetBuildTool(EVTTBuildTool::Prop); })]
                    + SUniformGridPanel::Slot(5, 0)[MakeButton(TEXT("FOG"), [](AVTTPlayerController* C){ C->SetBuildTool(EVTTBuildTool::Fog); })]
                    + SUniformGridPanel::Slot(0, 1)[MakeButton(TEXT("NEXT PROP"), [](AVTTPlayerController* C){ C->CycleProp(); })]
                    + SUniformGridPanel::Slot(1, 1)[MakeButton(TEXT("ROTATE"), [](AVTTPlayerController* C){ C->RotateSelected(); })]
                    + SUniformGridPanel::Slot(2, 1)[MakeButton(TEXT("COPY"), [](AVTTPlayerController* C){ C->DuplicateSelected(); })]
                    + SUniformGridPanel::Slot(3, 1)[MakeButton(TEXT("UNDO"), [](AVTTPlayerController* C){ C->Undo(); })]
                    + SUniformGridPanel::Slot(4, 1)[MakeButton(TEXT("REDO"), [](AVTTPlayerController* C){ C->Redo(); })]
                    + SUniformGridPanel::Slot(5, 1)[MakeButton(TEXT("SAVE"), [](AVTTPlayerController* C){ C->SaveMap(); })]
                    + SUniformGridPanel::Slot(6, 1)[MakeButton(TEXT("LOAD"), [](AVTTPlayerController* C){ C->LoadMap(); })]
                    + SUniformGridPanel::Slot(0, 2)[MakeButton(TEXT("HP -"), [](AVTTPlayerController* C){ C->DamageSelected(); })]
                    + SUniformGridPanel::Slot(1, 2)[MakeButton(TEXT("HP +"), [](AVTTPlayerController* C){ C->HealSelected(); })]
                    + SUniformGridPanel::Slot(2, 2)[MakeButton(TEXT("SIZE"), [](AVTTPlayerController* C){ C->CycleSelectedSize(); })]
                    + SUniformGridPanel::Slot(3, 2)[MakeButton(TEXT("CONDITION"), [](AVTTPlayerController* C){ C->CycleSelectedCondition(); })]
                    + SUniformGridPanel::Slot(4, 2)[MakeButton(TEXT("HIDE"), [](AVTTPlayerController* C){ C->ToggleSelectedHidden(); })]
                    + SUniformGridPanel::Slot(5, 2)[MakeButton(TEXT("DELETE"), [](AVTTPlayerController* C){ C->DeleteSelected(); })]
                    + SUniformGridPanel::Slot(5, 3)[MakeButton(TEXT("MODEL"), [](AVTTPlayerController* C){ C->CycleSelectedVisual(); })]
                    + SUniformGridPanel::Slot(6, 2)[MakeButton(TEXT("RULER"), [](AVTTPlayerController* C){ C->ToggleMeasure(); })]
                    + SUniformGridPanel::Slot(7, 2)[MakeButton(TEXT("D20"), [](AVTTPlayerController* C){ C->RollD20(); })]
                    + SUniformGridPanel::Slot(6, 0)[MakeButton(TEXT("INIT"), [](AVTTPlayerController* C){ C->AddSelectedToInitiative(); })]
                    + SUniformGridPanel::Slot(7, 0)[MakeButton(TEXT("NEXT TURN"), [](AVTTPlayerController* C){ C->NextInitiativeTurn(); })]
                    + SUniformGridPanel::Slot(6, 3)[MakeButton(TEXT("< SCENE"), [](AVTTPlayerController* C){ C->PreviousScene(); })]
                    + SUniformGridPanel::Slot(7, 3)[MakeButton(TEXT("SCENE >"), [](AVTTPlayerController* C){ C->NextScene(); })]
                    + SUniformGridPanel::Slot(0, 3)[MakeButton(TEXT("HOST"), [](AVTTPlayerController* C){ C->HostSession(); })]
                    + SUniformGridPanel::Slot(1, 3)[MakeButton(TEXT("JOIN"), [](AVTTPlayerController* C){ C->JoinSession(); })]
                    + SUniformGridPanel::Slot(2, 3)
                    [
                        SNew(SEditableTextBox)
                        .Text_Lambda([WeakThis]()
                        {
                            const AVTTPlayerController* Controller = WeakThis.Get();
                            return FText::FromString(Controller ? Controller->JoinAddress : TEXT("127.0.0.1"));
                        })
                        .OnTextChanged_Lambda([WeakThis](const FText& NewText)
                        {
                            if (AVTTPlayerController* Controller = WeakThis.Get())
                            {
                                Controller->JoinAddress = NewText.ToString();
                            }
                        })
                    ]
                ]
            ]
        ];

    GEngine->GameViewport->AddViewportWidgetContent(ToolbarWidget.ToSharedRef(), 100);
}

void AVTTPlayerController::RemoveToolbar()
{
    if (GEngine && GEngine->GameViewport && ToolbarWidget.IsValid())
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(ToolbarWidget.ToSharedRef());
    }
    ToolbarWidget.Reset();
}

void AVTTPlayerController::ShowControls() const
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(10, 20.0f, FColor::White,
            TEXT("LMB select/paint | B/T build tools | P prop | R rotate | C copy | K size | N condition | H hide | Z/Y undo/redo | F5/F9"));
    }
}
