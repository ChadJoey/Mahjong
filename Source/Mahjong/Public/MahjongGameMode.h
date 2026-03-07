// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MahjongPlayer.h"
#include "MahjongGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MAHJONG_API AMahjongGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMahjongGameMode();

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void InitializeMahjongGame();

	UFUNCTION()
	void SpawnAIPlayers(int32 NumAi = 4);

	UFUNCTION()
	void StartNewRound();

	UFUNCTION()
	TArray<AMahjongPlayerState*> GetAllMahjongPlayers() const;

	UFUNCTION()
	TArray<AMahjongPlayer*> GetAIControllers() const { return AIControllers; };

private:

	UPROPERTY()
	TArray<AMahjongPlayer*> AIControllers;


	// Classes to spawn (set in editor or defaults)
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AMahjongPlayer> AIControllerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AMahjongPlayerState> MahjongPlayerStateClass;


};
