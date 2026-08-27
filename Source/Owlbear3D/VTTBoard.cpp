#include "VTTBoard.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AVTTBoard::AVTTBoard()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    GridSurface = CreateDefaultSubobject<UBoxComponent>(TEXT("GridSurface"));
    GridSurface->SetupAttachment(SceneRoot);
    GridSurface->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GridSurface->SetCollisionResponseToAllChannels(ECR_Ignore);
    GridSurface->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GridSurface->SetHiddenInGame(true);

    FloorInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorInstances"));
    FloorInstances->SetupAttachment(SceneRoot);
    FloorInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    WallInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallInstances"));
    WallInstances->SetupAttachment(SceneRoot);
    WallInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        FloorInstances->SetStaticMesh(CubeMesh.Object);
        WallInstances->SetStaticMesh(CubeMesh.Object);
    }
}

void AVTTBoard::BeginPlay()
{
    Super::BeginPlay();

    GridSurface->SetBoxExtent(FVector(GridWidth * TileSize * 0.5f, GridHeight * TileSize * 0.5f, 5.0f));
    GridSurface->SetRelativeLocation(FVector(0.0f, 0.0f, -1.0f));

    BuildInitialRoom();
}

void AVTTBoard::BuildInitialRoom()
{
    ActiveCells.Empty();
    for (int32 X = 0; X < GridWidth; ++X)
    {
        for (int32 Y = 0; Y < GridHeight; ++Y)
        {
            ActiveCells.Add(FIntPoint(X, Y));
        }
    }

    RebuildFloorInstances();
    RebuildBoundaryWalls();
}

bool AVTTBoard::WorldToGrid(const FVector& WorldLocation, FIntPoint& OutGrid) const
{
    const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
    const float MinX = -GridWidth * TileSize * 0.5f;
    const float MinY = -GridHeight * TileSize * 0.5f;
    const int32 X = FMath::FloorToInt((Local.X - MinX) / TileSize);
    const int32 Y = FMath::FloorToInt((Local.Y - MinY) / TileSize);

    if (X < 0 || X >= GridWidth || Y < 0 || Y >= GridHeight)
    {
        return false;
    }

    OutGrid = FIntPoint(X, Y);
    return true;
}

FVector AVTTBoard::GridToWorld(const FIntPoint& Grid) const
{
    const float MinX = -GridWidth * TileSize * 0.5f;
    const float MinY = -GridHeight * TileSize * 0.5f;
    const FVector Local(
        MinX + (Grid.X + 0.5f) * TileSize,
        MinY + (Grid.Y + 0.5f) * TileSize,
        0.0f);
    return GetActorTransform().TransformPosition(Local);
}

bool AVTTBoard::IsCellActive(const FIntPoint& Grid) const
{
    return ActiveCells.Contains(Grid);
}

void AVTTBoard::ToggleCell(const FIntPoint& Grid)
{
    if (Grid.X < 0 || Grid.X >= GridWidth || Grid.Y < 0 || Grid.Y >= GridHeight)
    {
        return;
    }

    if (ActiveCells.Contains(Grid))
    {
        ActiveCells.Remove(Grid);
    }
    else
    {
        ActiveCells.Add(Grid);
    }

    RebuildFloorInstances();
}

void AVTTBoard::RebuildFloorInstances()
{
    FloorInstances->ClearInstances();
    const FVector FloorScale(TileSize / 100.0f * 0.96f, TileSize / 100.0f * 0.96f, 0.08f);

    for (const FIntPoint& Grid : ActiveCells)
    {
        const FVector World = GridToWorld(Grid) + FVector(0.0f, 0.0f, -9.0f);
        const FVector Local = GetActorTransform().InverseTransformPosition(World);
        FloorInstances->AddInstance(FTransform(FRotator::ZeroRotator, Local, FloorScale));
    }
}

void AVTTBoard::RebuildBoundaryWalls()
{
    WallInstances->ClearInstances();
    const float HalfWidth = GridWidth * TileSize * 0.5f;
    const float HalfHeight = GridHeight * TileSize * 0.5f;
    const FVector HorizontalScale(HalfWidth / 50.0f, 0.08f, 0.6f);
    const FVector VerticalScale(0.08f, HalfHeight / 50.0f, 0.6f);

    WallInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, -HalfHeight, 60.0f), HorizontalScale));
    WallInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, HalfHeight, 60.0f), HorizontalScale));
    WallInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(-HalfWidth, 0.0f, 60.0f), VerticalScale));
    WallInstances->AddInstance(FTransform(FRotator::ZeroRotator, FVector(HalfWidth, 0.0f, 60.0f), VerticalScale));
}
