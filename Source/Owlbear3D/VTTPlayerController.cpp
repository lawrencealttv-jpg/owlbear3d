#include "VTTPlayerController.h"

#include "VTTBoard.h"
#include "VTTCharacterPiece.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
}

void AVTTPlayerController::PrimaryClick()
{
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, true, Hit))
    {
        return;
    }

    if (AVTTCharacterPiece* ClickedPiece = Cast<AVTTCharacterPiece>(Hit.GetActor()))
    {
        SelectPiece(ClickedPiece);
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
        GEngine->AddOnScreenDebugMessage(21, 3.0f, bBuildMode ? FColor::Cyan : FColor::White,
            bBuildMode ? TEXT("BUILD MODE: click tiles to add/remove floor") : TEXT("PLAY MODE: click pieces, then click a tile to move"));
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
            TEXT("LMB select/move | 1 hero | 2 goblin | +/- HP | Delete | B build mode | WASD pan | Q/E rotate | Wheel zoom"));
    }
}
