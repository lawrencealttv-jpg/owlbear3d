#include "VTTPlayerController.h"

#include "VTTBoard.h"
#include "VTTCharacterPiece.h"
#include "VTTSaveGame.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    const FString SaveSlotName = TEXT("Owlbear3D_Autosave");
}

AVTTPlayerController::AVTTPlayerController()
{
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

    ShowControls();
}

void AVTTPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction(TEXT("PrimaryClick"), IE_Pressed, this, &AVTTPlayerController::PrimaryClick);
    InputComponent->BindAction(TEXT("SpawnHero"), IE_Pressed, this, &AVTTPlayerController::SpawnHero);
    InputComponent->BindAction(TEXT("SpawnMonster"), IE_Pressed, this, &AVTTPlayerController::SpawnMonster);
    InputComponent->BindAction(TEXT("DamagePiece"), IE_Pressed, this, &AVTTPlayerController::DamageSelected);
    InputComponent->BindAction(TEXT("HealPiece"), IE_Pressed, this, &AVTTPlayerController::HealSelected);
    InputComponent->BindAction(TEXT("DeletePiece"), IE_Pressed, this, &AVTTPlayerController::DeleteSelected);
    InputComponent->BindAction(TEXT("ToggleBuildMode"), IE_Pressed, this, &AVTTPlayerController::ToggleBuildMode);
    InputComponent->BindAction(TEXT("CycleBuildTool"), IE_Pressed, this, &AVTTPlayerController::CycleBuildTool);
    InputComponent->BindAction(TEXT("SaveMap"), IE_Pressed, this, &AVTTPlayerController::SaveMap);
    InputComponent->BindAction(TEXT("LoadMap"), IE_Pressed, this, &AVTTPlayerController::LoadMap);
}

void AVTTPlayerController::PrimaryClick()
{
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit))
    {
        return;
    }

    if (!bBuildMode)
    {
        if (AVTTCharacterPiece* ClickedPiece = Cast<AVTTCharacterPiece>(Hit.GetActor()))
        {
            SelectPiece(ClickedPiece);
            return;
        }
    }

    if (bBuildMode && BuildTool == EVTTBuildTool::Wall)
    {
        if (Board)
        {
            Board->ToggleWallAtWorldLocation(Hit.ImpactPoint);
        }
        return;
    }

    FIntPoint Grid;
    FVector GridWorld;
    if (!GetCursorGrid(Grid, GridWorld))
    {
        return;
    }

    if (bBuildMode)
    {
        Board->ToggleCell(Grid);
        return;
    }

    if (SelectedPiece && Board->IsCellActive(Grid))
    {
        SelectedPiece->MoveToGridLocation(GridWorld);
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
    FIntPoint Grid;
    FVector GridWorld;
    if (!GetCursorGrid(Grid, GridWorld) || !Board->IsCellActive(Grid))
    {
        return;
    }

    FActorSpawnParameters Params;
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
    if (SelectedPiece)
    {
        SelectedPiece->ChangeHP(-1);
    }
}

void AVTTPlayerController::HealSelected()
{
    if (SelectedPiece)
    {
        SelectedPiece->ChangeHP(1);
    }
}

void AVTTPlayerController::DeleteSelected()
{
    if (SelectedPiece)
    {
        AVTTCharacterPiece* PieceToDelete = SelectedPiece;
        SelectedPiece = nullptr;
        PieceToDelete->Destroy();
    }
}

void AVTTPlayerController::ToggleBuildMode()
{
    bBuildMode = !bBuildMode;
    if (GEngine)
    {
        const FString ModeText = BuildTool == EVTTBuildTool::Floor ? TEXT("FLOOR") : TEXT("WALL");
        GEngine->AddOnScreenDebugMessage(21, 3.0f, bBuildMode ? FColor::Cyan : FColor::White,
            bBuildMode ? FString::Printf(TEXT("BUILD MODE - %s TOOL: click to add/remove, T changes tool"), *ModeText)
                       : TEXT("PLAY MODE: click pieces, then click a tile to move"));
    }
}

void AVTTPlayerController::CycleBuildTool()
{
    BuildTool = BuildTool == EVTTBuildTool::Floor ? EVTTBuildTool::Wall : EVTTBuildTool::Floor;
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(22, 3.0f, FColor::Cyan,
            BuildTool == EVTTBuildTool::Floor ? TEXT("BUILD TOOL: FLOOR") : TEXT("BUILD TOOL: WALL - click near a tile edge"));
    }
}

void AVTTPlayerController::SaveMap()
{
    if (!Board)
    {
        return;
    }

    UVTTSaveGame* SaveData = Cast<UVTTSaveGame>(UGameplayStatics::CreateSaveGameObject(UVTTSaveGame::StaticClass()));
    if (!SaveData)
    {
        return;
    }

    SaveData->ActiveCells = Board->GetActiveCells();
    SaveData->WallEdges = Board->GetWallEdges();

    for (TActorIterator<AVTTCharacterPiece> It(GetWorld()); It; ++It)
    {
        FVTTPieceSaveData PieceData;
        PieceData.DisplayName = It->DisplayName;
        PieceData.MaxHP = It->MaxHP;
        PieceData.CurrentHP = It->CurrentHP;
        PieceData.Colour = It->PieceColour;
        PieceData.Location = It->GetActorLocation();
        SaveData->Pieces.Add(PieceData);
    }

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, 0);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(23, 3.0f, bSaved ? FColor::Green : FColor::Red,
            bSaved ? TEXT("MAP SAVED") : TEXT("MAP SAVE FAILED"));
    }
}

void AVTTPlayerController::LoadMap()
{
    UVTTSaveGame* SaveData = Cast<UVTTSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));
    if (!Board || !SaveData)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(24, 3.0f, FColor::Yellow, TEXT("NO SAVED MAP FOUND"));
        }
        return;
    }

    SelectPiece(nullptr);
    Board->ApplyLayout(SaveData->ActiveCells, SaveData->WallEdges);

    TArray<AVTTCharacterPiece*> ExistingPieces;
    for (TActorIterator<AVTTCharacterPiece> It(GetWorld()); It; ++It)
    {
        ExistingPieces.Add(*It);
    }
    for (AVTTCharacterPiece* Piece : ExistingPieces)
    {
        Piece->Destroy();
    }

    for (const FVTTPieceSaveData& PieceData : SaveData->Pieces)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AVTTCharacterPiece* Piece = GetWorld()->SpawnActor<AVTTCharacterPiece>(AVTTCharacterPiece::StaticClass(), PieceData.Location, FRotator::ZeroRotator, Params);
        if (Piece)
        {
            Piece->InitialisePiece(PieceData.DisplayName, PieceData.MaxHP, PieceData.Colour);
            Piece->ChangeHP(PieceData.CurrentHP - PieceData.MaxHP);
        }
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(25, 3.0f, FColor::Green, TEXT("MAP LOADED"));
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
        SelectedPiece->SetSelected(true);
    }
}

void AVTTPlayerController::ShowControls() const
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(10, 20.0f, FColor::White,
            TEXT("LMB select/move | 1/2 spawn | +/- HP | Delete | B build | T floor/wall | F5 save | F9 load | WASD/QE/Wheel camera"));
    }
}
