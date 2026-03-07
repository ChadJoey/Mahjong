// Fill out your copyright notice in the Description page of Project Settings.


#include "MahjongPlayer.h"

AMahjongPlayer::AMahjongPlayer()
{
	Difficulty = EAIDifficulty::Medium;
	ThinkingTime = 1.0;
}

FTileData AMahjongPlayer::DecideDiscard()
{
	Super::BeginPlay();
	switch (Difficulty)
	{
	case EAIDifficulty::Easy:
		return DecideDiscardEasy();
		break;
	case EAIDifficulty::Medium:
		return DecideDiscardMedium();
		break;
	case EAIDifficulty::Hard:
		return DecideDiscardHard();
		break;
	default:
		break;
	}
	return FTileData();
}

bool AMahjongPlayer::ShouldCallChi(const FTileData& DiscardedTile)
{
	// Implement chi logic based on difficulty
	if (Difficulty == EAIDifficulty::Easy)
		return false; // Easy AI doesn't call chi

	// Check if chi would improve shanten
	// TODO: Implement
	return false;
}

bool AMahjongPlayer::ShouldCallPon(const FTileData& DiscardedTile)
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	// Count how many of this tile we have
	TArray<FTileData> Hand = MyState->GetHand();
	int32 Count = 0;
	for (const FTileData& Tile : Hand)
	{
		if (Tile == DiscardedTile)
			Count++;
	}

	// Can pon if we have 2+ of this tile
	if (Count < 2)
		return false;

	// Medium+ AI: check if pon improves hand
	if (Difficulty >= EAIDifficulty::Medium)
	{
		// Calculate shanten before and after pon
		// TODO: Implement proper evaluation
	}

	return Difficulty >= EAIDifficulty::Medium;
}

bool AMahjongPlayer::ShouldCallKan(const FTileData& DiscardedTile)
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	// Count how many of this tile we have
	TArray<FTileData> Hand = MyState->GetHand();
	int32 Count = 0;
	for (const FTileData& Tile : Hand)
	{
		if (Tile == DiscardedTile)
			Count++;
	}

	// Can pon if we have 2+ of this tile
	if (Count < 3)
		return false;

	// Medium+ AI: check if pon improves hand
	if (Difficulty >= EAIDifficulty::Medium)
	{
		// Calculate shanten before and after pon
		// TODO: Implement proper evaluation
	}

	return Difficulty >= EAIDifficulty::Medium;
}

bool AMahjongPlayer::ShouldDeclareRiichi()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	TArray<FTileData> Hand = MyState->GetHand();
	TArray<uint8> Tiles34 = FShantenCalculator::ToTile34Array(Hand);
	int32 Shanten = FShantenCalculator::Calculate(Tiles34);
	//can only declare riichi if we are in tenpai (shanten = 0)
	if (Shanten != 0)
	{
		return false;
	}

	// Medium AI always declares when possible
	if (Difficulty == EAIDifficulty::Medium)
		return true;

	// Hard AI considers point situation and tile safety
	// TODO: Implement more sophisticated logic
	return true;
}

bool AMahjongPlayer::ShouldDeclareRon(const FTileData& DiscardedTile)
{
	// Check if adding this tile completes the hand
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	TArray<FTileData> TestHand = MyState->GetHand();
	TestHand.Add(DiscardedTile);

	TArray<uint8> Tiles34 = FShantenCalculator::ToTile34Array(TestHand);
	int32 Shanten = FShantenCalculator::Calculate(Tiles34);

	// Shanten = -1 means winning hand
	return (Shanten == -1);
}

bool AMahjongPlayer::ShouldDeclareTsumo()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState) return false;

	TArray<FTileData> Hand = MyState->GetHand();
	TArray<uint8> Tiles34 = FShantenCalculator::ToTile34Array(Hand);
	int32 Shanten = FShantenCalculator::Calculate(Tiles34);

	return (Shanten == -1);
}

void AMahjongPlayer::BeginPlay()
{
	Super::BeginPlay();
	switch (Difficulty)
	{
	case EAIDifficulty::Easy:
		break;
	case EAIDifficulty::Medium:
		HandEvaluator = NewObject<UShantenEvaluator>(this);
		break;
	case EAIDifficulty::Hard:
		HandEvaluator = NewObject<UShantenEvaluator>(this);
		break;
	default:
		break;
	}

}

void AMahjongPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

AMahjongPlayerState* AMahjongPlayer::GetMahjongPlayerState() const
{
	return Cast<AMahjongPlayerState>(PlayerState);
}

FTileData AMahjongPlayer::DecideDiscardEasy()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState || MyState->GetHandSize() == 0)
	{
		return FTileData();
	}

	TArray<FTileData> Hand = MyState->GetHand();
	int32 RandomIndex = FMath::RandRange(0, Hand.Num() - 1);
	return Hand[RandomIndex];
}

FTileData AMahjongPlayer::DecideDiscardMedium()
{
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState || !HandEvaluator)
	{
		return FTileData();
	}

	TArray<FTileData> Hand = MyState->GetHand();
	return HandEvaluator->GetBestDiscard(Hand);
}

FTileData AMahjongPlayer::DecideDiscardHard()
{
	/*
	this is a guideline
	make proper implementation after monte carlo is implemented
	*/


	// Use Monte Carlo + tile efficiency + defensive play
	AMahjongPlayerState* MyState = GetMahjongPlayerState();
	if (!MyState || !HandEvaluator)
		return FTileData();

	TArray<FTileData> Hand = MyState->GetHand();

	// Get shanten-optimal discards
	TMap<FTileData, int32> DiscardResults = FShantenCalculator::CalculateDiscardResult(Hand);

	// Find minimum shanten
	int32 MinShanten = INT32_MAX;
	for (const auto& Pair : DiscardResults)
	{
		MinShanten = FMath::Min(MinShanten, Pair.Value);
	}

	// Among tiles that maintain minimum shanten, choose safest
	TArray<FTileData> OptimalDiscards;
	for (const auto& Pair : DiscardResults)
	{
		if (Pair.Value == MinShanten)
		{
			OptimalDiscards.Add(Pair.Key);
		}
	}

	// TODO: Add defensive play logic (check which tiles are dangerous)
	// For now, just return first optimal discard
	return OptimalDiscards.Num() > 0 ? OptimalDiscards[0] : Hand[0];

}

float AMahjongPlayer::EvaluateHandMonteCarlo(const TArray<FTileData>& Hand, int32 NumSimulations)
{
	return 0.0f;
}
