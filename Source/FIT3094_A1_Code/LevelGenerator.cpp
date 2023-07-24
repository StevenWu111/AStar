// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelGenerator.h"

#include "FIT3094_A1_CodeGameModeBase.h"
#include "Ship.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(IndividualShips);
DEFINE_LOG_CATEGORY(Heuristics);
DEFINE_LOG_CATEGORY(Collisions)

// Sets default values
ALevelGenerator::ALevelGenerator()
{
 	// Set this actor to call Tick() every frame. 
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevelGenerator::BeginPlay()
{
	Super::BeginPlay();

	AFIT3094_A1_CodeGameModeBase* GameModeBase = Cast<AFIT3094_A1_CodeGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()));
	GenerateWorldFromFile(GameModeBase->GetMapArray(GameModeBase->GetAssessedMapFile()));
	GenerateScenarioFromFile(GameModeBase->GetMapArray(GameModeBase->GetScenarioFile()));
	NextLevel();
	
}

// Called every frame
void ALevelGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	int ShipsAtGoal = 0;

	for(int i = 0; i < Ships.Num(); i++)
	{
		if(Ships[i]->AtGoal)
		{
			ShipsAtGoal++;
		}
	}

	if(ShipsAtGoal == Ships.Num())
	{
		NextLevel();
	}
}

void ALevelGenerator::SpawnWorldActors(TArray<TArray<char>> Grid)
{
	if(DeepBlueprint && ShallowBlueprint && LandBlueprint)
	{
		for(int Y = 0; Y < MapSizeY; Y++)
		{
			for(int X = 0; X < MapSizeX; X++)
			{
				float XPos = X * GRID_SIZE_WORLD;
				float YPos = Y * GRID_SIZE_WORLD;

				FVector Position(XPos, YPos, 0);

				switch(Grid[Y][X])
				{
				case '.':
					Terrain.Add(GetWorld()->SpawnActor(DeepBlueprint, &Position));
					break;
				case 'T':
					Terrain.Add(GetWorld()->SpawnActor(ShallowBlueprint, &Position));
					break;
				case '@':
					Terrain.Add(GetWorld()->SpawnActor(LandBlueprint, &Position));
					break;
				default:
					break;
				}
			}
		}
	}

	if(Camera)
	{
		FVector CameraPosition = Camera->GetActorLocation();

		CameraPosition.X = MapSizeX * 0.5 * GRID_SIZE_WORLD;
		CameraPosition.Y = MapSizeY * 0.5 * GRID_SIZE_WORLD;

		if(!CameraRotated)
		{
			CameraRotated = true;
			FRotator CameraRotation = Camera->GetActorRotation();

			CameraRotation.Pitch = 270;
			CameraRotation.Roll = 180;

			Camera->SetActorRotation(CameraRotation);
			Camera->AddActorLocalRotation(FRotator(0,0,90));
		}
		
		Camera->SetActorLocation(CameraPosition);
		
	}
}

void ALevelGenerator::GenerateNodeGrid(TArray<TArray<char>> Grid)
{
	for(int Y = 0; Y < MapSizeY; Y++)
	{
		for(int X = 0; X < MapSizeX; X++)
		{
			WorldArray[Y][X] = new GridNode();
			WorldArray[Y][X]->Y = Y;
			WorldArray[Y][X]->X = X;

			switch(Grid[Y][X])
			{
			case '.':
				WorldArray[Y][X]->GridType = GridNode::DeepWater;
				break;
			case '@':
				WorldArray[Y][X]->GridType = GridNode::Land;
				break;
			case 'T':
				WorldArray[Y][X]->GridType = GridNode::ShallowWater;
				break;
			default:
				break;
			}
			
		}
	}
}

void ALevelGenerator::ResetAllNodes()
{
	for( int Y = 0; Y < MapSizeY; Y++)
	{
		for(int X = 0; X < MapSizeX; X++)
		{
			WorldArray[Y][X]->F = 0;
			WorldArray[Y][X]->G = 0;
			WorldArray[Y][X]->H = 0;
			WorldArray[Y][X]->Parent = nullptr;
			WorldArray[Y][X]->ObjectAtLocation = nullptr;
		}
	}
}

float ALevelGenerator::CalculateDistanceBetween(GridNode* First, GridNode* Second)
{
	FVector DistToTarget = FVector(Second->X - First->X,Second->Y - First->Y, 0);
	return DistToTarget.Size();
}

void ALevelGenerator::GenerateWorldFromFile(TArray<FString> WorldArrayStrings)
{
	if(WorldArrayStrings.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Map file not found!"))
		return;
	}

	FString Height = WorldArrayStrings[1];
	Height.RemoveFromStart("height ");
	MapSizeY = FCString::Atoi(*Height);

	FString Width = WorldArrayStrings[2];
	Width.RemoveFromStart("width ");
	MapSizeX = FCString::Atoi(*Width);

	TArray<TArray<char>> CharMapArray;
	CharMapArray.Init( TArray<char>(), MAX_MAP_SIZE);
	
	for(int i = 0; i < CharMapArray.Num(); i++)
	{
		CharMapArray[i].Init('x', MAX_MAP_SIZE);
	}
	
	for(int LineNum = 4; LineNum < MapSizeY + 4; LineNum++)
	{
		for(int CharNum = 0; CharNum < MapSizeX; CharNum++)
		{
			CharMapArray[LineNum-4][CharNum] = WorldArrayStrings[LineNum][CharNum];
		}
	}

	GenerateNodeGrid(CharMapArray);
	SpawnWorldActors(CharMapArray);
	
}

void ALevelGenerator::GenerateScenarioFromFile(TArray<FString> ScenarioArrayStrings)
{
	if(ScenarioArrayStrings.Num() == 0)
	{
		return;
	}
	
	for(int i = 1; i < ScenarioArrayStrings.Num(); i++)
	{
		TArray<FString> SplitLine;
		FString CurrentLine = ScenarioArrayStrings[i];
		
		CurrentLine.ParseIntoArray(SplitLine,TEXT("\t"));

		int ShipX = FCString::Atoi(*SplitLine[4]);
		int ShipY = FCString::Atoi(*SplitLine[5]);
		int GoldX = FCString::Atoi(*SplitLine[6]);
		int GoldY = FCString::Atoi(*SplitLine[7]);

		ShipSpawns.Add(FVector2d(ShipX, ShipY));
		GoldSpawns.Add(FVector2d(GoldX, GoldY));
	}
}

void ALevelGenerator::InitialisePaths()
{
	ResetPath();
	CalculatePath();
	DetailPlan();
}

void ALevelGenerator::RenderPath(AShip* Ship)
{
	GridNode* CurrentNode = Ship->GoalNode;

	if(CurrentNode)
	{
		while(CurrentNode->Parent != nullptr)
		{
			FVector Position(CurrentNode->X * GRID_SIZE_WORLD, CurrentNode->Y * GRID_SIZE_WORLD, 10);
			AActor* PathActor = GetWorld()->SpawnActor(PathDisplayBlueprint, &Position);
			PathDisplayActors.Add(PathActor);

			Ship->Path.EmplaceAt(0, WorldArray[CurrentNode->Y][CurrentNode->X]);
			CurrentNode = CurrentNode->Parent;
		}
	}
	
	
}

void ALevelGenerator::ResetPath()
{
	SearchCount = 0;
	ResetAllNodes();

	for(int i = 0; i < PathDisplayActors.Num(); i++)
	{
		PathDisplayActors[i]->Destroy();
	}
	PathDisplayActors.Empty();

	for(int i = 0; i < Ships.Num(); i++)
	{
		Ships[i]->Path.Empty();
	}
}

void ALevelGenerator::DetailPlan()
{
	int ShipPathLength = 0;
	
	for(int i = 0; i < Ships.Num(); i++)
	{
		ShipPathLength += Ships[i]->Path.Num();
	}
	
	int TotalPathCost = 0;
	
	for(int i = 0; i < Ships.Num(); i++)
	{
		int ShipPathCost = 0;
		
		for(int j = 1; j < Ships[i]->Path.Num(); j++)
		{
			TotalPathCost += Ships[i]->Path[j]->GetTravelCost();
		}
		
		if(IndividualStats)
		{
			UE_LOG(IndividualShips, Warning, TEXT("Ship %s has: Cells Searched: %d, Planned Path Cost: %d, Planned Path Action Amount: %d"), *Ships[i]->GetName(), Ships[i]->CellsSearched, ShipPathCost, Ships[i]->Path.Num());
		}
	}

	PreviousPlannedCost = TotalPathCost;
	
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red, FString::Printf(TEXT("Total Estimated Path Cost: %d"), TotalPathCost));
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red, FString::Printf(TEXT("Total Cells Expanded: %d with a Total Path Action Amount of: %d"), SearchCount, ShipPathLength));
	GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Red, FString::Printf(TEXT("CURRENT SCENARIO TOTAL")));
	
	UE_LOG(Heuristics, Warning, TEXT("CURRENT SCENARIO TOTAL"));
	UE_LOG(Heuristics, Warning, TEXT("Total Cells Expanded: %d with a total path length of: %d"), SearchCount, ShipPathLength);
	UE_LOG(Heuristics, Warning, TEXT("Total Estimated Path Cost: %d"), TotalPathCost);

}

void ALevelGenerator::DetailActual()
{
	if(CollisionAndReplanning)
	{
		int TotalPathCost = 0;
	
		for(int i = 0; i < PathCostTaken.Num(); i++)
		{
			TotalPathCost += PathCostTaken[i];
		}

		TotalPathCost += CrashPenalty;

		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("Ratio of Actual vs Planned: %fx"), (float)(TotalPathCost)/PreviousPlannedCost));
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("Actual Total Path Cost including Crashes & Replanning: %d"), TotalPathCost));
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("Actual Total Cells Expanded: %d with actual Total Path Action Amount of: %d"), SearchCount, PathCostTaken.Num()));
		GEngine->AddOnScreenDebugMessage(-1, 12.f, FColor::Blue, FString::Printf(TEXT("PREVIOUS SCENARIO")));
		
		UE_LOG(Heuristics, Warning, TEXT("PREVIOUS SCENARIO"));
		UE_LOG(Heuristics, Warning, TEXT("Actual Total Cells Expanded: %d with actual Total Path Action Amount of: %d"), SearchCount, PathCostTaken.Num());
		UE_LOG(Heuristics, Warning, TEXT("Actual Total Path Cost including Crashes & Replanning: %d"), TotalPathCost);
		UE_LOG(Heuristics, Warning, TEXT("Ratio of Actual vs Planned: %fx"), (float)(TotalPathCost)/PreviousPlannedCost);
	}

	CrashPenalty = 0;
	PathCostTaken.Empty();
}

void ALevelGenerator::NextLevel()
{
	DestroyAllActors();
	
	if(ScenarioIndex >= 7)
	{
		if(!FinishedScenarios)
		{
			DetailActual();
			FinishedScenarios = true;
			UE_LOG(LogTemp, Warning, TEXT("Completed!"));
		}
	}
	
	else
	{
		if(ScenarioIndex != 0)
		{
			DetailActual();
		}
		
		for(int i = 0; i < Scenarios[ScenarioIndex]; i++)
		{
			if(GoldBlueprint && ShipBlueprint)
			{
				int GoldXPos = GoldSpawns[i + TotalIndex].X;
				int GoldYPos = GoldSpawns[i + TotalIndex].Y;

				FVector GoldPosition(GoldXPos* GRID_SIZE_WORLD, GoldYPos* GRID_SIZE_WORLD, 20);
				AActor* Gold = GetWorld()->SpawnActor(GoldBlueprint, &GoldPosition);
				
				Goals.Add(Gold);

				int ShipXPos = ShipSpawns[i + TotalIndex].X;
				int ShipYPos = ShipSpawns[i + TotalIndex].Y;

				FVector ShipPosition(ShipXPos* GRID_SIZE_WORLD, ShipYPos* GRID_SIZE_WORLD, 20);
				AShip* Ship = Cast<AShip>(GetWorld()->SpawnActor(ShipBlueprint, &ShipPosition));

				Ship->GoalNode = WorldArray[GoldYPos][GoldXPos];
				Ships.Add(Ship);
			}
		}
		
		TotalIndex += Scenarios[ScenarioIndex];
		ScenarioIndex++;
		InitialisePaths();
	}
}

void ALevelGenerator::DestroyAllActors()
{
	for(int i = 0; i < Goals.Num(); i++)
	{
		Goals[i]->Destroy();
	}
	Goals.Empty();
	
	for(int i = 0; i < PathDisplayActors.Num(); i++)
	{
		PathDisplayActors[i]->Destroy();
	}
	PathDisplayActors.Empty();
	
	for(int i = 0; i < Ships.Num(); i++)
	{
		Ships[i]->Destroy();
	}
	Ships.Empty();
	
}

//----------------------------------------------------------YOUR CODE-----------------------------------------------------------------------//

void ALevelGenerator::CalculatePath()
{
	//INSERT YOUR PATHFINDING ALGORITHM HERE
	//Loop through every ship in the level by using the for each loop
	for (const auto Ship:Ships)
	{
		ResetAllNodes();
		//Get the start node and end node
		const int StartLocationX = Ship->GetActorLocation().X/GRID_SIZE_WORLD;
		const int StartLocationY = Ship->GetActorLocation().Y/GRID_SIZE_WORLD;
		GridNode* GoalLocation = Ship->GoalNode;
		GridNode* StartNode = WorldArray[StartLocationY][StartLocationX];
		//Initialize the openlist by add start node in it.
		OpenList.Add(StartNode);
		//Main loop of the search algorithms 
		while (!OpenList.IsEmpty())
		{
			//get the node that has minimum f value.
			//The FindMinNode is used to search the node that has lowest f value in the open list. And return that node
			GridNode* CurrentNode = FindMinNode();
			//update open and close list
			CloseList.Add(CurrentNode);
			OpenList.Remove(CurrentNode);
			//Check to see the current node is the target node or not. This will break the loop if it reach the target node.
			if (CurrentNode == GoalLocation)
			{
				break;
			}
			//This if condition is used to prevent crash
			if (CurrentNode)
			{
				//GetNeighbours function take the input of a GridNode and send all the neighbours of it into the open list
				//Also I used this function to add the parents just after add them into open list as well as calculate g,h,f value and assign them into the variables in GridNode Class
				GetNeighbours(CurrentNode, GoalLocation);
			}
			//Add the search count
			SearchCount++;
		}
		//Clear the list because of both lists are the global variable. We need clear it before get into the next ship
		OpenList.Empty();
		CloseList.Empty();
		RenderPath(Ship);
	}
}
/*
 * Input:
 *			A GridNode that we want to search the neighbours of it
 *			A GridNode of goal location
 * Description:
 *			This function will store all the neighbours of the node that we provide into the OpenList and set the parent to the current node
 *			Also it will assign the f,g,h value into the that node
 */
void ALevelGenerator::GetNeighbours(GridNode* CurrNode, GridNode* TargetLocation)
{
	int X = CurrNode->X;
	int Y = CurrNode->Y;
	//check to see is it exceeded map size
	//Is it has been expended (in the close list
	//And because the land will cost 100 when our ship is travelled therefore I dont want my ship goes on the ground in any situation (cost < 100)
	//Node of X+1
	if (X + 1 < MapSizeX && !CloseList.Contains(WorldArray[Y][X+1]) && WorldArray[Y][X+1]->GetTravelCost() < 100)
	{
		GridNode* Neighbour = WorldArray[Y][X+1];
		OpenList.Add(Neighbour);
		Neighbour->Parent = CurrNode;
		if (Neighbour->Parent)
		{
			Neighbour->G = Neighbour->Parent->G + Neighbour->GetTravelCost();
			//GetDistance function is used to get the distance between this neighbour and the goal location by applied Manhattan Distance
			Neighbour->H = GetDistance(Neighbour,TargetLocation);
		}
		Neighbour->F = Neighbour->G + Neighbour->H;
	}
	//Node of X-1
	if (X - 1 >= 0 && !CloseList.Contains(WorldArray[Y][X-1]) && WorldArray[Y][X-1]->GetTravelCost() < 100)
	{
		GridNode* Neighbour = WorldArray[Y][X-1];
		OpenList.Add(Neighbour);
		Neighbour->Parent = CurrNode;
		if (Neighbour->Parent)
		{
			Neighbour->G = Neighbour->Parent->G + Neighbour->GetTravelCost();
			Neighbour->H = GetDistance(Neighbour,TargetLocation);
		}
		Neighbour->F = Neighbour->G + Neighbour->H;
	}
	//Node of Y+1
	if (Y + 1 < MapSizeY && !CloseList.Contains(WorldArray[Y+1][X]) && WorldArray[Y+1][X]->GetTravelCost() < 100)
	{
		GridNode* Neighbour = WorldArray[Y+1][X];
		OpenList.Add(Neighbour);
		Neighbour->Parent = CurrNode;
		if (Neighbour->Parent)
		{
			Neighbour->G = Neighbour->Parent->G + Neighbour->GetTravelCost();
			Neighbour->H = GetDistance(Neighbour,TargetLocation);
		}
		Neighbour->F = Neighbour->G + Neighbour->H;
	}
	//Node of Y-1
	if (Y - 1 >= 0 && !CloseList.Contains(WorldArray[Y-1][X]) && WorldArray[Y-1][X]->GetTravelCost() < 100)
	{
		GridNode* Neighbour = WorldArray[Y-1][X];
		OpenList.Add(Neighbour);
		Neighbour->Parent = CurrNode;
		if (Neighbour->Parent)
		{
			Neighbour->G = Neighbour->Parent->G + Neighbour->GetTravelCost();
			Neighbour->H = GetDistance(Neighbour,TargetLocation);
		}
		Neighbour->F = Neighbour->G + Neighbour->H;
	}
}

//Return the distance between two nodes by using the Manhattan distance
float ALevelGenerator::GetDistance(GridNode* CurrNode, GridNode* GoalLocation)
{
	const int EndX = GoalLocation->X;
	const int EndY = GoalLocation->Y;
	const int StartX = CurrNode->X;
	const int StartY = CurrNode->Y;
	//Get the distance between currNode and the goal location, use Manhattan Distance.
	//Used WA* which has weight of 2
	return 2 * (abs(EndX - StartX) + abs(EndY - StartY));
}

//Loop through all the nodes inside of OpenList and return the node that has the lowest f value
GridNode* ALevelGenerator::FindMinNode()
{
	GridNode* CurrMinNode = nullptr;
	if (!OpenList.IsEmpty())
	{
		//Initialzie MinF as Infinity which means every f value will lower than this, this can make sure we assign the useful f value later
		float MinF = INFINITY;
		for(auto Node: OpenList)
		{
			if (Node->F < MinF)
				{
					MinF = Node->F;
					CurrMinNode = Node;
				}
			
		}
	}
	return  CurrMinNode;

}


void ALevelGenerator::Replan(AShip* Ship)
{
	if(CollisionAndReplanning)
	{
		//INSERT REPLANNING HERE
		//Reset All Nodes in the current level
		ResetAllNodes();
		//Clear the Open and Close list, prepare for the replan (make sure they are empty)
		OpenList.Empty();
		CloseList.Empty();
		//Get the Node of start and end
		GridNode* GoalLocation = Ship->GoalNode;
		const int StartLocationX = Ship->GetActorLocation().X/GRID_SIZE_WORLD;
		const int StartLocationY = Ship->GetActorLocation().Y/GRID_SIZE_WORLD;
		GridNode* StartNode = WorldArray[StartLocationY][StartLocationX];
		//This is the node that this ship potentially going to crash
		GridNode* Crash = Ship->Path[0];
		//Clear the path of the current ship, prepare for the replan the path
		Ship->Path.Empty();
		//Start search with StartNode
		OpenList.Add(StartNode);
		//Main Loop, very similar to the calculate path function
		while (!OpenList.IsEmpty())
		{
			GridNode* CurrentNode = FindMinNode();
			CloseList.Add(CurrentNode);
			OpenList.Remove(CurrentNode);
			if (CurrentNode == GoalLocation)
			{
				break;
			}
			if (CurrentNode)
			{
				GetNeighbours(CurrentNode, GoalLocation);
			}
			//Creat RemoveNode variable to store the node that I want to remove.
			//I'm doing this because UE does not allow me to change the open list during the following for loop
			//So I need to remove this node from open list outside of the loop
			//Initialize it with nullptr
			GridNode* RemoveNode = nullptr;
			if (!OpenList.IsEmpty())
			{
				//Loop through all the neighbours that we added before. And sure it remove the node that contains an object or it is our potential crash node
				for (auto Neighbour:OpenList)
				{
					if (Neighbour == Crash || Neighbour->ObjectAtLocation)
					{
						Neighbour->Parent = nullptr;
						RemoveNode = Neighbour;
					}
				}
				if (RemoveNode)
				{
					OpenList.Remove(RemoveNode);
				}
			}
			//Increase the SearchCount
			SearchCount++;
		}
		//Clear both list and prepare for the next search
		OpenList.Empty();
		CloseList.Empty();
		//render the new Path
		RenderPath(Ship);
	}
}


