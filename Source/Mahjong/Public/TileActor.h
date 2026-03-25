// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileTypes.h"
#include "TileActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoveComplete);

UCLASS()
class MAHJONG_API ATileActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATileActor();

	UFUNCTION(BlueprintCallable, Category = "Tile")
	void InitTile(const FTileData& InTileData,UStaticMesh* InFrontMesh,UMaterialInterface* InFrontMaterial, UStaticMesh* InBackMes);

	UFUNCTION(BlueprintCallable, Category = "Tile")
	void SetFaceUp(bool bFaceUp);

	UFUNCTION(BlueprintCallable, Category = "Tile")
	void MoveTo(FTransform TargetTransform, float Duration = 0.25f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tile")
	FTileData TileData;

	UPROPERTY(BlueprintAssignable, Category = "Tile")
	FOnMoveComplete OnMoveComplete;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* TileFront;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* TileBack;

	// Movement interpolation
	bool       bIsMoving = false;
	float      MoveElapsed = 0.f;
	float      MoveDuration = 0.25f;
	FTransform MoveStart;
	FTransform MoveTarget;

};
