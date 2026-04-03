// Fill out your copyright notice in the Description page of Project Settings.


#include "ShantenCalculator.h"
#include <map>
#include "Async/ParallelFor.h"
#include "Async/Async.h"
#include "Async/AsyncWork.h"
#include <atomic>

std::atomic<bool> FShantenCalculator::bTablesInitialized{ false };

void FShantenCalculator::InitializeTablesAsync(TFunction<void()> OnComplete)
{
	Async(EAsyncExecution::ThreadPool,
		[Callback = MoveTemp(OnComplete)]()
		{
			InitializeTables();

			// TFunction is copyable, so the inner lambda can capture by value
			// without needing a second move.
			AsyncTask(ENamedThreads::GameThread,
				[Callback]() { Callback(); });
		});
}

void FShantenCalculator::InitializeTables()
{
	UE_LOG(LogTemp, Log, TEXT("Building shanten tables (parallel)..."));

	// 20 independent BFS runs: NumMelds(0-4) × HasPair(0-1) × IsSuit(0-1)
	// Flat encoding: Idx = NumMelds*4 + HasPair*2 + IsSuit
	// Each iteration writes to a unique ShantenTables[][][], no shared state.
	ParallelFor(5 * 2 * 2, [](int32 Idx)
		{
			const int32 NumMelds = Idx / 4;
			const int32 HasPair = (Idx % 4) / 2;
			const int32 IsSuit = Idx % 2;

			ShantenTables[NumMelds][HasPair][IsSuit] =
				BuildShantenTableForShape(NumMelds, HasPair != 0, IsSuit != 0);
		});

	// Release store: any thread that observes IsInitialized()==true is
	// guaranteed to also see the completed table data.
	bTablesInitialized.store(true, std::memory_order_release);
	DebugPrintTableStats();
	UE_LOG(LogTemp, Log, TEXT("Shanten tables ready."));
}



void FShantenCalculator::InitializeTablesOld()
{

	UE_LOG(LogTemp, Log, TEXT("	Building shanten tables..."));

	for (int32 NumMelds = 0; NumMelds <= 4; ++NumMelds)
	{
		for (int32 HasPair = 0; HasPair <= 1; ++HasPair)
		{
			bool bHasPair = (HasPair == 1);

			ShantenTables[NumMelds][HasPair][1] = BuildShantenTableForShape(NumMelds, static_cast<bool>(HasPair), true); // suit table
			ShantenTables[NumMelds][HasPair][0] = BuildShantenTableForShape(NumMelds, static_cast<bool>(HasPair), false); // honor table

			UE_LOG(LogTemp, Log, TEXT("  Built table [%d melds, %s pair] - Suit: %d entries, Honor: %d entries"),
				NumMelds, bHasPair ? TEXT("with") : TEXT("without"),
				ShantenTables[NumMelds][HasPair][1].Num(),
				ShantenTables[NumMelds][HasPair][0].Num());

		}
	}
}

bool FShantenCalculator::IsInitialized()
{
	return bTablesInitialized.load(std::memory_order_acquire);
}


int32 FShantenCalculator::Calculate(const TArray<uint8>& Tiles34)
{
	if (!IsInitialized())
	{
		InitializeTables();
		bTablesInitialized = true;
		UE_LOG(LogTemp, Log, TEXT("Shanten tables initialized."));
	}

	int32 StandardShanten = CalculateStandardShanten(Tiles34);
	int32 ChiitoitsuShanten = CalculateChiitoitsuShanten(Tiles34);
	int32 KokushiShanten = CalculateKokushiShanten(Tiles34);


	return FMath::Min3(StandardShanten, ChiitoitsuShanten, KokushiShanten);
}

TMap<FTileData, int32> FShantenCalculator::CalculateDiscardResult(const TArray<FTileData>& Hand)
{
	TMap<FTileData, int32> Results;

	// For each unique tile in hand
	TSet<FTileData> UniqueTiles(Hand);

	for (const FTileData& Tile : UniqueTiles)
	{
		// Create a copy of hand without this tile
		TArray<FTileData> TestHand = Hand;
		TestHand.RemoveSingle(Tile);

		// Calculate shanten
		int32 Shanten = Calculate(ToTile34Array(TestHand));
		Results.Add(Tile, Shanten);
	}

	return Results;
}

TArray<uint8> FShantenCalculator::ToTile34Array(const TArray<FTileData>& Tiles)
{
	TArray<uint8> Tiles34;
	Tiles34.Init(0, 34); // Initialize 34 elements to 0

	for (const FTileData& Tile : Tiles)
	{
		int32 Index = -1;
		switch (Tile.Suit)
		{
		case ETileSuit::Man:   Index = Tile.Value - 1; break;        // 0-8
		case ETileSuit::Pin:   Index = 9 + Tile.Value - 1; break;    // 9-17
		case ETileSuit::Sou:   Index = 18 + Tile.Value - 1; break;   // 18-26
		case ETileSuit::Honor: Index = 27 + Tile.Value - 1; break;   // 27-33
		}

		if (Index >= 0 && Index < 34)
		{
			Tiles34[Index]++;
		}
	}

	return Tiles34;
}


TArray<TArray<uint8>> FShantenCalculator::EnumerateWinningConfigsForShape(int32 NumMelds, bool bHasPair, bool bAllowSequences)
{
	TArray<TArray<uint8>> Results;
	TArray<uint8> EmptyConfig;
	EmptyConfig.Init(0, 9);

	BuildConfigsRecursive(EmptyConfig, NumMelds, bHasPair, 0, bAllowSequences, Results);

	return Results;
}



// Helper: Count total tiles in a configuration
static int32 GetTileCount(const TArray<uint8>& Config)
{
	int32 Total = 0;
	for (uint8 Count : Config)
	{
		Total += Count;
	}
	return Total;
}

// Helper: Check if configuration is valid
static bool IsValidConfiguration(const TArray<uint8>& Config)
{
	// Each tile type can have at most 4 copies
	for (uint8 Count : Config)
	{
		if (Count > 4)
			return false;
	}

	// Total tiles in one suit should be <= 13
	// (since a full hand is 14 tiles across all suits)
	int32 Total = GetTileCount(Config);
	return Total <= 13;
}


// Helper: Hash a config for faster lookups
static uint64 GetConfigHash(const TArray<uint8>& Config)
{
	uint64 Hash = 0;
	for (int32 i = 0; i < Config.Num() && i < 9; ++i)
	{
		Hash = Hash * 5 + Config[i];
	}
	return Hash;
}


TArray<TArray<uint8>> FShantenCalculator::GetAdjacentConfigs(const TArray<uint8> Config)
{
	TArray<TArray<uint8>> Neighbors;
	Neighbors.Reserve(18);

	// Add one tile at each position
	for (int32 i = 0; i < 9; ++i)
	{
		if (Config[i] < 4) // Can't exceed 4 copies
		{
			TArray<uint8> NewConfig = Config;
			NewConfig[i]++;

			// Only add if total tiles <= 13
			if (IsValidConfiguration(NewConfig))
			{
				Neighbors.Add(NewConfig);
			}
		}
	}

	// Remove one tile at each position
	for (int32 i = 0; i < 9; ++i)
	{
		if (Config[i] > 0)
		{
			TArray<uint8> NewConfig = Config;
			NewConfig[i]--;

			// Always valid when removing (can't go below 0)
			Neighbors.Add(NewConfig);
		}
	}

	return Neighbors;
}

void FShantenCalculator::BuildConfigsRecursive(TArray<uint8> CurrentConfig, int32 TripletsLeft,
	bool bPairNeeded, int32 StartPos, bool bAllowSequences, TArray<TArray<uint8>>& OutResults)
{
	// Base case: completed configuration
	if (TripletsLeft == 0 && !bPairNeeded)
	{
		OutResults.Add(CurrentConfig);
		return;
	}

	// Try placing groups starting from StartPos
	for (int32 pos = StartPos; pos < 9; ++pos)
	{
		// Try placing a triplet (3 of the same tile)
		if (TripletsLeft > 0 && CurrentConfig[pos] + 3 <= 4)
		{
			CurrentConfig[pos] += 3;
			BuildConfigsRecursive(CurrentConfig, TripletsLeft - 1, bPairNeeded, pos, bAllowSequences, OutResults);
			CurrentConfig[pos] -= 3;
		}

		// Try placing a sequence (only if allowed and position permits)
		if (bAllowSequences && TripletsLeft > 0 && pos + 2 < 9
			&& CurrentConfig[pos] + 1 <= 4
			&& CurrentConfig[pos + 1] + 1 <= 4
			&& CurrentConfig[pos + 2] + 1 <= 4)
		{
			CurrentConfig[pos]++;
			CurrentConfig[pos + 1]++;
			CurrentConfig[pos + 2]++;
			BuildConfigsRecursive(CurrentConfig, TripletsLeft - 1, bPairNeeded, pos, bAllowSequences, OutResults);
			CurrentConfig[pos]--;
			CurrentConfig[pos + 1]--;
			CurrentConfig[pos + 2]--;
		}

		if (bPairNeeded && CurrentConfig[pos] + 2 <= 4)
		{
			CurrentConfig[pos] += 2;
			BuildConfigsRecursive(CurrentConfig, TripletsLeft, false, pos + 1, bAllowSequences, OutResults);
			CurrentConfig[pos] -= 2;
		}
	}
}

TMap<TArray<uint8>, int32> FShantenCalculator::BuildShantenTableForShape(int32 NumMelds, bool bHasPair,bool bAllowSequences)
{
	TMap<TArray<uint8>, int32> Table;
	TQueue<TArray<uint8>> Queue;

	// Get winning configurations
	TArray<TArray<uint8>> WinningConfigs = EnumerateWinningConfigsForShape(
		NumMelds, bHasPair, bAllowSequences);

	// Initialize winning configs
	for (const TArray<uint8>& Config : WinningConfigs)
	{
		Table.Add(Config, 0);
		Queue.Enqueue(Config);
	}

	// BFS with additional optimizations
	TSet<uint64> VisitedHashes; // Track visited configs by hash for speed

	while (!Queue.IsEmpty())
	{
		TArray<uint8> CurrentConfig;
		Queue.Dequeue(CurrentConfig);

		int32 CurrentShanten = Table[CurrentConfig];

		// Optimization 1: Stop at max shanten
		if (CurrentShanten >= 8)
			continue;

		TArray<TArray<uint8>> Neighbors = GetAdjacentConfigs(CurrentConfig);

		for (const TArray<uint8>& Neighbor : Neighbors)
		{
			// Optimization 2: Hash check before full map lookup
			uint64 Hash = GetConfigHash(Neighbor);

			if (VisitedHashes.Contains(Hash))
				continue;

			if (!Table.Contains(Neighbor))
			{
				Table.Add(Neighbor, CurrentShanten + 1);
				Queue.Enqueue(Neighbor);
				VisitedHashes.Add(Hash);
			}
		}
	}

	return Table;
}

int32 FShantenCalculator::CalculateStandardShanten(const TArray<uint8>& Tiles34)
{
	// Split into 4 suits
	TArray<uint8> ManTiles, PinTiles, SouTiles, HonorTiles;
	ManTiles.Append(&Tiles34[0], 9);    // Indices 0-8
	PinTiles.Append(&Tiles34[9], 9);    // Indices 9-17
	SouTiles.Append(&Tiles34[18], 9);   // Indices 18-26
	HonorTiles.Append(&Tiles34[27], 7); // Indices 27-33
	HonorTiles.Add(0); // Pad to 9 for consistency
	HonorTiles.Add(0);

	int32 MinBFSSum = INT32_MAX; // Maximum possible shanten

	// Try all distributions of 4 melds across 4 suits
	// Using stars and bars: distribute 4 identical items into 4 bins
	for (int32 m0 = 0; m0 <= 4; ++m0)           // Man melds
	{
		for (int32 m1 = 0; m1 <= 4 - m0; ++m1)  // Pin melds
		{
			for (int32 m2 = 0; m2 <= 4 - m0 - m1; ++m2) // Sou melds
			{
				int32 m3 = 4 - m0 - m1 - m2;    // Honor melds

				// Try pair in each suit (or no pair for incomplete hands)
				for (int32 PairSuit = -1; PairSuit < 4; ++PairSuit)
				{
					int32 BFSSum = CalculateShantenForDistribution(
						ManTiles, PinTiles, SouTiles, HonorTiles,
						m0, m1, m2, m3, PairSuit);

					MinBFSSum = FMath::Min(MinBFSSum, BFSSum);
				}
			}
		}
	}

	if (MinBFSSum == 0) return -1;
	if (MinBFSSum == INT32_MAX) return 8;


	return (MinBFSSum - 1) / 2;
}

int32 FShantenCalculator::CalculateChiitoitsuShanten(const TArray<uint8>& Tiles34)
{
	// Chitoitsu requires 7 pairs
	int32 Pairs = 0;
	int32 UniqueTypes = 0;

	for (int32 i = 0; i < 34; ++i)
	{
		if (Tiles34[i] >= 2)
		{
			Pairs++;
		}
		if (Tiles34[i] >= 1)
		{
			UniqueTypes++;
		}
	}

	// Shanten = 6 - pairs, but need at least 7 unique tile types
	int32 Shanten = 6 - Pairs;
	if (UniqueTypes < 7)
	{
		Shanten = FMath::Max(Shanten, 7 - UniqueTypes);
	}

	return Shanten;
}

int32 FShantenCalculator::CalculateKokushiShanten(const TArray<uint8>& Tiles34)
{
	// Kokushi requires one of each terminal/honor: 1m,9m,1p,9p,1s,9s + all 7 honors
	TArray<int32> KokushiIndices = { 0, 8, 9, 17, 18, 26, 27, 28, 29, 30, 31, 32, 33 };

	int32 UniqueTypes = 0;
	bool bHasPair = false;

	for (int32 Index : KokushiIndices)
	{
		if (Tiles34[Index] >= 1)
		{
			UniqueTypes++;
			if (Tiles34[Index] >= 2)
			{
				bHasPair = true;
			}
		}
	}

	// Shanten = 13 - unique_types - (has_pair ? 1 : 0)
	return 13 - UniqueTypes - (bHasPair ? 1 : 0);
}

int32 FShantenCalculator::CalculateShantenForDistribution(const TArray<uint8>& ManTiles, const TArray<uint8>& PinTiles, const TArray<uint8>& SouTiles, const TArray<uint8>& HonorTiles, int32 ManMelds, int32 PinMelds, int32 SouMelds, int32 HonorMelds, int32 PairSuit)
{
	int32 TotalShanten = 0;
	bool bManHasPair = (PairSuit == 0);
	TotalShanten += GetShantenForShape(ManTiles, ManMelds, bManHasPair, true);

	bool bPinHasPair = (PairSuit == 1);
	TotalShanten += GetShantenForShape(PinTiles, PinMelds, bPinHasPair, true);

	bool bSouHasPair = (PairSuit == 2);
	TotalShanten += GetShantenForShape(SouTiles, SouMelds, bSouHasPair, true);

	bool bHonorHasPair = (PairSuit == 3);
	TotalShanten += GetShantenForShape(HonorTiles, HonorMelds, bHonorHasPair, false);

	return TotalShanten;
}

int32 FShantenCalculator::GetShantenForShape(const TArray<uint8>& TileConfig, int32 NumMelds, bool bHasPair, bool bAllowSequences)
{
	if (NumMelds < 0 || NumMelds > 4)
	{
		return 8;
	}

	int32 PairIndex = bHasPair ? 1 : 0;
	int32 SeqIndex = bAllowSequences ? 1 : 0;

	const TMap<TArray<uint8>, int32>& Table = ShantenTables[NumMelds][PairIndex][SeqIndex];

	const int32* FoundShanten = Table.Find(TileConfig);

	if (FoundShanten)
	{
		return *FoundShanten;
	}


	return 8;
}


void FShantenCalculator::DebugPrintTableStats()
{
	UE_LOG(LogTemp, Log, TEXT("=== Shanten Table Statistics ==="));

	int64 TotalEntries = 0;
	int64 TotalMemoryBytes = 0;

	for (int32 NumMelds = 0; NumMelds <= 4; ++NumMelds)
	{
		for (int32 HasPair = 0; HasPair <= 1; ++HasPair)
		{
			int32 SuitEntries = ShantenTables[NumMelds][HasPair][1].Num();
			int32 HonorEntries = ShantenTables[NumMelds][HasPair][0].Num();

			UE_LOG(LogTemp, Log, TEXT("[%d melds, %s pair] Suit: %d, Honor: %d"),
				NumMelds,
				HasPair ? TEXT("with") : TEXT("without"),
				SuitEntries,
				HonorEntries);

			TotalEntries += SuitEntries + HonorEntries;
		}
	}

	// Rough memory estimate (TMap has overhead)
	TotalMemoryBytes = TotalEntries * 32; // ~32 bytes per entry with overhead

	UE_LOG(LogTemp, Log, TEXT("Total entries: %lld"), TotalEntries);
	UE_LOG(LogTemp, Log, TEXT("Estimated memory: %.2f MB"),
		TotalMemoryBytes / (1024.0f * 1024.0f));
}