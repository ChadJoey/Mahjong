// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TileTypes.h"
#include "MahjongHandComponent.generated.h"


UCLASS( ClassGroup=(MahJong), meta=(BlueprintSpawnableComponent))
class MAHJONG_API UMahjongHandComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMahjongHandComponent();


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<FTileData> Hand;


	UPROPERTY()
	int32 CachedShanten;

	UPROPERTY()
	bool BisShantenDirty;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	int32 GetShanten();

	void AddTile(const FTileData& Tile);
	void RemoveTile(const FTileData& Tile);
	TArray<FTileData> GetOptimalDiscards();

	//DECLARE_DYNAMIC_MULTICAST_DELEGATE_ONEPARAM(FOnHandChanged, const TArray<FTileData>&, NewHand);
	//UPROPERTY(BlueprintAssignable)
	//FOnHandChanged OnHandChanged;
};
