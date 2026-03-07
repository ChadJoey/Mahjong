// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TileTypes.h"
#include "ShantenCalculator.h"
#include "HandAnalyzer.generated.h"

/**
 * 
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UHandEvaluator : public UInterface
{
    GENERATED_BODY()
};

class IHandEvaluator
{
    GENERATED_BODY()

public:
    // Evaluate hand for AI decision making
    virtual float EvaluateHand(const TArray<FTileData>& Hand) = 0;

    // Get best discard based on evaluation
    virtual FTileData GetBestDiscard(const TArray<FTileData>& Hand) = 0;
};

UCLASS()
class UShantenEvaluator : public UObject, public IHandEvaluator
{
    GENERATED_BODY()

public:
    virtual float EvaluateHand(const TArray<FTileData>& Hand) override
    {

        // Lower shanten = better hand
        return -FShantenCalculator::Calculate(FShantenCalculator::ToTile34Array(Hand));
    }

    virtual FTileData GetBestDiscard(const TArray<FTileData>& Hand) override;
};