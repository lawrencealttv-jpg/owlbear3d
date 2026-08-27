#include "VTTCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"

AVTTCameraPawn::AVTTCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
    bOnlyRelevantToOwner = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(SceneRoot);
    SpringArm->TargetArmLength = 1700.0f;
    SpringArm->SetRelativeRotation(FRotator(-52.0f, 0.0f, 0.0f));
    SpringArm->bDoCollisionTest = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void AVTTCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AVTTCameraPawn::SetForwardInput);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AVTTCameraPawn::SetRightInput);
    PlayerInputComponent->BindAxis(TEXT("Zoom"), this, &AVTTCameraPawn::Zoom);
    PlayerInputComponent->BindAction(TEXT("RotateLeft"), IE_Pressed, this, &AVTTCameraPawn::RotateLeft);
    PlayerInputComponent->BindAction(TEXT("RotateRight"), IE_Pressed, this, &AVTTCameraPawn::RotateRight);
}

void AVTTCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!FMath::IsNearlyZero(ForwardInput) || !FMath::IsNearlyZero(RightInput))
    {
        const FVector Forward = GetActorForwardVector();
        const FVector Right = GetActorRightVector();
        const FVector Offset = (Forward * ForwardInput + Right * RightInput).GetClampedToMaxSize(1.0f) * PanSpeed * DeltaSeconds;
        AddActorWorldOffset(Offset, false);
    }
}

void AVTTCameraPawn::SetForwardInput(float Value)
{
    ForwardInput = Value;
}

void AVTTCameraPawn::SetRightInput(float Value)
{
    RightInput = Value;
}

void AVTTCameraPawn::Zoom(float Value)
{
    if (!FMath::IsNearlyZero(Value))
    {
        SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength - Value * 180.0f, 550.0f, 2800.0f);
    }
}

void AVTTCameraPawn::RotateLeft()
{
    AddActorWorldRotation(FRotator(0.0f, -45.0f, 0.0f));
}

void AVTTCameraPawn::RotateRight()
{
    AddActorWorldRotation(FRotator(0.0f, 45.0f, 0.0f));
}
