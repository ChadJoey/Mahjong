// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TileTypes.h"
/**
 * 
 */


class MAHJONG_API FShantenCalculator
{
public:


	static std::atomic<bool> bTablesInitialized;
	void InitializeTablesAsync(TFunction<void()> OnComplete);

	static void InitializeTables();
	static bool IsInitialized();

	void InitializeTablesOld();

	static int32 Calculate(const TArray<uint8>& Tiles34);
	static TMap<FTileData, int32> CalculateDiscardResult(const TArray<FTileData>& Hand);

	// Convert to 34-tile array representation [0-8 man, 9-17 pin, 17-26 sou, 27-33 honors.]
	static TArray<uint8> ToTile34Array(const TArray<FTileData>& Tiles);


private:

	static TMap<TArray<uint8>, int32> BuildShantenTableForShape(int32 NemMelds,bool bHasPair ,bool bAllowSequences);

	
	static int32 CalculateStandardShanten(const TArray<uint8>& Tiles34);// Standard shanten algorithm
	static int32 CalculateChiitoitsuShanten(const TArray<uint8>& Tiles34);	// Seven pairs
	static int32 CalculateKokushiShanten(const TArray<uint8>& Tiles34);	// Kokushi


	static int32 CalculateShantenForDistribution(
		const TArray<uint8>& ManTiles, const TArray<uint8>& PinTiles,
		const TArray<uint8>& SouTiles, const TArray<uint8>& HonorTiles,
		int32 ManMelds, int32 PinMelds, int32 SouMelds, int32 HonorMelds,
		int32 PairSuit);

	static int32 GetShantenForShape(const TArray<uint8>& TileConfig, int32 NumMelds, bool bHasPair, bool bAllowSequences);

	static TArray<TArray<uint8>> EnumerateWinningConfigsForShape(int32 NumMelds, bool bHasPair, bool bAllowSequences);
	static TArray<TArray<uint8>> GetAdjacentConfigs(const TArray<uint8> configs);
	static void BuildConfigsRecursive(TArray<uint8> CurrentConfig, int32 TripletsLeft, bool bPairNeeded, int32 StartPos,bool bAllowSequences, TArray<TArray<uint8>>& OutResults);

	static void DebugPrintTableStats();


};


static TMap<TArray<uint8>, int32> ShantenTables[5][2][2];