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

#include "FunctionLibraries/HxlbMath.h"

#include "HexLibRuntimeLoggingDefs.h"

FVector UHxlbMath::VectorFloor(FVector Vector)
{
	return FVector(
		FMath::Floor(Vector.X),
		FMath::Floor(Vector.Y),
		FMath::Floor(Vector.Z)
	);
}

FVector UHxlbMath::VectorCeil(FVector Vector)
{
	return FVector(
		FMath::CeilToDouble(Vector.X),
		FMath::CeilToDouble(Vector.Y),
		FMath::CeilToDouble(Vector.Z)
	);
}

int32 UHxlbMath::BP_AxialQ(FIntPoint AxialCoordinate)
{
	return HEX_Q(AxialCoordinate);
}

int32 UHxlbMath::BP_AxialR(FIntPoint AxialCoordinate)
{
	return HEX_R(AxialCoordinate);
}

int32 UHxlbMath::BP_CubeQ(FIntVector CubeCoordinate)
{
	return HEX_Q(CubeCoordinate);
}

int32 UHxlbMath::BP_CubeR(FIntVector CubeCoordinate)
{
	return HEX_R(CubeCoordinate);	
}

int32 UHxlbMath::BP_CubeS(FIntVector CubeCoordinate)
{
	return HEX_S(CubeCoordinate);	
}

FIntPoint UHxlbMath::CubeToAxial(FIntVector CubeCoordinate)
{
	return FIntPoint(CubeCoordinate.X, CubeCoordinate.Y);
}

FIntVector UHxlbMath::AxialToCube(FIntPoint AxialCoordinate)
{
	return FIntVector(AxialCoordinate.X, AxialCoordinate.Y, -1 * (AxialCoordinate.X + AxialCoordinate.Y));
}

FIntVector UHxlbMath::CubeRound(FVector FractionalCubeCoordinate)
{
	int32 Q = FMath::RoundToInt(FractionalCubeCoordinate.X);
	int32 R = FMath::RoundToInt(FractionalCubeCoordinate.Y);
	int32 S = FMath::RoundToInt(FractionalCubeCoordinate.Z);

	double QDiff = FMath::Abs(Q - FractionalCubeCoordinate.X);
	double RDiff = FMath::Abs(R - FractionalCubeCoordinate.Y);
	double SDiff = FMath::Abs(S - FractionalCubeCoordinate.Z);

	if (QDiff > RDiff && QDiff > SDiff)
	{
		Q = -1 * (R + S);
	}
	else if (RDiff >  SDiff)
	{
		R = -1 * (Q + S);
	}
	else
	{
		S = -1 * (Q + R);
	}

	return FIntVector(Q, R, S);
}

FIntPoint UHxlbMath::AxialRound(FVector2d FractionalAxialCoordinate)
{
	FVector FractionalCube = FVector(
		FractionalAxialCoordinate.X,
		FractionalAxialCoordinate.Y,
		-1 * (FractionalAxialCoordinate.X + FractionalAxialCoordinate.Y)
	);
	
	return CubeToAxial(CubeRound(FractionalCube));
}

FIntPoint UHxlbMath::CartesianToAxial(FVector CartesianCoordinate, double Size, EHexOrientation Orientation)
{
	if (Orientation == EHexOrientation::Undefined || Orientation == EHexOrientation::Flat)
	{
		UE_LOG(LogHxlbRuntime, Error, TEXT("Unsupported Hex Orientation: %s"), *UEnum::GetValueAsString(Orientation));
	}
	
	const double Q = ((FMath::Sqrt(3.0) * CartesianCoordinate.X) - CartesianCoordinate.Y) / (Size * 3.0);
	const double R = (2 * CartesianCoordinate.Y) / (Size * 3);
	
	FIntPoint Result = AxialRound(FVector2d(Q, R));

	return Result;
}

FIntPoint UHxlbMath::WorldToAxial(FVector WorldCoordinate, double Size, EHexOrientation Orientation)
{
	FVector CartesianCoord = FVector(WorldCoordinate.Y, WorldCoordinate.X, WorldCoordinate.Z);
	return CartesianToAxial(CartesianCoord, Size, Orientation);
}

FVector UHxlbMath::AxialToCartesian(FIntPoint AxialCoordinate, double Size)
{
	const double X = Size * (FMath::Sqrt(3.0) * AxialCoordinate.X + FMath::Sqrt(3.0) / 2.0 * AxialCoordinate.Y);
	const double Y = Size * (3.0 / 2.0 * AxialCoordinate.Y);
	return FVector(X, Y, 0.0);
}

FVector UHxlbMath::AxialToWorld(FIntPoint AxialCoordinate, double Size)
{
	FVector CartesianCoord = AxialToCartesian(AxialCoordinate, Size);
	return FVector(CartesianCoord.Y, CartesianCoord.X, CartesianCoord.Z);
}

FVector UHxlbMath::GetHexCenterPoint(FVector PointOnHex, double Size)
{
	return AxialToWorld(WorldToAxial(PointOnHex, Size), Size);
}

FVector UHxlbMath::GetHexCornerHelper(FVector Center, double Size, int32 CornerIndex, double OffsetDeg)
{
	double AngleDeg = (60 * CornerIndex) - OffsetDeg;
	double AngleRad = FMath::DegreesToRadians(AngleDeg);
	return FVector(Center.X + Size * FMath::Cos(AngleRad), Center.Y + Size * FMath::Sin(AngleRad), 0.0);
}

FVector UHxlbMath::GetHexCorner(FVector Center, double Size, int32 CornerIndex, EHexOrientation Orientation)
{
	// pointy algo
	// In Unreal, the forward direction is +X, so we end up flipping the algos for flat and pointy.
	if (Orientation == EHexOrientation::Pointy)
	{
		return GetHexCornerHelper(Center, Size, CornerIndex, 0.0);
	}

	// flat algo
	return GetHexCornerHelper(Center, Size, CornerIndex, 30.0);
}

int32 UHxlbMath::CubeLength(FIntVector CubeCoord)
{
	return (FMath::Abs(CubeCoord.X) + FMath::Abs(CubeCoord.Y) + FMath::Abs(CubeCoord.Z)) / 2;
}

int32 UHxlbMath::AxialLength(FIntPoint AxialCoord)
{
	return CubeLength(AxialToCube(AxialCoord));
}

int32 UHxlbMath::CubeDistance(FIntVector CubeCoordA, FIntVector CubeCoordB)
{
	return CubeLength(CubeCoordA - CubeCoordB);
}

int32 UHxlbMath::AxialDistance(FIntPoint AxialCoordA, FIntPoint AxialCoordB)
{
	return CubeDistance(AxialToCube(AxialCoordA), AxialToCube(AxialCoordB));
}

FIntVector UHxlbMath::ReflectCube_Q(FIntVector CubeCoord)
{
	return FIntVector(HEX_Q(CubeCoord), HEX_S(CubeCoord), HEX_R(CubeCoord));
}

FIntVector UHxlbMath::ReflectCube_R(FIntVector CubeCoord)
{
	return FIntVector(HEX_S(CubeCoord), HEX_R(CubeCoord), HEX_Q(CubeCoord));
}

FIntVector UHxlbMath::ReflectCube_S(FIntVector CubeCoord)
{
	return FIntVector(HEX_R(CubeCoord), HEX_Q(CubeCoord), HEX_S(CubeCoord));
}

FIntPoint UHxlbMath::ReflectAxial_Q(FIntPoint AxialCoord)
{
	return CubeToAxial(ReflectCube_Q(AxialToCube(AxialCoord)));
}

FIntPoint UHxlbMath::ReflectAxial_R(FIntPoint AxialCoord)
{
	return CubeToAxial(ReflectCube_R(AxialToCube(AxialCoord)));
}

FIntPoint UHxlbMath::ReflectAxial_S(const FIntPoint AxialCoord)
{
	return CubeToAxial(ReflectCube_S(AxialToCube(AxialCoord)));
}

float UHxlbMath::CenterAngle(FVector HexCenter, FVector WorldCoord)
{
	float XPrime = WorldCoord.X - HexCenter.X;
	float YPrime = WorldCoord.Y - HexCenter.Y;
	
	return FMath::Atan2(XPrime, YPrime);
}

uint8 UHxlbMath::NeighborEdgeIndex(FIntPoint HexCoord, FIntPoint NeighborCoord, double Size, EHexOrientation Orientation)
{
	if (Orientation == EHexOrientation::Undefined || Orientation == EHexOrientation::Flat)
	{
		UE_LOG(LogHxlbRuntime, Error, TEXT("Unsupported Hex Orientation: %s"), *UEnum::GetValueAsString(Orientation));
	}

	FVector HexWorldCoord = AxialToWorld(HexCoord, Size);
	FVector NeighborWorldCoord = AxialToWorld(NeighborCoord, Size);

	float NeighborAngle = CenterAngle(NeighborWorldCoord, HexWorldCoord);
	NeighborAngle = NeighborAngle + UE_PI + kRad30;

	return FMath::TruncToInt32(NeighborAngle / kRad60) % 6;
}

bool UHxlbMath::AxialToTexture(FIntPoint AxialCoord, int32 TextureSizeX, int32 TextureSizeY, FIntPoint& OutTextureCoord, uint32 BoundaryOffset)
{
	int32 PixelX = HEX_Q(AxialCoord) + (TextureSizeX / 2);
	int32 PixelY = HEX_R(AxialCoord) + (TextureSizeY / 2);
	OutTextureCoord = FIntPoint(PixelX, PixelY);

	// Note: We maintain 1px of padding around the texture and use a clamped texture sample so that any out of bounds
	//       texture samples come back as a 0.0 value. An additional 1px of padding is added for gridline adjacency.
	int32 MinX = 0 + BoundaryOffset;
	int32 MaxX = (TextureSizeX - 1) - BoundaryOffset; // TextureMaxX - BoundaryOffset
	int32 MinY = 0 + BoundaryOffset;
	int32 MaxY = (TextureSizeY - 1) - BoundaryOffset; // TextureMaxY - BoundaryOffset
	
	return (TextureSizeX * TextureSizeY) > 0 && PixelX >= MinX && PixelX <= MaxX && PixelY >= MinY && PixelY <= MaxY;
}

int32 UHxlbMath::TextureToPixelBuffer(FIntPoint TextureCoords, int32 BufferSizeX)
{
	return TextureCoords.X + (BufferSizeX * TextureCoords.Y);
}

bool UHxlbMath::AxialToPixelBuffer(FIntPoint AxialCoord, int32 TextureSizeX, int32 TextureSizeY, int32& BufferIndex, uint32 BoundaryOffset)
{
	FIntPoint TextureCoord;
	if (!AxialToTexture(AxialCoord, TextureSizeX, TextureSizeY, TextureCoord, BoundaryOffset))
	{
		// UE_LOG(LogHxlbRuntime, Error, TEXT("Invalid texture coord: (%d, %d) from axial coord: (%d, %d). Texture size is: (%d, %d)"),
		//	TextureCoord.X, TextureCoord.Y, AxialCoord.X, AxialCoord.Y, TextureSizeX, TextureSizeY);
		return false;
	}

	BufferIndex = TextureToPixelBuffer(TextureCoord, TextureSizeX);
	int32 BufferSize = TextureSizeX * TextureSizeY;
	return BufferSize > 0 && BufferIndex >= 0 && BufferIndex < BufferSize;
}
