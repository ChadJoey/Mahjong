// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TileTypes.generated.h"

UENUM(BlueprintType)
enum class ETileSuit : uint8 
{
	Man,      // Characters
	Pin,      // Circles  
	Sou,      // Bamboo
	Honor     // Winds + Dragons
};

USTRUCT(BlueprintType)
struct MAHJONG_API FTileData
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETileSuit Suit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	uint8 Value; // 1-9 for suits, 1-7 for honors

	FTileData() : Suit(ETileSuit::Man), Value(1) {};

	FTileData(ETileSuit InSuit, uint8 InValue) : Suit(InSuit), Value(InValue) {}


	int32 GetTileId() const 
	{
		return static_cast<int32>(Suit) * 10 + Value;
	}

	bool operator==(const FTileData& Other) const {
		return Suit == Other.Suit && Value == Other.Value;
	}
};


FORCEINLINE uint32 GetTypeHash(const FTileData& Tile)
{
	return HashCombine(GetTypeHash(static_cast<uint8>(Tile.Suit)), GetTypeHash(Tile.Value));
}