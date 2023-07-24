// Fill out your copyright notice in the Description page of Project Settings.
#include "Ship.h"

#include "LevelGenerator.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AShip::AShip()
{
 	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	MoveSpeed = 500;
	Tolerance = MoveSpeed / 20;
	GoalNode = nullptr;
}

// Called when the game starts or when spawned
void AShip::BeginPlay()
{
	Super::BeginPlay();
	LevelGenerator = Cast<ALevelGenerator>(UGameplayStatics::GetActorOfClass(GetWorld(), ALevelGenerator::StaticClass()));
	GetComponents(UStaticMeshComponent::StaticClass(), Meshes);
}

// Called every frame
void AShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(Path.Num() > 0)
	{
		if(Path[0]->ObjectAtLocation != this && Path[0]->ObjectAtLocation != nullptr && PotentialCrash == nullptr)
		{
			PotentialCrash = Cast<AShip>(Path[0]->ObjectAtLocation);
			UE_LOG(LogTemp, Warning, TEXT("Ship %s has a potential crash with Ship %s!"), *this->GetName(), *Path[0]->ObjectAtLocation->GetName());
			if(LevelGenerator)
			{
				LevelGenerator->Replan(this);
				while (Path.Num()==0)
				{
					LevelGenerator->Replan(this);
				}
			}
		}
		else
		{
			Path[0]->ObjectAtLocation = this;
		}
		
		FVector CurrentPosition = GetActorLocation();

		float TargetXPos = Path[0]->X * ALevelGenerator::GRID_SIZE_WORLD;
		float TargetYPos = Path[0]->Y * ALevelGenerator::GRID_SIZE_WORLD;

		FVector TargetPosition(TargetXPos, TargetYPos, CurrentPosition.Z);

		Direction = TargetPosition - CurrentPosition;
		Direction.Normalize();

		CurrentPosition += Direction * MoveSpeed * DeltaTime;

		if(FVector::Dist(CurrentPosition, TargetPosition) <= Tolerance)
		{
			CurrentPosition = TargetPosition;
			
			if(PotentialCrash)
			{
				if(FVector::Dist(GetActorLocation(), PotentialCrash->GetActorLocation()) <= 90 || Path[0] == PotentialCrash->LastNode)
				{
					UE_LOG(Collisions, Warning, TEXT("Ship %s CRASHED WITH Ship %s!"), *this->GetName(), *PotentialCrash->GetName());
					
					if(LevelGenerator)
					{
						LevelGenerator->CrashPenalty += 50;
					}
					PotentialCrash->PotentialCrash = nullptr;
				}
				PotentialCrash = nullptr;
			}
			
			if(Path[0] == GoalNode)
			{
				AtGoal = true;
				for(int i = 0; i < Meshes.Num(); i++)
				{
					Cast<UStaticMeshComponent>(Meshes[i])->SetMaterial(0, FinishedMaterial);
				}
			}
			else
			{
				Path[0]->ObjectAtLocation = nullptr;
			}
			
			if(LevelGenerator)
			{
				if(FirstMove)
				{
					FirstMove = false;
				}
				else
				{
					LevelGenerator->PathCostTaken.Add(Path[0]->GetTravelCost());
				}
			}

			LastNode = Path[0];
			Path.RemoveAt(0);
		}
		
		SetActorLocation(CurrentPosition);
		SetActorRotation(Direction.Rotation());
	}
}

