// Fill out your copyright notice in the Description page of Project Settings.

#include "MahjongVisualManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

// Sets default values
AMahjongVisualManager::AMahjongVisualManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMahjongVisualManager::BeginPlay()
{
	Super::BeginPlay();

	HandTiles.SetNum(4);
	DiscardTiles.SetNum(4);

	if (AMahjongGameMode* GM = GetWorld()->GetAuthGameMode<AMahjongGameMode>())
	{
		GM->OnPhaseChanged.AddDynamic(this, &AMahjongVisualManager::OnPhaseChanged);
		GM->OnTileDiscarded.AddDynamic(this, &AMahjongVisualManager::OnTileDiscarded);
		GM->OnRoundEnded.AddDynamic(this, &AMahjongVisualManager::OnRoundEnded);
		GM->OnTileDrawn.AddDynamic(this, &AMahjongVisualManager::OnTileDrawn);

		GM->StartGame();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("VisualManager: AMahjongGameMode not found." "Check your Project Settings -> Maps & Modes."));
	}
	
}

void AMahjongVisualManager::OnPhaseChanged(EGamePhase OldPhase, EGamePhase NewPhase)
{
	if (NewPhase == EGamePhase::Dealing)
	{
		ClearAllTileActors();
		DealAllHands();
	}
}

void AMahjongVisualManager::OnTileDrawn(int32 PlayerIndex, FTileData Tile)
{
	if (!SeatLayouts.IsValidIndex(PlayerIndex)) return;

	// SlotIdx is the current count BEFORE adding — GetHandTileTransform uses +1 internally
	const int32 SlotIdx = HandTiles[PlayerIndex].Num();
	FTransform Target = GetHandTileTransform(PlayerIndex, SlotIdx);
	FVector SpawnLoc = Target.GetLocation() + FVector(0, 0, MoveArcHeight);

	ATileActor* Actor = SpawnTileActor(Tile, SpawnLoc, false, TileSpawnRotation);
	if (!Actor) return;

	HandTiles[PlayerIndex].Add(Actor);
	Actor->MoveTo(Target, 0.2f);

	// Recenter now that the hand has 14 tiles
	RefreshHandLayout(PlayerIndex);
}

void AMahjongVisualManager::OnTileDiscarded(int32 PlayerIndex, FTileData Tile)
{
	TArray<ATileActor*>& PHand = HandTiles[PlayerIndex];
	if (PHand.Num() == 0) return;

	ATileActor* ToDiscard = nullptr;
	for (int32 i = PHand.Num() - 1; i >= 0; --i)
	{
		if (PHand[i] && PHand[i]->TileData == Tile)
		{
			ToDiscard = PHand[i];
			PHand.RemoveAt(i);
			break;
		}
	}
	if (!ToDiscard) return;

	const int32 DiscardIdx = DiscardTiles[PlayerIndex].Num();
	DiscardTiles[PlayerIndex].Add(ToDiscard);

	ToDiscard->SetFaceUp(true);

	MoveTileToDiscard(ToDiscard, PlayerIndex, DiscardIdx);

	RefreshHandLayout(PlayerIndex);

}

void AMahjongVisualManager::OnRoundEnded(int32 WinnerIndex, bool bTsumo)
{

	if (WinnerIndex >= 0){
		UE_LOG(LogTemp, Log, TEXT("VisualManager: Player %d wins by %s"),
			WinnerIndex, bTsumo ? TEXT("Tsumo") : TEXT("Ron"));
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("VisualManager: Ryuukyoku"));
	}
}

void AMahjongVisualManager::DealAllHands()
{
	for (int32 i = 0; i < 4; ++i) { HandTiles[i].Empty(); DiscardTiles[i].Empty(); }
	DealQueue.Empty();

	AMahjongGameMode* GM = GetWorld()->GetAuthGameMode<AMahjongGameMode>();
	if (!GM) { UE_LOG(LogTemp, Error, TEXT("DealAllHands: no GameMode")); return; }

	for (int32 i = 0; i < 4; ++i)
	{
		AMahjongPlayerState* MPS = GM->GetPlayerStateAt(i);
		if (!MPS) { UE_LOG(LogTemp, Warning, TEXT("DealAllHands: no PlayerState for seat %d"), i); continue; }

		for (const FTileData& Tile : MPS->GetHand())
			DealQueue.Add({ i, Tile });
	}

	UE_LOG(LogTemp, Log, TEXT("DealAllHands: queued %d tiles"), DealQueue.Num());

	GetWorldTimerManager().SetTimer(DealTimerHandle,
		this, &AMahjongVisualManager::DealNextTile, DealInterval, true);
}


void AMahjongVisualManager::DealNextTile()
{
	if (DealQueue.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(DealTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("DealNextTile: queue empty, dealing complete"));

		if (AMahjongGameMode* GM = GetWorld()->GetAuthGameMode<AMahjongGameMode>())
			GM->OnDealingComplete();
		return;
	}

	auto [PlayerIndex, Tile] = DealQueue[0];
	DealQueue.RemoveAt(0);

	if (!SeatLayouts.IsValidIndex(PlayerIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("DealNextTile: no SeatLayout for player %d"), PlayerIndex);
		return;
	}

	const int32 SlotIdx = HandTiles[PlayerIndex].Num();
	FTransform Target = GetHandTileTransform(PlayerIndex, SlotIdx);
	FVector SpawnLoc = Target.GetLocation() + FVector(0, 0, MoveArcHeight);

	ATileActor* Actor = SpawnTileActor(Tile, SpawnLoc, false, TileSpawnRotation);
	if (!Actor) return;

	HandTiles[PlayerIndex].Add(Actor);
	Actor->MoveTo(Target, DealInterval * 0.9f);
}

void AMahjongVisualManager::RefreshHandLayout(int32 PlayerIndex)
{
	TArray<ATileActor*>& PHand = HandTiles[PlayerIndex];
	if (!SeatLayouts.IsValidIndex(PlayerIndex)) return;

	const FTransform& Origin = SeatLayouts[PlayerIndex].HandOrigin;
	const FVector Right = Origin.GetRotation().GetRightVector();
	const FQuat FinalRot = Origin.GetRotation() * TileOrientationCorrection.Quaternion();

	for (int32 i = 0; i < PHand.Num(); ++i)
	{
		if (!PHand[i]) continue;
		// Same formula as GetHandTileTransform — no centering
		FVector Loc = Origin.GetLocation() + Right * (i * TileSpacing);
		FTransform Target(FinalRot, Loc, Origin.GetScale3D());
		PHand[i]->MoveTo(Target, 0.15f);
	}
}

void AMahjongVisualManager::MoveTileToDiscard(ATileActor* Actor, int32 PlayerIndex, int32 DiscardIndex)
{
	FTransform Target = GetDiscardTransform(PlayerIndex, DiscardIndex);

	// Arc: lift the tile up, then drop it into its discard slot
	FVector ArcPeak = FMath::Lerp(
		Actor->GetActorLocation(), Target.GetLocation(), 0.5f)
		+ FVector(0, 0, MoveArcHeight);

	// Two-stage move: up to arc peak, then down to target.
	// For simplicity we do a single lerp; replace with a spline if you want a true arc.
	Actor->MoveTo(Target, 0.3f);
}

void AMahjongVisualManager::ClearAllTileActors()
{
	GetWorldTimerManager().ClearTimer(DealTimerHandle);

	for (TArray<ATileActor*>& PHand : HandTiles)
	{
		for (ATileActor* A : PHand) if (A) A->Destroy();
		PHand.Empty();
	}
	for (TArray<ATileActor*>& PDiscard : DiscardTiles)
	{
		for (ATileActor* A : PDiscard) if (A) A->Destroy();
		PDiscard.Empty();
	}
	DealQueue.Empty();
}

// Called every frame
void AMahjongVisualManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

ATileActor* AMahjongVisualManager::SpawnTileActor(const FTileData& Tile, FVector Location, bool bFaceUp, FRotator Rotation)
{
	if (!TileBackMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnTileActor: TileBackMesh is null"));
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATileActor* Actor = GetWorld()->SpawnActor<ATileActor>(
		ATileActor::StaticClass(), Location, Rotation, Params);

	if (!Actor) return nullptr;

	// Always look up front visuals — InitTile needs them regardless of face direction
	UStaticMesh* FrontMesh = nullptr;
	UMaterialInterface* FrontMat = nullptr;
	GetTileVisuals(Tile, FrontMesh, FrontMat);

	Actor->InitTile(Tile, FrontMesh, FrontMat, TileBackMesh);
	Actor->SetFaceUp(bFaceUp); // rotation handles which face shows
	return Actor;
}

FTransform AMahjongVisualManager::GetHandTileTransform(int32 PlayerIndex, int32 SlotIndex) const
{
	if (!SeatLayouts.IsValidIndex(PlayerIndex)) return FTransform::Identity;

	const FSeatLayout& Layout = SeatLayouts[PlayerIndex];
	const FTransform& Origin = Layout.HandOrigin;

	const FVector Right = Origin.GetRotation().GetRightVector();
	const FVector Loc = Origin.GetLocation() + Right * (SlotIndex * TileSpacing);

	const FQuat FinalRot = Origin.GetRotation() * TileOrientationCorrection.Quaternion();
	return FTransform(FinalRot, Loc, Origin.GetScale3D());
}

FTransform AMahjongVisualManager::GetDiscardTransform(int32 PlayerIndex, int32 DiscardIndex) const
{
	if (!SeatLayouts.IsValidIndex(PlayerIndex)) return FTransform::Identity;

	const FSeatLayout& Layout = SeatLayouts[PlayerIndex];
	const FTransform& Origin = Layout.DiscardOrigin;

	const FVector Right = Origin.GetRotation().GetRightVector();
	const FVector Forward = Origin.GetRotation().GetForwardVector();

	const int32 Col = DiscardIndex % DiscardsPerRow;
	const int32 Row = DiscardIndex / DiscardsPerRow;

	const FVector Loc = Origin.GetLocation()
		+ Right * (Col * DiscardSpacing)
		+ Forward * (Row * DiscardSpacing);

	const FRotator Correction(TileOrientationCorrection);
	const FQuat FinalRot = Origin.GetRotation() * Correction.Quaternion();

	return FTransform(FinalRot, Loc, Origin.GetScale3D());

}

bool AMahjongVisualManager::GetTileVisuals(const FTileData& Tile, UStaticMesh*& OutMesh, UMaterialInterface*& OutMat) const
{
	if (!TileVisualTable) return false;

	// Row key matches FTileData::ToString() e.g. "1m", "9p", "7z"
	const FString Key = Tile.ToString();
	const FTileVisualRow* Row = TileVisualTable->FindRow<FTileVisualRow>(
		FName(*Key), TEXT("TileVisual lookup"));

	if (!Row) return false;

	OutMesh = Row->FrontMesh;
	OutMat = Row->Material;
	return true;
}
