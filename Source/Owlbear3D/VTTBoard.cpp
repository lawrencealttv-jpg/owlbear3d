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

    WallEdges.Empty();
    for (int32 X = 0; X < GridWidth; ++X)
    {
        WallEdges.Add(FIntVector(X, 0, 0));
        WallEdges.Add(FIntVector(X, GridHeight, 0));
    }
    for (int32 Y = 0; Y < GridHeight; ++Y)
    {
        WallEdges.Add(FIntVector(0, Y, 1));
        WallEdges.Add(FIntVector(GridWidth, Y, 1));
    }
    RebuildWallInstances();
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

bool AVTTBoard::ToggleWallAtWorldLocation(const FVector& WorldLocation)
{
    FIntPoint Grid;
    if (!WorldToGrid(WorldLocation, Grid))
    {
        return false;
    }

    const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
    const FVector CellCentre = GetActorTransform().InverseTransformPosition(GridToWorld(Grid));
    const FVector2D FromCentre(Local.X - CellCentre.X, Local.Y - CellCentre.Y);

    FIntVector Edge;
    if (FMath::Abs(FromCentre.X) > FMath::Abs(FromCentre.Y))
    {
        const int32 EdgeX = FromCentre.X >= 0.0f ? Grid.X + 1 : Grid.X;
        Edge = FIntVector(EdgeX, Grid.Y, 1);
    }
    else
    {
        const int32 EdgeY = FromCentre.Y >= 0.0f ? Grid.Y + 1 : Grid.Y;
        Edge = FIntVector(Grid.X, EdgeY, 0);
    }

    if (WallEdges.Contains(Edge))
    {
        WallEdges.Remove(Edge);
    }
    else
    {
        WallEdges.Add(Edge);
    }
    RebuildWallInstances();
    return true;
}

TArray<FIntPoint> AVTTBoard::GetActiveCells() const
{
    return ActiveCells.Array();
}

TArray<FIntVector> AVTTBoard::GetWallEdges() const
{
    return WallEdges.Array();
}

void AVTTBoard::ApplyLayout(const TArray<FIntPoint>& NewActiveCells, const TArray<FIntVector>& NewWallEdges)
{
    ActiveCells.Empty();
    for (const FIntPoint& Cell : NewActiveCells)
    {
        if (Cell.X >= 0 && Cell.X < GridWidth && Cell.Y >= 0 && Cell.Y < GridHeight)
        {
            ActiveCells.Add(Cell);
        }
    }

    WallEdges.Empty();
    for (const FIntVector& Edge : NewWallEdges)
    {
        const bool bHorizontalValid = Edge.Z == 0 && Edge.X >= 0 && Edge.X < GridWidth && Edge.Y >= 0 && Edge.Y <= GridHeight;
        const bool bVerticalValid = Edge.Z == 1 && Edge.X >= 0 && Edge.X <= GridWidth && Edge.Y >= 0 && Edge.Y < GridHeight;
        if (bHorizontalValid || bVerticalValid)
        {
            WallEdges.Add(Edge);
        }
    }

    RebuildFloorInstances();
    RebuildWallInstances();
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

void AVTTBoard::RebuildWallInstances()
{
    WallInstances->ClearInstances();
    const float MinX = -GridWidth * TileSize * 0.5f;
    const float MinY = -GridHeight * TileSize * 0.5f;
    const FVector HorizontalScale(TileSize / 100.0f * 0.98f, 0.08f, 0.6f);
    const FVector VerticalScale(0.08f, TileSize / 100.0f * 0.98f, 0.6f);

    for (const FIntVector& Edge : WallEdges)
    {
        if (Edge.Z == 0)
        {
            const FVector Location(MinX + (Edge.X + 0.5f) * TileSize, MinY + Edge.Y * TileSize, 60.0f);
            WallInstances->AddInstance(FTransform(FRotator::ZeroRotator, Location, HorizontalScale));
        }
        else
        {
            const FVector Location(MinX + Edge.X * TileSize, MinY + (Edge.Y + 0.5f) * TileSize, 60.0f);
            WallInstances->AddInstance(FTransform(FRotator::ZeroRotator, Location, VerticalScale));
        }
    }
}
