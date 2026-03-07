// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TileTypes.h"
#include "MahjongPlayerState.generated.h"





UENUM(BlueprintType)
enum class EMeldType : uint8
{
	Chi,        // Sequence (1-2-3)
	Pon,        // Triplet (same tile x3)
	OpenKan,    // Open quad (called from discard)
	ClosedKan,  // Closed quad (from own hand)
	None
};


USTRUCT(BlueprintType)
struct MAHJONG_API FMeldData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FTileData> Tiles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMeldType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsClosed;

	FMeldData() : Type(EMeldType::None), bIsClosed(true) {}

	FMeldData(const TArray<FTileData>& InTiles, bool bInClosed = false)
		: Tiles(InTiles), bIsClosed(bInClosed) {
	}

	// Auto-detect meld type from tiles
	void DetectMeldType()
	{
		if (Tiles.Num() == 3)
		{
			if (Tiles[0] == Tiles[1] && Tiles[1] == Tiles[2])
			{
				Type = EMeldType::Pon;
			}
			else
			{
				Type = EMeldType::Chi;
			}
		}
		else if (Tiles.Num() == 4)
		{
			Type = bIsClosed ? EMeldType::ClosedKan : EMeldType::OpenKan;
		}
	}

	bool IsSequence() const { return Type == EMeldType::Chi; }
	bool IsTriplet() const { return Type == EMeldType::Pon; }
	bool IsQuad() const { return Type == EMeldType::OpenKan || Type == EMeldType::ClosedKan; }
};


UENUM(BlueprintType)
enum class EMahjongSeat : uint8
{
	East,
	South,
	West,
	North
};


UCLASS()
class MAHJONG_API AMahjongPlayerState : public APlayerState
{
	GENERATED_BODY()
public:


	AMahjongPlayerState();


	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	void AddTileToHand(const FTileData& Tile);

	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	void RemoveTileFromHand(const FTileData& Tile);
	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	TArray<FTileData> GetHand() const { return Hand; };
	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	int32 GetHandSize() const { return Hand.Num(); };

	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	void SortHand();

	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	EMahjongSeat GetSeat() const { return Seat; };

	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	void SetSeat(EMahjongSeat NewSeat) { Seat = NewSeat; };

	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	int32 GetPoints() const { return Points; };

	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	void AddPoints(int32 Amount) { Points += Amount; };

	// Revealed tiles (melds called from other players)
	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	void AddRevealedMeld(const FMeldData& Meld);

	UFUNCTION(BlueprintCallable, Category = "Mahjong")
	TArray<FMeldData> GetRevealedMelds() const { return RevealedMelds; };


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mahjong")
	TArray<FTileData> Hand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mahjong")
	TArray<FMeldData> RevealedMelds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mahjong")
	EMahjongSeat Seat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mahjong")
	int32 Points;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mahjong")
	bool bIsRiichi;

};
