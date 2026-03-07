// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HandAnalyzer.h"
#include "MahjongPlayerState.h"
#include "MahjongPlayer.generated.h"

UENUM(BlueprintType)
enum class EAIDifficulty : uint8
{
	Easy,      // Just discards random tiles
	Medium,    // Uses shanten calculation
	Hard       // Uses Monte Carlo + tile efficiency
};

UCLASS()
class MAHJONG_API AMahjongPlayer : public AAIController
{
	GENERATED_BODY()
	

public:
	AMahjongPlayer();


	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	FTileData DecideDiscard();

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	bool ShouldCallChi(const FTileData& DiscardedTile);

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	bool ShouldCallPon(const FTileData& DiscardedTile);

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	bool ShouldCallKan(const FTileData& DiscardedTile);

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	bool ShouldDeclareRiichi();

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	bool ShouldDeclareRon(const FTileData& DiscardedTile);

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	bool ShouldDeclareTsumo();

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	void SetDifficulty(EAIDifficulty NewDifficulty) { Difficulty = NewDifficulty; };

	UFUNCTION(BlueprintCallable, Category = "Mahjong AI")
	void SetThinkingTime(float Seconds) { ThinkingTime = Seconds; };

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float) override;

	AMahjongPlayerState* GetMahjongPlayerState() const;

	FTileData DecideDiscardEasy();
	FTileData DecideDiscardMedium();
	FTileData DecideDiscardHard();


	float EvaluateHandMonteCarlo(const TArray<FTileData>& Hand, int32 NumSimulations = 1000);

private:
	UPROPERTY(EditAnywhere, Category = "AI")
	EAIDifficulty Difficulty;
	UPROPERTY(EditAnywhere, Category = "AI")
	float ThinkingTime;

	IHandEvaluator* HandEvaluator;
};
