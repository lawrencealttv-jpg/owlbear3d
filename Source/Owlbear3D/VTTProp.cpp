#include "VTTProp.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AVTTProp::AVTTProp()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    HitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBox"));
    HitBox->SetupAttachment(SceneRoot);
    HitBox->SetBoxExtent(FVector(42.0f, 42.0f, 55.0f));
    HitBox->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
    HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HitBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    PropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropMesh"));
    PropMesh->SetupAttachment(HitBox);
    PropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SelectionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionMesh"));
    SelectionMesh->SetupAttachment(SceneRoot);
    SelectionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionMesh->SetVisibility(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
    CubeAsset = CubeFinder.Object;
    CylinderAsset = CylinderFinder.Object;
    ConeAsset = ConeFinder.Object;

    if (CylinderAsset)
    {
        SelectionMesh->SetStaticMesh(CylinderAsset);
        SelectionMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 1.5f));
        SelectionMesh->SetRelativeScale3D(FVector(0.52f, 0.52f, 0.025f));
    }

    InitialiseProp(EVTTPropType::Crate);
}

void AVTTProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVTTProp, PropType);
}

void AVTTProp::InitialiseProp(EVTTPropType NewType)
{
    PropType = NewType;
    PropMesh->SetRelativeRotation(FRotator::ZeroRotator);

    switch (PropType)
    {
    case EVTTPropType::Table:
        PropMesh->SetStaticMesh(CylinderAsset);
        PropMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -12.0f));
        PropMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 0.12f));
        HitBox->SetBoxExtent(FVector(48.0f, 48.0f, 40.0f));
        break;
    case EVTTPropType::Column:
        PropMesh->SetStaticMesh(CylinderAsset);
        PropMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
        PropMesh->SetRelativeScale3D(FVector(0.32f, 0.32f, 1.45f));
        HitBox->SetBoxExtent(FVector(35.0f, 35.0f, 95.0f));
        break;
    case EVTTPropType::Statue:
        PropMesh->SetStaticMesh(ConeAsset);
        PropMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));
        PropMesh->SetRelativeScale3D(FVector(0.38f, 0.38f, 1.15f));
        HitBox->SetBoxExtent(FVector(40.0f, 40.0f, 85.0f));
        break;
    case EVTTPropType::Chest:
        PropMesh->SetStaticMesh(CubeAsset);
        PropMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -30.0f));
        PropMesh->SetRelativeScale3D(FVector(0.58f, 0.36f, 0.30f));
        HitBox->SetBoxExtent(FVector(42.0f, 32.0f, 32.0f));
        break;
    case EVTTPropType::Crate:
    default:
        PropMesh->SetStaticMesh(CubeAsset);
        PropMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -18.0f));
        PropMesh->SetRelativeScale3D(FVector(0.62f, 0.62f, 0.62f));
        HitBox->SetBoxExtent(FVector(36.0f, 36.0f, 36.0f));
        break;
    }

    if (PropMesh->GetMaterial(0))
    {
        if (UMaterialInstanceDynamic* Material = PropMesh->CreateAndSetMaterialInstanceDynamic(0))
        {
            const bool bStone = PropType == EVTTPropType::Column || PropType == EVTTPropType::Statue;
            Material->SetVectorParameterValue(TEXT("Color"), bStone ? FLinearColor(0.38f, 0.38f, 0.42f)
                : FLinearColor(0.28f, 0.105f, 0.035f));
        }
    }
    ForceNetUpdate();
}

void AVTTProp::SetSelected(bool bNewSelected)
{
    SelectionMesh->SetVisibility(bNewSelected);
}

void AVTTProp::RotateClockwise()
{
    AddActorWorldRotation(FRotator(0.0f, 45.0f, 0.0f));
}

FString AVTTProp::GetPropName() const
{
    switch (PropType)
    {
    case EVTTPropType::Table: return TEXT("Table");
    case EVTTPropType::Column: return TEXT("Column");
    case EVTTPropType::Statue: return TEXT("Statue");
    case EVTTPropType::Chest: return TEXT("Chest");
    default: return TEXT("Crate");
    }
}

void AVTTProp::OnRep_PropType()
{
    InitialiseProp(PropType);
}
