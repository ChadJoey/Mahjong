// Fill out your copyright notice in the Description page of Project Settings.


#include "ShantenTestActor.h"
#include "ShantenCalculator.h"

// Sets default values
AShantenTestActor::AShantenTestActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}



static FString GetSuitStr(const FTileData& Tile)
{
	switch (Tile.Suit)
	{
	case ETileSuit::Man:   return TEXT("m");
	case ETileSuit::Pin:   return TEXT("p");
	case ETileSuit::Sou:   return TEXT("s");
	case ETileSuit::Honor: return TEXT("z");
	default:               return TEXT("?");
	}
}


void AShantenTestActor::RunShantenTest()
{
	if (TestHand.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShantenTest] TestHand is empty. add tiles in the Details panel"))
	}

	FString HandStr;
	for (const FTileData& Tile : TestHand)
	{
		HandStr += FString::Printf(TEXT("%d%s "), Tile.Value, *GetSuitStr(Tile));
	}

	TArray<uint8> Tiles34 = FShantenCalculator::ToTile34Array(TestHand);
	int32 Shanten = FShantenCalculator::Calculate(Tiles34);

	FString Interpretation;
	if (Shanten == -1) Interpretation = TEXT("Complete hand (winning)");
	if (Shanten == 0) Interpretation = TEXT("Tenpai (one tile from winning)");
	else Interpretation = FString::Printf(TEXT("%d tiles from tenpai"), Shanten);


	UE_LOG(LogTemp, Log, TEXT("[ShantenTest] Hand	 :%s"), *HandStr);
	UE_LOG(LogTemp, Log, TEXT("[ShantenTest] Shanten :%d - %s"), Shanten, *Interpretation);

	TArray<FTileData> Hand14 = TestHand;
	Hand14.Add(DrawnTile);

	FString DrawnSuit = GetSuitStr(DrawnTile);
	UE_LOG(LogTemp, Log, TEXT("[ShantenTest] Drew         : %d%s"),
		DrawnTile.Value, *DrawnSuit);
	UE_LOG(LogTemp, Log, TEXT("[ShantenTest] Discard analysis (best first):"));


	TMap<FTileData, int32> DiscardResults = FShantenCalculator::CalculateDiscardResult(Hand14);

	TArray<TPair<FTileData, int32>> Sorted = DiscardResults.Array();
	Sorted.Sort([](const TPair<FTileData, int32>& A, const TPair<FTileData, int32>& B)
		{
			return A.Value < B.Value;
		});

	UE_LOG(LogTemp, Log, TEXT("[ShantenTest] Discard analysis (best first):"));
	for (const auto& Pair : Sorted)
	{
		FString PairSuit = GetSuitStr(Pair.Key);
		UE_LOG(LogTemp, Log, TEXT("[ShantenTest]   Discard %d%s -> shanten %d"), Pair.Key.Value, *PairSuit, Pair.Value);
	}
}

// Called when the game starts or when spawned
void AShantenTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (!FShantenCalculator::IsInitialized())
		FShantenCalculator::InitializeTables();

	RunShantenTest();
	
}

// Called every frame
void AShantenTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

