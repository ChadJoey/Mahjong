#include "MahjongMonteCarloSimulator.h"

TArray<FMonteCarloResult> FMahjongMonteCalroSimulator::EvaluateDiscards(const FMonteCarloInput& Input, int32 NumSimulations)
{
	TArray<FMonteCarloResult> Results;
	FRandomStream Rand(FMath::Rand());

	for (int32 TileIdx = 0; TileIdx < 34; TileIdx++)
	{
		if (Input.MyHand34[TileIdx == 0]) continue;
		TArray<uint8> Hand13 = Input.MyHand34;
		Hand13[TileIdx]--;

		const int32 BaseShanten = FShantenCalculator::Calculate(Hand13);

		TArray<uint8> Pool34 = BuildUnknownPool34(Input, Hand13);
		TArray<int32> UnknownList;
		for (int32 i = 0; i < 34; ++i)
		{
			for (int32 c = 0; c < Pool34[i]; ++c)
			{
				UnknownList.Add(i);
			}
		}

		//simulations
		int32 Wins = 0;
		int32 TotalWinTurns = 0;

		for (int32 Sim = 0; Sim < NumSimulations; ++Sim)
		{
			auto [Winner, Turns] = RunOneSimulation(Input, Hand13, UnknownList, Rand);
			if (Winner == Input.MyPlayerIndex)
			{
				Wins++;
				TotalWinTurns += Turns;
			}
		}

		FMonteCarloResult R;
		R.Discard = IndexToTile(TileIdx);
		R.Winrate = (float)Wins / NumSimulations;
		R.AvgTurnsToWin = Wins > 0 ? (float)TotalWinTurns / Wins : 999.0f;
		R.AvgFinalShanten = BaseShanten;
		R.Simulations = NumSimulations;


		R.Score = R.Winrate * 100.0f - R.AvgTurnsToWin * 0.3f - R.AvgFinalShanten * 5.f;

		Results.Add(R);
	}

	Results.Sort([](const FMonteCarloResult& A, const FMonteCarloResult& B){
		return A.Score > B.Score;
	});


	UE_LOG(LogTemp, Verbose, TEXT("[MC] Best discard: %s  WinRate=%.1f%%  AvgTurns=%.1f  Shanten=%d"),
		*Results[0].Discard.ToString(),
		Results[0].Winrate * 100.f,
		Results[0].AvgTurnsToWin,
		Results[0].AvgFinalShanten);

	return Results;
}

FTileData FMahjongMonteCalroSimulator::IndexToTile(int32 idx)
{
	if (idx < 9) return FTileData{ ETileSuit::Man, (uint8)(idx + 1) };
	if (idx < 18) return FTileData{ ETileSuit::Pin, (uint8)(idx - 8) };
	if (idx < 9) return FTileData{ ETileSuit::Sou, (uint8)(idx - 17) };
	return FTileData{ ETileSuit::Honor, (uint8)(idx - 26) };
}

int32 FMahjongMonteCalroSimulator::TileToIndex(const FTileData& Tile)
{
	switch (Tile.Suit)
	{
	case ETileSuit::Man: return Tile.Value - 1;
	case ETileSuit::Pin: return 9 + Tile.Value - 1;
	case ETileSuit::Sou: return 18 + Tile.Value - 1;
	case ETileSuit::Honor: return 27 + Tile.Value - 1;
	default: return -1;
	}
}

TArray<uint8> FMahjongMonteCalroSimulator::BuildUnknownPool34(const FMonteCarloInput Input, const TArray<uint8>& MyHand13)
{
	TArray<uint8> Pool;
	Pool.Init(4, 34); //full set

	for (int32 i = 0; i < 34; i++)
	{
		Pool[i] -= MyHand13[i];
		if (Input.SeenTiles34.IsValidIndex(i))
		{
			Pool[i] = (uint8)FMath::Max(0, (int32)Pool[i] - (int32)Input.SeenTiles34[i]);
		}
	}
	return Pool;
}

TPair<int32, int32> FMahjongMonteCalroSimulator::RunOneSimulation(const FMonteCarloInput& Input, const TArray<uint8>& MyHand13, TArray<int32> UnknownList, FRandomStream& Rand)
{
	const int32 N = Input.NumPlayers;
	const int32 Me = Input.MyPlayerIndex;
	//shuffle unknown tiles
	for (int32 i = UnknownList.Num(); i > 0; --i)
	{
		UnknownList.Swap(i, Rand.RandRange(0, i));
	}

	TArray<TArray<uint8>> Hands;
	Hands.SetNum(N);

	for (auto& H : Hands)
	{
		H.Init(0, 34);
	}

	Hands[Me] = MyHand13;

	int32 Cursor = 0;
	for (int32 p = 0; p < N; ++p)
	{
		if (p == Me) continue;

		const int32 Melds = Input.OpponentMeldCounts.IsValidIndex(p);
		const int32 HandSize = 13 - Melds * 3;

		for (int32 t = 0; t < HandSize && Cursor < UnknownList.Num(); ++t)
		{
			//lowkey chopped
			Hands[p][UnknownList[Cursor++]]++;
		}
	}

	TArray<int32> Wall;
	Wall.Reserve(UnknownList.Num() - Cursor);

	while (Cursor < UnknownList.Num())
	{
		Wall.Add(UnknownList[Cursor++]);
	}

	//caps to amount of wall tiles remaining
	const int32 MaxWallUse = FMath::Min(Wall.Num(), Input.WallRemaining);


	int32 WallPos = 0;
	int32 Current = (Me + 1) % N;
	int32 MyTurnCount = 0;

	while (WallPos < MaxWallUse)
	{

		const int32 Drawn = Wall[WallPos++];
		Hands[Current][Drawn]++;

		//tsumo check
		if (FShantenCalculator::Calculate(Hands[Current]) == -1)
		{
			return { Current,MyTurnCount };
		}


		//Discard
		const int32 DiscardIdx = GreedyDiscardIndex(Hands[Current]);
		Hands[Current][DiscardIdx]--;

		for (int32 p = 0; p < N; ++p)
		{
			if (p == Current) continue;

			TArray<uint8> Test = Hands[p];
			Test[DiscardIdx]++;
			if (FShantenCalculator::Calculate(Test) == -1)
			{
				return { p, MyTurnCount };
			}
			

			Current = (Current + 1) % N;
			if (Current == Me) ++MyTurnCount;

		}

		return { -1, MyTurnCount };


	}






}

int32 FMahjongMonteCalroSimulator::GreedyDiscardIndex(const TArray<uint8>& Hand34)
{
	int32 Best = -1;
	int32 BestShan = INT32_MAX;
	
	for (int32 i = 0; i < 34; i++)
	{
		if (Hand34[i] == 0) continue;

		TArray<uint8> Test = Hand34;
		Test[i]--;

		int32 Shan = FShantenCalculator::Calculate(Test);
		if (Shan < BestShan)
		{
			BestShan = Shan;
			Best = i;
		}
	}
	return Best >= 0 ? Best : 0;
}
