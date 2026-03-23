#pragma once

#include "CoreMinimal.h"
#include "TileTypes.h"
#include "ShantenCalculator.h"


struct FMonteCarloInput 
{
	TArray<uint8> MyHand34;
	int32 MyPlayerIndex = 0;
	int32 NumPlayers = 4;
	int32 WallRemaining = 70;

	TArray<uint8> SeenTiles34;

	TArray<int32> OpponentMeldCounts;
};


struct FMonteCarloResult
{
	FTileData Discard;
	float Winrate = 0.f;
	float AvgTurnsToWin = 0.f;
	int32 = AvgFinalShanten = 0;
	int32 = Simulations = 0;
	float Score = 0.f;
};

class MAHGJONG_API FMahjongMonteCalroSimulator {
public:

	static TArray<FMonteCarloResult> EvaluateDiscards(const FMonteCarloInput& Input, int32 NumSimulations = 300);

protected:
private:
};