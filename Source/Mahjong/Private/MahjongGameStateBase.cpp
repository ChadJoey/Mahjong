// Fill out your copyright notice in the Description page of Project Settings.


#include "MahjongGameStateBase.h"
#include <MahjongPlayerState.h>

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
    TArray<FTileData> All;
    for (APlayerState* PS : PlayerArray)
    {
        if (AMahjongPlayerState* MPS = Cast<AMahjongPlayerState>(PS))
        {
            All.Append(MPS->GetDiscards());
            for (const FMeldData& Meld : MPS->GetRevealedMelds())
                All.Append(Meld.Tiles);
        }
    }
    return All;
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
