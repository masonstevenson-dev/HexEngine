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

#include "FunctionLibraries/HxlbUtilityFunctions.h"

#include "HexLibRuntimeLoggingDefs.h"
#include "Foundation/HxlbHexIterators.h"
#include "FunctionLibraries/HxlbMath.h"
#include "Macros/HexLibLoggingMacros.h"

using HexMath = UHxlbMath;

TArray<FIntPoint> UHxlbUtilityFunctions::GetHexesInRange(FIntPoint CenterHex, int32 Range)
{
	TArray<FIntPoint> Hexes;

	auto Iterator = FHxlbRadialIterator(CenterHex, Range);
	while (Iterator.Next())
	{
		Hexes.Add(Iterator.Get());
	}

	return Hexes;
}

TArray<FIntPoint> UHxlbUtilityFunctions::GetHexRectangleFromCenter(FIntPoint CenterHex, int32 HalfWidth, int32 HalfHeight)
{
	TArray<FIntPoint> Hexes;

	auto Iterator = FHxlbRectangularIterator(CenterHex, HalfWidth, HalfHeight);
	while (Iterator.Next())
	{
		Hexes.Add(Iterator.Get());
	}
	
	return Hexes;
}

TArray<FIntPoint> UHxlbUtilityFunctions::GetHexRectangleFromCorners(FIntPoint StartCorner, FIntPoint EndCorner)
{
	TArray<FIntPoint> Hexes;

	auto Iterator = FHxlbRectangularIterator(StartCorner, EndCorner);
	while (Iterator.Next())
	{
		Hexes.Add(Iterator.Get());
	}
	
	return Hexes;
}

TArray<FIntPoint> UHxlbUtilityFunctions::SimpleRadiusIntersection(FVector Origin, double Radius, double HexSize)
{
	TArray<FIntPoint> Hexes;
	
	FIntPoint OriginHex = HexMath::WorldToAxial(Origin, HexSize);
	Hexes.Add(OriginHex);
	
	bool ContinueSearch = true;
	int32 SearchRadius = 1;

	while (ContinueSearch)
	{
		int32 Hits = 0;
		
		auto Iterator = FHxlbRingIterator(OriginHex, SearchRadius);
		while (Iterator.Next())
		{
			FIntPoint HexCoord = Iterator.Get();
			
			// for each hex, first check if its center coord overlaps with the circle. If it does, we are done. Otherwise,
			// check each corner for overlap.
			double Dist = FVector::DistXY(Origin, HexMath::AxialToWorld(HexCoord, HexSize));
			if (Dist <= Radius)
			{
				// HXLB_LOG(LogHxlbRuntime, Warning, TEXT("Radius: %.2lf Dist: %.2lf"), Radius, Dist);
				
				Hits += 1;
				Hexes.Add(HexCoord);
				continue;
			}
			
			for (int CornerIndex = 0; CornerIndex < 6; CornerIndex++)
			{
				FVector CurrentCorner = HexMath::GetHexCorner(HexMath::AxialToWorld(HexCoord, HexSize), HexSize, CornerIndex);
				
				if (FVector::DistXY(Origin, CurrentCorner) <= Radius)
				{
					Hits += 1;
					Hexes.Add(HexCoord);
					break;
				}
			}
		}
	
		// When we find an entire ring of hexes that do not overlap, we are done.
		if (Hits == 0)
		{
			ContinueSearch = false;
		}
		else
		{
			SearchRadius++;
		}
	}
	
	return Hexes;
}
