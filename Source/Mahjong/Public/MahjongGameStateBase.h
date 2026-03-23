// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TileTypes.h"
#include "MahjongGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class MAHJONG_API AMahjongGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	

public:
	AMahjongGameStateBase();

protected:

	UPROPERTY()
	TArray<FTileData> DiscardTiles;

	UPROPERTY()
	TArray<FTileData> RevealedTiles;


	//implement as stretch goal
	UPROPERTY()
	TArray<FTileData> DoraIndicators;

public:
	UFUNCTION(BlueprintCallable)
	void AddDiscard(const FTileData& Tile);

	UFUNCTION(BlueprintCallable)
	TArray<FTileData> GetVisibleTiles() const;


	UFUNCTION(BlueprintCallable)
	int32 GetVisibleCount(const FTileData& Tile) const;


	UFUNCTION(BlueprintCallable)
	void AddRevealedTile(const FTileData& Tile) { RevealedTiles.Add(Tile); }

};
