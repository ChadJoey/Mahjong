// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileActor.h"
#include "MahjongGameMode.h"
#include "MahjongVisualManager.generated.h"



// Create a DataTable with this row struct. Each row key = tile notation ("1m","9p","7z" etc.)
USTRUCT(BlueprintType)
struct FTileVisualRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* FrontMesh = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* Material = nullptr;
};


// Place this actor in the level and set the four SeatOrigins in the Details panel.
// Each SeatOrigin's location is the CENTER of that player's hand row.
// Its rotation defines which way the tiles face (Z-up, forward = toward table center).
USTRUCT(BlueprintType)
struct FSeatLayout
{
	GENERATED_BODY()

	// Center of the hand row for this seat
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform HandOrigin;

	// Where discarded tiles stack (between the hand and table center)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform DiscardOrigin;
};



UCLASS()
class MAHJONG_API AMahjongVisualManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMahjongVisualManager();
    // Set this to your TileVisuals DataTable in the Details panel
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Visuals")
    UDataTable* TileVisualTable;

    // Mesh used for face-down tiles (the tile back)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Visuals")
    UStaticMesh* TileBackMesh;

    // Layout for each of the 4 seats — set in Details panel
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Layout")
    TArray<FSeatLayout> SeatLayouts; // must have exactly 4 entries

    // Gap between tiles in a hand row (in cm — match your tile mesh width)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Layout")
    float TileSpacing = 8.f;

    // Gap between tiles in the discard grid
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Layout")
    float DiscardSpacingX = 15.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Layout")
    float DiscardSpacingY = 15.f;
    // Discard tiles per row before wrapping (standard = 6)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Layout")
    int32 DiscardsPerRow = 6;

    // Delay between dealing each tile (seconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Visuals")
    float DealInterval = 0.07f;

    // Height tiles lift to while moving (arc effect, in cm)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Visuals")
    float MoveArcHeight = 12.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Visuals")
    FRotator TileSpawnRotation = FRotator::ZeroRotator;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong|Visuals")
    FRotator TileOrientationCorrection = FRotator(0.f, 90.f, 90.f);

    virtual void Tick(float DeltaTime) override;
protected:
    virtual void BeginPlay() override;

private:
    // ── GameMode delegate handlers ────────────────────────────────────────────
    UFUNCTION()
    void OnPhaseChanged(EGamePhase OldPhase, EGamePhase NewPhase);

    UFUNCTION()
    void OnTileDiscarded(int32 PlayerIndex, FTileData Tile);

    UFUNCTION()
    void OnRoundEnded(int32 WinnerIndex, bool bTsumo);

    UFUNCTION()
    void OnTileDrawn(int32 PlayerIndex, FTileData Tile);

    // ── Tile spawning and layout ──────────────────────────────────────────────
    void DealAllHands();
    void DealNextTile();                          // timer callback — deals one tile at a time
    void RefreshHandLayout(int32 PlayerIndex);    // repositions all tiles in a hand
    void MoveTileToDiscard(ATileActor* Actor, int32 PlayerIndex, int32 DiscardIndex);
    void ClearAllTileActors();

    ATileActor* SpawnTileActor(const FTileData& Tile, FVector Location, bool bFaceUp, FRotator Rotation = FRotator::ZeroRotator);
    FTransform  GetHandTileTransform(int32 PlayerIndex, int32 SlotIndex) const;
    FTransform  GetDiscardTransform(int32 PlayerIndex, int32 DiscardIndex) const;

    // Looks up mesh+material from the DataTable using FTileData::ToString() as the key
    bool GetTileVisuals(const FTileData& Tile, UStaticMesh*& OutMesh,
        UMaterialInterface*& OutMat) const;

    // ── Runtime state ─────────────────────────────────────────────────────────
    // HandTiles[player][slot] — matches the order of PlayerState.GetHand()
    TArray<TArray<ATileActor*>> HandTiles;

    // DiscardTiles[player] — in discard order
    TArray<TArray<ATileActor*>> DiscardTiles;

    // Pending deal queue: pairs of (PlayerIndex, TileData)
    TArray<TPair<int32, FTileData>> DealQueue;
    FTimerHandle DealTimerHandle;
};
