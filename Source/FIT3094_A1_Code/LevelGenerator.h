// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GridNode.h"
#include "Ship.h"
#include "GameFramework/Actor.h"
#include "LevelGenerator.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(IndividualShips, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(Heuristics, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(Collisions, Warning, All);

UCLASS()
class FIT3094_A1_CODE_API ALevelGenerator : public AActor
{
	GENERATED_BODY()

public:
	
	// Sets default values for this actor's properties
	ALevelGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	static const int MAX_MAP_SIZE = 200;
	static const int GRID_SIZE_WORLD = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int MapSizeX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int MapSizeY;
	UPROPERTY(EditAnywhere)
		TArray<AActor*> Goals;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> DeepBlueprint;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> LandBlueprint;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> ShallowBlueprint;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> GoldBlueprint;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> ShipBlueprint;
	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> PathDisplayBlueprint;
	UPROPERTY(EditAnywhere)
		AActor* Camera;

	UPROPERTY(EditAnywhere, Category = "Debugging")
		bool CollisionAndReplanning;
	UPROPERTY(EditAnywhere, Category = "Debugging")
		bool IndividualStats = false;

	bool CameraRotated = false;

	GridNode* WorldArray[MAX_MAP_SIZE][MAX_MAP_SIZE];
	TArray<AActor*> PathDisplayActors;
	TArray<AActor*> Terrain;
	TArray<FVector2d> ShipSpawns;
	TArray<FVector2d> GoldSpawns;
	TArray<AShip*> Ships;

	TArray<int> PathCostTaken;
	int SearchCount = 0;
	int CrashPenalty = 0;

	int ScenarioIndex = 0;
	int TotalIndex = 200;
	int Scenarios [7] = {1, 2, 5, 10, 25, 50, 100};
	bool FinishedScenarios = false;
	int PreviousPlannedCost = 1;
	
	

	void SpawnWorldActors(TArray<TArray<char>> Grid);
	void GenerateNodeGrid(TArray<TArray<char>> Grid);
	void ResetAllNodes();
	float CalculateDistanceBetween(GridNode* First, GridNode* Second);
	void GenerateWorldFromFile(TArray<FString> WorldArrayStrings);
	void GenerateScenarioFromFile(TArray<FString> ScenarioArrayStrings);
	void InitialisePaths();
	void RenderPath(AShip* Ship);
	void ResetPath();
	void DetailPlan();
	void DetailActual();
	void NextLevel();
	void DestroyAllActors();
	
	void CalculatePath();
	void Replan(AShip* Ship);

	//Addational Function
	void GetNeighbours(GridNode* CurrNode, GridNode* TargetLocation);
	float GetDistance(GridNode* CurrNode, GridNode* GoalLocation);
	TArray<GridNode*> OpenList;
	TArray<GridNode*> CloseList;
	GridNode* FindMinNode();
	
	

};


