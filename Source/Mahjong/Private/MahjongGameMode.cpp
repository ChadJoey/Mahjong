// Fill out your copyright notice in the Description page of Project Settings.


#include "MahjongGameMode.h"
#include "MahjongGameStateBase.h"

AMahjongGameMode::AMahjongGameMode()
{
	GameStateClass = AMahjongGameStateBase::StaticClass();
	PlayerStateClass = AMahjongPlayerState::StaticClass();

	AIControllerClass = AMahjongPlayer::StaticClass();
	MahjongPlayerStateClass = AMahjongPlayerState::StaticClass();
}

void AMahjongGameMode::BeginPlay()
{
    Super::BeginPlay();


    // Initialize shanten calculator tables (do this ONCE at game start)
    if (!FShantenCalculator::IsInitialized())
    {
        FShantenCalculator::InitializeTables();
    }

    // Auto-initialize game
    InitializeMahjongGame();
}

void AMahjongGameMode::InitializeMahjongGame()
{
    UE_LOG(LogTemp, Log, TEXT("Initializing Mahjong game..."));

    // Spawn 4 AI players
    SpawnAIPlayers(4);

    // Start first round
    StartNewRound();
}

void AMahjongGameMode::SpawnAIPlayers(int32 NumAi)
{
}

void AMahjongGameMode::StartNewRound()
{
}

TArray<AMahjongPlayerState*> AMahjongGameMode::GetAllMahjongPlayers() const
{
	return TArray<AMahjongPlayerState*>();
}
