#pragma once
#include "EngineUtils.h"


struct FOCGUtils
{
public:
	template <typename T>
	static TArray<T*> GetAllActorsOfClass(const UWorld* World)
	{
		TArray<T*> FoundActors;
		for (T* Actor : TActorRange<T>(World))
		{
			FoundActors.Add(Actor);
		}
		return FoundActors;
	}
};
