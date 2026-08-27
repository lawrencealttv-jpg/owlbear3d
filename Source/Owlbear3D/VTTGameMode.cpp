#include "VTTGameMode.h"

#include "VTTBoard.h"
#include "VTTCameraPawn.h"
#include "VTTCharacterPiece.h"
#include "VTTPlayerController.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AVTTGameMode::AVTTGameMode()
{
    DefaultPawnClass = AVTTCameraPawn::StaticClass();
    PlayerControllerClass = AVTTPlayerController::StaticClass();
}

void AVTTGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* LocalController = GetWorld()->GetFirstPlayerController())
    {
        if (!LocalController->GetPawn())
        {
            AVTTCameraPawn* CameraPawn = GetWorld()->SpawnActor<AVTTCameraPawn>(AVTTCameraPawn::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
            if (CameraPawn)
            {
                LocalController->Possess(CameraPawn);
            }
        }
    }

    AVTTBoard* Board = GetWorld()->SpawnActor<AVTTBoard>(AVTTBoard::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Board)
    {
        return;
    }

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.0f, 0.0f, 1000.0f), FRotator(-55.0f, -35.0f, 0.0f));
    if (Sun && Sun->GetLightComponent())
    {
        Sun->GetLightComponent()->SetIntensity(4.0f);
        Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.88f, 0.72f));
    }

    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (Sky && Sky->GetLightComponent())
    {
        Sky->GetLightComponent()->SetIntensity(0.8f);
        Sky->GetLightComponent()->RecaptureSky();
    }

    auto SpawnPiece = [this, Board](const FIntPoint& Grid, const FString& Name, int32 HP, const FLinearColor& Colour)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AVTTCharacterPiece* Piece = GetWorld()->SpawnActor<AVTTCharacterPiece>(AVTTCharacterPiece::StaticClass(), Board->GridToWorld(Grid), FRotator::ZeroRotator, Params);
        if (Piece)
        {
            Piece->InitialisePiece(Name, HP, Colour);
        }
    };

    SpawnPiece(FIntPoint(5, 5), TEXT("Adventurer"), 24, FLinearColor(0.12f, 0.48f, 1.0f));
    SpawnPiece(FIntPoint(10, 7), TEXT("Goblin"), 12, FLinearColor(0.20f, 0.75f, 0.18f));
}
