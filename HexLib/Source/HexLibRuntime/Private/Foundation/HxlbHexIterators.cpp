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

#include "Foundation/HxlbHexIterators.h"

#include "FunctionLibraries/HxlbMath.h"

using HexMath = UHxlbMath;

FHxlbRadialIterator::FHxlbRadialIterator(FIntPoint NewOrigin, int32 NewRadius)
{
	if (NewRadius < 0)
	{
		return;
	}
	
	Origin = NewOrigin;
	Radius = NewRadius;

	if (Radius != 0)
	{
		Current = FIntPoint(-Radius, 0);
	}
	bFirstIteration = true;

	// finish initialization
	bIsInitialized = true;
}

bool FHxlbRadialIterator::Next()
{
	if (!bIsInitialized)
	{
		return false;
	}
	if (bFirstIteration)
	{
		bFirstIteration = false;
		return true;
	}
	if (Radius == 0)
	{
		return false;
	}

	HEX_R(Current)++;
	int32 RMax = FMath::Min(Radius, -HEX_Q(Current) + Radius);

	if (HEX_R(Current) > RMax)
	{
		HEX_Q(Current)++;
		HEX_R(Current) = FMath::Max(-Radius, -HEX_Q(Current) - Radius);
	}

	int32 RMin = FMath::Max(-Radius, -HEX_Q(Current) - Radius);
	RMax = FMath::Min(Radius, -HEX_Q(Current) + Radius);
	
	if (HEX_Q(Current) < -Radius || HEX_Q(Current) > Radius || HEX_R(Current) < RMin || HEX_R(Current) > RMax)
	{
		return false;
	}
	return true;
}

FIntPoint FHxlbRadialIterator::Get()
{
	return Current + Origin;
}

FHxlbRingIterator::FHxlbRingIterator(FIntPoint NewOrigin, int32 NewRadius)
{
	if (NewRadius < 0)
	{
		return;
	}
	
	Origin = NewOrigin;
	Radius = NewRadius;
	CurrentDirection = 0;
	SideCount = 0;
	
	bFirstIteration = true;

	if (Radius == 0)
	{
		// Special case. If radius is zero we just return the origin. We set CurrentDirection to 6 so that subsequent
		// calls to Next() don't update anything.
		CubeCurrent = HexMath::AxialToCube(Origin);
		CurrentDirection = 6;
	}
	else
	{
		// note the 4 is arbitrary. We could start with any direction
		CubeCurrent = HexMath::AxialToCube(Origin) + (HexMath::DirectionIndexToCube(4) * Radius);
	}
	
	// finish initialization
	bIsInitialized = true;
}

bool FHxlbRingIterator::Next()
{
	if (!bIsInitialized)
	{
		return false;
	}
	if (bFirstIteration)
	{
		bFirstIteration = false;
		return true;
	}
	if (CurrentDirection >= 6)
	{
		return false;
	}
	
	// Get Neighbor
	CubeCurrent = CubeCurrent + HexMath::DirectionIndexToCube(CurrentDirection);
	SideCount++;
	
	if (SideCount >= Radius)
	{
		CurrentDirection += 1;
		SideCount = 0;
	}
	
	return true;
}

FIntPoint FHxlbRingIterator::Get()
{
	return HexMath::CubeToAxial(CubeCurrent);
}

FHxlbRectangularIterator::FHxlbRectangularIterator(FIntPoint NewOrigin, int32 NewHalfWidth, int32 NewHalfHeight)
{
	if (NewHalfWidth < 1 || NewHalfHeight < 1)
	{
		return;
	}
	
	Origin = NewOrigin;
	HalfWidth = NewHalfWidth;
	HalfHeight = NewHalfHeight;
	
	bFirstIteration = true;

	Left = -HalfWidth;
	Right = HalfWidth;

	// Start Hex
	int32 StartRow = -HalfHeight;
	int32 StartCol = Left - FMath::Floor(StartRow / 2.0);
	Current = StartHex = FIntPoint(StartCol, StartRow);

	//End Hex
	int32 EndRow = HalfHeight;
	int32 EndCol = Right - FMath::Floor(EndRow / 2.0);
	EndHex = FIntPoint(EndCol, EndRow);

	// finish initialization
	bIsInitialized = true;
}

FHxlbRectangularIterator::FHxlbRectangularIterator(FIntPoint NewStartHex, FIntPoint NewEndHex)
{
	Origin = NewStartHex;

	Current = StartHex = FIntPoint::ZeroValue;
	EndHex = NewEndHex - NewStartHex;

	Left = 0;
	if (HEX_Q(EndHex) < -1 * FMath::Floor(HEX_R(EndHex) / 2.0))
	{
		EndHex = HexMath::ReflectAxial_R(EndHex);
		bReflectWidth = true;
	}
	if (HEX_R(EndHex) < 0)
	{
		EndHex = HexMath::ReflectAxial_R(EndHex) * -1;
		bReflectHeight = true;
	}
	
	Right = HEX_Q(EndHex) + FMath::Floor(HEX_R(EndHex) / 2.0);

	bFirstIteration = true;

	// finish initialization
	bIsInitialized = true;
}


bool FHxlbRectangularIterator::Next()
{
	if (!bIsInitialized)
	{
		return false;
	}
	if (bFirstIteration)
	{
		bFirstIteration = false;
		return true;
	}

	// pointy algo
	HEX_Q(Current)++;
	int32 MaxCol = Right - FMath::Floor(HEX_R(Current) / 2.0);
	
	if (HEX_Q(Current) > MaxCol)
	{
		HEX_R(Current)++;
		HEX_Q(Current) = Left - FMath::Floor(HEX_R(Current) / 2.0);
	}

	int32 MinCol = Left - FMath::Floor(HEX_R(Current) / 2.0);
	MaxCol = Right - FMath::Floor(HEX_R(Current) / 2.0);
	
	if (HEX_R(Current) < HEX_R(StartHex) || HEX_R(Current) > HEX_R(EndHex) || HEX_Q(Current) < MinCol || HEX_Q(Current) > MaxCol)
	{
		return false;
	}
	return true;
}

FIntPoint FHxlbRectangularIterator::Get()
{
	FIntPoint ShiftedCoords = Current;
	if (bReflectWidth)
	{
		ShiftedCoords = HexMath::ReflectAxial_R(ShiftedCoords);
	}
	if (bReflectHeight)
	{
		ShiftedCoords = HexMath::ReflectAxial_R(ShiftedCoords) * -1;
	}
	ShiftedCoords += Origin;
	return ShiftedCoords;
}