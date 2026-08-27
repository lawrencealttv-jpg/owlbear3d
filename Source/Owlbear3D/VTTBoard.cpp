#include "VTTBoard.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AVTTBoard::AVTTBoard()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);

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

    DoorInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DoorInstances"));
    DoorInstances->SetupAttachment(SceneRoot);
    DoorInstances->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    FogInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FogInstances"));
    FogInstances->SetupAttachment(SceneRoot);
    FogInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        FloorInstances->SetStaticMesh(CubeMesh.Object);
        WallInstances->SetStaticMesh(CubeMesh.Object);
        DoorInstances->SetStaticMesh(CubeMesh.Object);
        FogInstances->SetStaticMesh(CubeMesh.Object);
    }
}

void AVTTBoard::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AVTTBoard, RepActiveCells);
    DOREPLIFETIME(AVTTBoard, RepWallEdges);
    DOREPLIFETIME(AVTTBoard, RepDoorEdges);
    DOREPLIFETIME(AVTTBoard, RepFoggedCells);
}

void AVTTBoard::BeginPlay()
{
    Super::BeginPlay();

    auto TintComponent = [](UInstancedStaticMeshComponent* Component, const FLinearColor& Colour)
    {
        if (Component && Component->GetMaterial(0))
        {
            if (UMaterialInstanceDynamic* Material = Component->CreateDynamicMaterialInstance(0))
            {
                Material->SetVectorParameterValue(TEXT("Color"), Colour);
            }
        }
    };
    TintComponent(FloorInstances, FLinearColor(0.32f, 0.30f, 0.26f));
    TintComponent(WallInstances, FLinearColor(0.15f, 0.14f, 0.13f));
    TintComponent(DoorInstances, FLinearColor(0.30f, 0.12f, 0.035f));
    TintComponent(FogInstances, FLinearColor(0.015f, 0.02f, 0.03f));

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
    DoorEdges.Empty();
    FoggedCells.Empty();
    RebuildDoorInstances();
    RebuildFogInstances();
    SyncReplicatedLayout();
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
    SyncReplicatedLayout();
}

void AVTTBoard::SetCellActive(const FIntPoint& Grid, bool bActive)
{
    if (Grid.X < 0 || Grid.X >= GridWidth || Grid.Y < 0 || Grid.Y >= GridHeight)
    {
        return;
    }

    if (bActive)
    {
        ActiveCells.Add(Grid);
    }
    else
    {
        ActiveCells.Remove(Grid);
    }
    RebuildFloorInstances();
    SyncReplicatedLayout();
}

bool AVTTBoard::ToggleWallAtWorldLocation(const FVector& WorldLocation)
{
    FIntVector Edge;
    if (!FindNearestEdge(WorldLocation, Edge))
    {
        return false;
    }

    SetWall(Edge, !HasWall(Edge));
    return true;
}

bool AVTTBoard::FindNearestEdge(const FVector& WorldLocation, FIntVector& OutEdge) const
{
    FIntPoint Grid;
    if (!WorldToGrid(WorldLocation, Grid))
    {
        return false;
    }

    const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
    const FVector CellCentre = GetActorTransform().InverseTransformPosition(GridToWorld(Grid));
    const FVector2D FromCentre(Local.X - CellCentre.X, Local.Y - CellCentre.Y);

    if (FMath::Abs(FromCentre.X) > FMath::Abs(FromCentre.Y))
    {
        const int32 EdgeX = FromCentre.X >= 0.0f ? Grid.X + 1 : Grid.X;
        OutEdge = FIntVector(EdgeX, Grid.Y, 1);
    }
    else
    {
        const int32 EdgeY = FromCentre.Y >= 0.0f ? Grid.Y + 1 : Grid.Y;
        OutEdge = FIntVector(Grid.X, EdgeY, 0);
    }
    return IsValidEdge(OutEdge);
}

bool AVTTBoard::HasWall(const FIntVector& Edge) const
{
    return WallEdges.Contains(Edge);
}

bool AVTTBoard::HasDoor(const FIntVector& Edge) const
{
    return DoorEdges.Contains(Edge);
}

void AVTTBoard::SetWall(const FIntVector& Edge, bool bActive)
{
    if (!IsValidEdge(Edge))
    {
        return;
    }

    if (bActive)
    {
        WallEdges.Add(Edge);
        DoorEdges.Remove(Edge);
    }
    else
    {
        WallEdges.Remove(Edge);
    }
    RebuildWallInstances();
    RebuildDoorInstances();
    SyncReplicatedLayout();
}

void AVTTBoard::SetDoor(const FIntVector& Edge, bool bActive)
{
    if (!IsValidEdge(Edge))
    {
        return;
    }

    if (bActive)
    {
        DoorEdges.Add(Edge);
        WallEdges.Remove(Edge);
    }
    else
    {
        DoorEdges.Remove(Edge);
    }
    RebuildWallInstances();
    RebuildDoorInstances();
    SyncReplicatedLayout();
}

void AVTTBoard::SetFogged(const FIntPoint& Grid, bool bFogged)
{
    if (Grid.X < 0 || Grid.X >= GridWidth || Grid.Y < 0 || Grid.Y >= GridHeight)
    {
        return;
    }
    if (bFogged)
    {
        FoggedCells.Add(Grid);
    }
    else
    {
        FoggedCells.Remove(Grid);
    }
    RebuildFogInstances();
    SyncReplicatedLayout();
}

bool AVTTBoard::IsFogged(const FIntPoint& Grid) const
{
    return FoggedCells.Contains(Grid);
}

TArray<FIntPoint> AVTTBoard::GetActiveCells() const
{
    return ActiveCells.Array();
}

TArray<FIntVector> AVTTBoard::GetWallEdges() const
{
    return WallEdges.Array();
}

TArray<FIntVector> AVTTBoard::GetDoorEdges() const
{
    return DoorEdges.Array();
}

TArray<FIntPoint> AVTTBoard::GetFoggedCells() const
{
    return FoggedCells.Array();
}

void AVTTBoard::ApplyLayout(const TArray<FIntPoint>& NewActiveCells, const TArray<FIntVector>& NewWallEdges,
    const TArray<FIntVector>& NewDoorEdges, const TArray<FIntPoint>& NewFoggedCells)
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
        if (IsValidEdge(Edge))
        {
            WallEdges.Add(Edge);
        }
    }

    DoorEdges.Empty();
    for (const FIntVector& Edge : NewDoorEdges)
    {
        if (IsValidEdge(Edge))
        {
            DoorEdges.Add(Edge);
            WallEdges.Remove(Edge);
        }
    }

    FoggedCells.Empty();
    for (const FIntPoint& Cell : NewFoggedCells)
    {
        if (Cell.X >= 0 && Cell.X < GridWidth && Cell.Y >= 0 && Cell.Y < GridHeight)
        {
            FoggedCells.Add(Cell);
        }
    }

    RebuildFloorInstances();
    RebuildWallInstances();
    RebuildDoorInstances();
    RebuildFogInstances();
    SyncReplicatedLayout();
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

void AVTTBoard::RebuildDoorInstances()
{
    DoorInstances->ClearInstances();
    const float MinX = -GridWidth * TileSize * 0.5f;
    const float MinY = -GridHeight * TileSize * 0.5f;
    const FVector HorizontalScale(TileSize / 100.0f * 0.72f, 0.12f, 0.48f);
    const FVector VerticalScale(0.12f, TileSize / 100.0f * 0.72f, 0.48f);

    for (const FIntVector& Edge : DoorEdges)
    {
        if (Edge.Z == 0)
        {
            const FVector Location(MinX + (Edge.X + 0.5f) * TileSize, MinY + Edge.Y * TileSize, 48.0f);
            DoorInstances->AddInstance(FTransform(FRotator::ZeroRotator, Location, HorizontalScale));
        }
        else
        {
            const FVector Location(MinX + Edge.X * TileSize, MinY + (Edge.Y + 0.5f) * TileSize, 48.0f);
            DoorInstances->AddInstance(FTransform(FRotator::ZeroRotator, Location, VerticalScale));
        }
    }
}

void AVTTBoard::RebuildFogInstances()
{
    FogInstances->ClearInstances();
    const FVector FogScale(TileSize / 100.0f * 0.98f, TileSize / 100.0f * 0.98f, 1.6f);
    for (const FIntPoint& Grid : FoggedCells)
    {
        const FVector World = GridToWorld(Grid) + FVector(0.0f, 0.0f, 155.0f);
        const FVector Local = GetActorTransform().InverseTransformPosition(World);
        FogInstances->AddInstance(FTransform(FRotator::ZeroRotator, Local, FogScale));
    }
}

bool AVTTBoard::IsValidEdge(const FIntVector& Edge) const
{
    const bool bHorizontalValid = Edge.Z == 0 && Edge.X >= 0 && Edge.X < GridWidth && Edge.Y >= 0 && Edge.Y <= GridHeight;
    const bool bVerticalValid = Edge.Z == 1 && Edge.X >= 0 && Edge.X <= GridWidth && Edge.Y >= 0 && Edge.Y < GridHeight;
    return bHorizontalValid || bVerticalValid;
}

void AVTTBoard::SyncReplicatedLayout()
{
    if (!HasAuthority())
    {
        return;
    }
    RepActiveCells = ActiveCells.Array();
    RepWallEdges = WallEdges.Array();
    RepDoorEdges = DoorEdges.Array();
    RepFoggedCells = FoggedCells.Array();
    ForceNetUpdate();
}

void AVTTBoard::OnRep_Layout()
{
    ActiveCells.Empty();
    for (const FIntPoint& Cell : RepActiveCells) ActiveCells.Add(Cell);
    WallEdges.Empty();
    for (const FIntVector& Edge : RepWallEdges) WallEdges.Add(Edge);
    DoorEdges.Empty();
    for (const FIntVector& Edge : RepDoorEdges) DoorEdges.Add(Edge);
    FoggedCells.Empty();
    for (const FIntPoint& Cell : RepFoggedCells) FoggedCells.Add(Cell);
    RebuildFloorInstances();
    RebuildWallInstances();
    RebuildDoorInstances();
    RebuildFogInstances();
}
