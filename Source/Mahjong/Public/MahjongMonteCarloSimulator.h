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
	int32 AvgFinalShanten = 0;
	int32 Simulations = 0;
	float Score = 0.f;
};

class MAHJONG_API FMahjongMonteCalroSimulator {
public:

	static TArray<FMonteCarloResult> EvaluateDiscards(const FMonteCarloInput& Input, int32 NumSimulations = 300);

	static FTileData IndexToTile(int32 idx);
	static int32 TileToIndex(const FTileData& Tile);

protected:
private:
	// Pool = 4 of every tile − MyHand13 − SeenTiles
	static TArray<uint8> BuildUnknownPool34(const FMonteCarloInput, const TArray<uint8>& MyHand13);

	// One forward simulation. UnknownList is a pre-built flat list (shuffled inside).
   // Returns {winning_player_index, turns_elapsed}; winner==-1 means ryuukyoku.
	static TPair<int32, int32> RunOneSimulation(const FMonteCarloInput& Input, const TArray<uint8>& MyHand13, TArray<int32> UnknownList, FRandomStream& Rand);

	static int32 GreedyDiscardIndex(const TArray<uint8>& Hand34);

};