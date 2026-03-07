// Fill out your copyright notice in the Description page of Project Settings.


#include "MahjongGameStateBase.h"

AMahjongGameStateBase::AMahjongGameStateBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMahjongGameStateBase::AddDiscard(const FTileData& Tile)
{
	DiscardTiles.Add(Tile);
}

TArray<FTileData> AMahjongGameStateBase::GetVisibleTiles() const
{
	return DiscardTiles;
}

int32 AMahjongGameStateBase::GetVisibleCount(const FTileData& Tile) const
{
    int32 Count = 0;
    for (const FTileData& VisibleTile : DiscardTiles)
    {
        if (VisibleTile == Tile)
        {
            Count++;
        }
    }
    return Count;
}

void AMahjongGameStateBase::ClearVisibleTiles()
{
    DiscardTiles.Reset();
}
