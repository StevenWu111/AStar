// Fill out your copyright notice in the Description page of Project Settings.


#include "GridNode.h"


GridNode::GridNode()
{
	X = 0;
	Y = 0;
	G = 0;
	H = 0;
	F = 0;

	GridType = DeepWater;
	Parent = nullptr;
	ObjectAtLocation = nullptr;
}

float GridNode::GetTravelCost() const
{
	switch(GridType)
	{
	case Land:
		return 100;
	case DeepWater:
		return 3;
	default:
		return 1;
	}
}
