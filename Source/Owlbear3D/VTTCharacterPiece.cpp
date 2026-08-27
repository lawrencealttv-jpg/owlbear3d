#include "VTTCharacterPiece.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AVTTCharacterPiece::AVTTCharacterPiece()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    HitCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HitCapsule"));
    HitCapsule->SetupAttachment(SceneRoot);
    HitCapsule->InitCapsuleSize(34.0f, 72.0f);
    HitCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 72.0f));
    HitCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitCapsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
    BaseMesh->SetupAttachment(SceneRoot);
    BaseMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 4.0f));
    BaseMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.08f));
    BaseMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(SceneRoot);
    BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
    BodyMesh->SetRelativeScale3D(FVector(0.34f, 0.26f, 0.72f));
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(SceneRoot);
    HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 122.0f));
    HeadMesh->SetRelativeScale3D(FVector(0.32f));
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SelectionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionMesh"));
    SelectionMesh->SetupAttachment(SceneRoot);
    SelectionMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 1.5f));
    SelectionMesh->SetRelativeScale3D(FVector(0.52f, 0.52f, 0.025f));
    SelectionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionMesh->SetVisibility(false);

    if (CylinderMesh.Succeeded())
    {
        BaseMesh->SetStaticMesh(CylinderMesh.Object);
        SelectionMesh->SetStaticMesh(CylinderMesh.Object);
    }
    if (CubeMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CubeMesh.Object);
    }
    if (SphereMesh.Succeeded())
    {
        HeadMesh->SetStaticMesh(SphereMesh.Object);
    }

    NameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameText"));
    NameText->SetupAttachment(SceneRoot);
    NameText->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
    NameText->SetHorizontalAlignment(EHTA_Center);
    NameText->SetWorldSize(26.0f);
    NameText->SetTextRenderColor(FColor::White);

    HPText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HPText"));
    HPText->SetupAttachment(SceneRoot);
    HPText->SetRelativeLocation(FVector(0.0f, 0.0f, 152.0f));
    HPText->SetHorizontalAlignment(EHTA_Center);
    HPText->SetWorldSize(22.0f);
    HPText->SetTextRenderColor(FColor(100, 255, 120));

    TargetLocation = FVector::ZeroVector;
}

void AVTTCharacterPiece::InitialisePiece(const FString& NewName, int32 NewMaxHP, const FLinearColor& NewColour)
{
    DisplayName = NewName;
    MaxHP = FMath::Max(1, NewMaxHP);
    CurrentHP = MaxHP;
    ApplyColour(NewColour);
    UpdateLabels();
}

void AVTTCharacterPiece::SetSelected(bool bNewSelected)
{
    SelectionMesh->SetVisibility(bNewSelected);
}

void AVTTCharacterPiece::MoveToGridLocation(const FVector& NewLocation)
{
    TargetLocation = NewLocation;
    bMoving = true;
}

void AVTTCharacterPiece::ChangeHP(int32 Delta)
{
    CurrentHP = FMath::Clamp(CurrentHP + Delta, 0, MaxHP);
    UpdateLabels();
}

void AVTTCharacterPiece::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bMoving)
    {
        const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), TargetLocation, DeltaSeconds, 650.0f);
        SetActorLocation(NewLocation);
        if (NewLocation.Equals(TargetLocation, 1.0f))
        {
            SetActorLocation(TargetLocation);
            bMoving = false;
        }
    }

    if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        const FRotator FaceCamera = (CameraManager->GetCameraLocation() - NameText->GetComponentLocation()).Rotation();
        NameText->SetWorldRotation(FaceCamera);
        HPText->SetWorldRotation(FaceCamera);
    }
}

void AVTTCharacterPiece::UpdateLabels()
{
    NameText->SetText(FText::FromString(DisplayName));
    HPText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d HP"), CurrentHP, MaxHP)));

    const float Ratio = MaxHP > 0 ? static_cast<float>(CurrentHP) / static_cast<float>(MaxHP) : 0.0f;
    HPText->SetTextRenderColor(Ratio > 0.5f ? FColor(100, 255, 120) : Ratio > 0.2f ? FColor::Yellow : FColor(255, 80, 80));
}

void AVTTCharacterPiece::ApplyColour(const FLinearColor& NewColour)
{
    for (UStaticMeshComponent* Mesh : { BodyMesh.Get(), HeadMesh.Get(), BaseMesh.Get() })
    {
        if (Mesh && Mesh->GetMaterial(0))
        {
            UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0);
            if (DynamicMaterial)
            {
                DynamicMaterial->SetVectorParameterValue(TEXT("Color"), NewColour);
            }
        }
    }
}
