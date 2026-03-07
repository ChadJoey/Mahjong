// Fill out your copyright notice in the Description page of Project Settings.


#include "MahjongPlayerState.h"

AMahjongPlayerState::AMahjongPlayerState()
{
	Points = 25000;
	bIsRiichi = false;
	Seat = EMahjongSeat::East;
}

void AMahjongPlayerState::AddTileToHand(const FTileData& Tile)
{
	Hand.Add(Tile);
}

void AMahjongPlayerState::RemoveTileFromHand(const FTileData& Tile)
{
	Hand.RemoveSingle(Tile);
}

void AMahjongPlayerState::SortHand()
{
	Hand.Sort([](const FTileData& A, const FTileData& B) {
		if (A.Suit != B.Suit)
			return A.Suit < B.Suit;
		return A.Value < B.Value;
		});
}

void AMahjongPlayerState::AddRevealedMeld(const FMeldData& Meld)
{
	RevealedMelds.Add(Meld);
}
