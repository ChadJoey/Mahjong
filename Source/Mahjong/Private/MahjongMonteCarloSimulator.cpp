#include "MahjongMonteCarloSimulator.h"
#include "Async/ParallelFor.h"
#include "Async/Async.h"
#include "Async/AsyncWork.h"



TArray<FMonteCarloResult> FMahjongMonteCarloSimulator::EvaluateDiscardsOld(const FMonteCarloInput& Input, int32 NumSimulations)
{
	int32 MinShanten = INT32_MAX;
	for (int32 i = 0; i < 34; ++i)
	{
		if (Input.MyHand34[i] == 0) continue;
		TArray<uint8> Test = Input.MyHand34;
		Test[i]--;
		MinShanten = FMath::Min(MinShanten, FShantenCalculator::Calculate(Test));
	}


	TArray<FMonteCarloResult> Results;
	FRandomStream Rand(FMath::Rand());

	for (int32 TileIdx = 0; TileIdx < 34; TileIdx++)
	{
		if (Input.MyHand34[TileIdx] == 0) continue;
		TArray<uint8> Hand13 = Input.MyHand34;
		Hand13[TileIdx]--;

		const int32 BaseShanten = FShantenCalculator::Calculate(Hand13);

		if (BaseShanten > MinShanten) continue;

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

	const int32 CandidateCount = Results.Num();
	const int32 TotalSims = CandidateCount * NumSimulations;

	UE_LOG(LogTemp, Log, TEXT("[MC] Ran %d simulations across %d candidates (%d each)"),
		TotalSims,
		CandidateCount,
		NumSimulations);

	UE_LOG(LogTemp, Verbose, TEXT("[MC] Best discard: %s  WinRate=%.1f%%  AvgTurns=%.1f  Shanten=%d"),
		*Results[0].Discard.ToString(),
		Results[0].Winrate * 100.f,
		Results[0].AvgTurnsToWin,
		Results[0].AvgFinalShanten);
	return Results;
}


TArray<FMonteCarloResult> FMahjongMonteCarloSimulator::EvaluateDiscards(const FMonteCarloInput& Input, int32 NumSimulations)
{
	// ── 1. Best achievable shanten after any single discard (serial, 34 iters)
	int32 MinShanten = INT32_MAX;
	for (int32 i = 0; i < 34; ++i)
	{
		if (Input.MyHand34[i] == 0) continue;
		TArray<uint8> Test = Input.MyHand34;
		Test[i]--;
		MinShanten = FMath::Min(MinShanten, FShantenCalculator::Calculate(Test));
	}

	// ── 2. Collect candidates + pre-build their pools (serial, cheap) ─────
	struct FCandidate
	{
		int32         TileIdx;
		TArray<uint8> Hand13;
		TArray<int32> UnknownList;
		int32         BaseShanten;
		int32         Seed;        // generated here on the game thread —
	};                             // FMath::Rand is not thread-safe

	TArray<FCandidate> Candidates;
	Candidates.Reserve(14); // max possible discards in a 14-tile hand

	for (int32 TileIdx = 0; TileIdx < 34; ++TileIdx)
	{
		if (Input.MyHand34[TileIdx] == 0) continue;

		TArray<uint8> Hand13 = Input.MyHand34;
		Hand13[TileIdx]--;

		const int32 Shanten = FShantenCalculator::Calculate(Hand13);
		if (Shanten > MinShanten) continue;

		TArray<uint8> Pool34 = BuildUnknownPool34(Input, Hand13);
		TArray<int32> UnknownList;
		for (int32 i = 0; i < 34; ++i)
			for (int32 c = 0; c < Pool34[i]; ++c)
				UnknownList.Add(i);

		FCandidate& C = Candidates.AddDefaulted_GetRef();
		C.TileIdx = TileIdx;
		C.Hand13 = MoveTemp(Hand13);
		C.UnknownList = MoveTemp(UnknownList);
		C.BaseShanten = Shanten;
		C.Seed = FMath::RandRange(0, INT32_MAX);
	}

	if (Candidates.IsEmpty()) return {};

	const int32 NumC = Candidates.Num();

	// ── 3. Flat 2D parallel: (Candidate × SimBatch) ───────────────────────
	//
	//  Problem with one-task-per-candidate: scaling NumSimulations to 500+
	//  leaves the entire inner sim loop serial — one core per candidate,
	//  wasting all remaining cores while it churns through thousands of sims.
	//
	//  Solution: split each candidate's sims into batches so the full core
	//  budget is used regardless of candidate count or sim count.
	//
	//  Work layout: TotalTasks = NumC × NumBatches
	//    FlatIdx = CIdx * NumBatches + BIdx
	//  Each task writes to a unique flat slot → zero contention, no atomics.

	// Target ~4 tasks per logical core for good load-balancing.
	const int32 NumCores = FMath::Max(1, FPlatformMisc::NumberOfCoresIncludingHyperthreads());
	const int32 NumBatches = FMath::Max(1, (NumCores * 4 + NumC - 1) / NumC);
	const int32 SimsPerBatch = (NumSimulations + NumBatches - 1) / NumBatches;
	const int32 TotalTasks = NumC * NumBatches;

	// Pre-allocate flat accumulators — one slot per (candidate, batch) pair.
	TArray<int32> BatchWins;  BatchWins.Init(0, TotalTasks);
	TArray<int32> BatchTurns; BatchTurns.Init(0, TotalTasks);

	ParallelFor(TotalTasks, [&](int32 FlatIdx)
		{
			const int32 CIdx = FlatIdx / NumBatches;
			const int32 BIdx = FlatIdx % NumBatches;
			const int32 SimStart = BIdx * SimsPerBatch;
			const int32 SimEnd = FMath::Min(SimStart + SimsPerBatch, NumSimulations);

			// XOR the candidate seed with the batch index for divergent streams.
			FRandomStream Rand(Candidates[CIdx].Seed ^ (BIdx * 2654435761u));

			int32 LocalWins = 0, LocalTurns = 0;
			for (int32 s = SimStart; s < SimEnd; ++s)
			{
				auto [Winner, Turns] = RunOneSimulation(
					Input, Candidates[CIdx].Hand13, Candidates[CIdx].UnknownList, Rand);

				if (Winner == Input.MyPlayerIndex)
				{
					++LocalWins;
					LocalTurns += Turns;
				}
			}

			BatchWins[FlatIdx] = LocalWins;
			BatchTurns[FlatIdx] = LocalTurns;
		});

	// ── 4. Aggregate batches per candidate (serial, trivially fast) ────────
	TArray<FMonteCarloResult> Results;
	Results.SetNum(NumC);

	for (int32 ci = 0; ci < NumC; ++ci)
	{
		int32 TotalWins = 0, TotalTurns = 0;
		for (int32 b = 0; b < NumBatches; ++b)
		{
			TotalWins += BatchWins[ci * NumBatches + b];
			TotalTurns += BatchTurns[ci * NumBatches + b];
		}

		FMonteCarloResult& R = Results[ci];
		R.Discard = IndexToTile(Candidates[ci].TileIdx);
		R.Winrate = (float)TotalWins / NumSimulations;
		R.AvgTurnsToWin = TotalWins > 0 ? (float)TotalTurns / TotalWins : 999.f;
		R.AvgFinalShanten = Candidates[ci].BaseShanten;
		R.Simulations = NumSimulations;
		R.TotalSimulations = NumC * NumSimulations;
		R.CandidatesConsidered = NumC;
		R.Score = R.Winrate * 100.f
			- R.AvgTurnsToWin * 0.3f
			- R.AvgFinalShanten * 5.f;
	}

	// ── 5. Sort by score (serial, trivially fast) ──────────────────────────
	Results.Sort([](const FMonteCarloResult& A, const FMonteCarloResult& B)
		{
			return A.Score > B.Score;
		});

	UE_LOG(LogTemp, Log,
		TEXT("[MC] %d candidates × %d sims (%d batches of %d) on %d cores | "
			"Best: %s  WR=%.1f%%  AvgTurns=%.1f  Shanten=%d"),
		NumC, NumSimulations, NumBatches, SimsPerBatch, NumCores,
		*Results[0].Discard.ToString(),
		Results[0].Winrate * 100.f,
		Results[0].AvgTurnsToWin,
		Results[0].AvgFinalShanten);

	return Results;
}



	void FMahjongMonteCarloSimulator::EvaluateDiscardsAsync(
	FMonteCarloInput Input,
	int32 NumSimulations,
	TFunction<void(TArray<FMonteCarloResult>)> OnComplete)
{
	Async(EAsyncExecution::ThreadPool,
		[Input = MoveTemp(Input), NumSimulations,
		OnComplete = MoveTemp(OnComplete)]()
		{
			TArray<FMonteCarloResult> Results =
				EvaluateDiscards(Input, NumSimulations);

			AsyncTask(ENamedThreads::GameThread,
				[Results = MoveTemp(Results), OnComplete]()
				{
					OnComplete(Results);
				});
		});
}





FTileData FMahjongMonteCarloSimulator::IndexToTile(int32 idx)
{
	if (idx < 9) return FTileData{ ETileSuit::Man, (uint8)(idx + 1) };
	if (idx < 18) return FTileData{ ETileSuit::Pin, (uint8)(idx - 8) };
	if (idx < 27) return FTileData{ ETileSuit::Sou, (uint8)(idx - 17) };
	return FTileData{ ETileSuit::Honor, (uint8)(idx - 26) };
}

int32 FMahjongMonteCarloSimulator::TileToIndex(const FTileData& Tile)
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

TArray<uint8> FMahjongMonteCarloSimulator::BuildUnknownPool34(const FMonteCarloInput Input, const TArray<uint8>& MyHand13)
{
	TArray<uint8> Pool;
	Pool.Init(4, 34); //full set

	for (int32 i = 0; i < 34; i++)
	{
		int32 Count = 4;
		Count -= (int32)MyHand13[i];
		if (Input.SeenTiles34.IsValidIndex(i))
			Count -= (int32)Input.SeenTiles34[i];
		Pool[i] = (uint8)FMath::Max(0, Count);
	}
	return Pool;
}

TPair<int32, int32> FMahjongMonteCarloSimulator::RunOneSimulation(const FMonteCarloInput& Input, const TArray<uint8>& MyHand13, TArray<int32> UnknownList, FRandomStream& Rand)
{
	const int32 N = Input.NumPlayers;
	const int32 Me = Input.MyPlayerIndex;

	if (UnknownList.Num() == 0)
		return { -1, 0 };

	// Shuffle unknown tiles
	for (int32 i = UnknownList.Num() - 1; i > 0; --i)
		UnknownList.Swap(i, Rand.RandRange(0, i));

	// Deal hands
	TArray<TArray<uint8>> Hands;
	Hands.SetNum(N);
	for (auto& H : Hands)
		H.Init(0, 34);

	Hands[Me] = MyHand13;

	int32 Cursor = 0;
	for (int32 p = 0; p < N; ++p)
	{
		if (p == Me) continue;

		const int32 Melds = Input.OpponentMeldCounts.IsValidIndex(p);
		const int32 HandSize = 13 - Melds * 3;

		for (int32 t = 0; t < HandSize && Cursor < UnknownList.Num(); ++t)
			Hands[p][UnknownList[Cursor++]]++;
	}

	// Remaining tiles become the wall
	TArray<int32> Wall;
	Wall.Reserve(UnknownList.Num() - Cursor);
	while (Cursor < UnknownList.Num())
		Wall.Add(UnknownList[Cursor++]);

	const int32 MaxWallUse = FMath::Min(Wall.Num(), Input.WallRemaining);

	int32 WallPos = 0;
	int32 Current = (Me + 1) % N;
	int32 MyTurnCount = 0;

	while (WallPos < MaxWallUse)
	{
		const int32 Drawn = Wall[WallPos++];
		Hands[Current][Drawn]++;

		// Tsumo check
		if (FShantenCalculator::Calculate(Hands[Current]) == -1)
			return { Current, MyTurnCount };

		// Discard
		const int32 DiscardIdx = GreedyDiscardIndex(Hands[Current]);
		Hands[Current][DiscardIdx]--;

		// Ron check — modify in place instead of copying
		for (int32 p = 0; p < N; ++p)
		{
			if (p == Current) continue;

			Hands[p][DiscardIdx]++;
			const bool bRon = (FShantenCalculator::Calculate(Hands[p]) == -1);
			Hands[p][DiscardIdx]--;

			if (bRon)
				return { p, MyTurnCount };
		}

		Current = (Current + 1) % N;
		if (Current == Me) ++MyTurnCount;
	}

	return { -1, MyTurnCount };
}

int32 FMahjongMonteCarloSimulator::GreedyDiscardIndex(TArray<uint8>& Hand34)
{
	int32 Best = -1;
	int32 BestShan = INT32_MAX;

	for (int32 i = 0; i < 34; ++i)
	{
		if (Hand34[i] == 0) continue;

		Hand34[i]--;
		const int32 Shan = FShantenCalculator::Calculate(Hand34);
		Hand34[i]++; // undo

		if (Shan < BestShan)
		{
			BestShan = Shan;
			Best = i;
			if (BestShan == -1) break;
		}
	}
	return Best >= 0 ? Best : 0;
}
