// Copyright © Mason Stevenson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "FunctionLibraries/HxlbLandscapeUtil.h"

#include "Engine/World.h"

bool HxlbLandscapeUtil::LandscapeTrace(FHxlbLandscapeTraceContext& TraceContext, FVector& OutHitLocation)
{
	ALandscape* TargetLandscape = TraceContext.Landscape.Get();
	if (!TargetLandscape)
	{
		return false;
	}

	UWorld* World = TargetLandscape->GetWorld();

	FVector TraceEnd = TraceContext.TraceStart + WORLD_MAX * TraceContext.TraceDirection;
	TArray<FHitResult> HitResults;

	// Note using the ECC_Visibility trace channel as the object type searches for the landscape's editor-specific
	// collision shape.
	bool TraceResult = World->LineTraceMultiByObjectType(
		HitResults,
		TraceContext.TraceStart,
		TraceEnd,
		FCollisionObjectQueryParams(ECC_Visibility),
		FCollisionQueryParams(SCENE_QUERY_STAT(HxlbLandscapeTrace), true)
	);
	if (!TraceResult)
	{
		return false;
	}

	bool bFoundLandscape = false;
	for (int HitIndex = 0; HitIndex < HitResults.Num() && !bFoundLandscape; HitIndex++)
	{
		const FHitResult& Hit = HitResults[HitIndex];
		auto* CollisionComponent = Cast<ULandscapeHeightfieldCollisionComponent>(Hit.Component.Get());
		if (CollisionComponent)
		{
			bFoundLandscape = true;
			OutHitLocation = Hit.Location;
		}
	}
	
	return bFoundLandscape;
}
