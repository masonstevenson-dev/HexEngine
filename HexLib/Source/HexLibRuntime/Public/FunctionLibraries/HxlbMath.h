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

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"

#include "HxlbMath.generated.h"

#define HEX_Q(IntVector) IntVector.X
#define HEX_R(IntVector) IntVector.Y
#define HEX_S(IntVector) IntVector.Z

UENUM(BlueprintType)
enum class EHexOrientation: uint8
{
	Undefined UMETA(Hidden),
	Flat,
	Pointy
};

// The directions that each neighbor hex would be labeled if starting from a single flat or pointy hex.
UENUM(BlueprintType)
enum class EHexDirection: uint8
{
	Flat_North,
	Flat_NorthEast,
	Flat_SouthEast,
	Flat_South,
	Flat_SouthWest,
	Flat_NorthWest,
	Pointy_NorthEast,
	Pointy_East,
	Pointy_SouthEast,
	Pointy_SouthWest,
	Pointy_West,
	Pointy_NorthWest
};

UCLASS()
class HEXLIBRUNTIME_API UHxlbMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

// Constants
public:
	static constexpr float kRad30 = UE_PI / 6.0f;
	static constexpr float kRad60 = UE_PI / 3.0f;
	
public:
	UFUNCTION(BlueprintPure, Category="Hex Coordinates", meta = (DisplayName = "AXIAL_Q"))
	static int32 BP_AxialQ(FIntPoint AxialCoordinate);

	UFUNCTION(BlueprintPure, Category="Hex Coordinates", meta = (DisplayName = "AXIAL_R"))
	static int32 BP_AxialR(FIntPoint AxialCoordinate);

	UFUNCTION(BlueprintPure, Category="Hex Coordinates", meta = (DisplayName = "CUBE_Q"))
	static int32 BP_CubeQ(FIntVector CubeCoordinate);

	UFUNCTION(BlueprintPure, Category="Hex Coordinates", meta = (DisplayName = "CUBE_R"))
	static int32 BP_CubeR(FIntVector CubeCoordinate);

	UFUNCTION(BlueprintPure, Category="Hex Coordinates", meta = (DisplayName = "CUBE_S"))
	static int32 BP_CubeS(FIntVector CubeCoordinate);
	
	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FIntPoint CubeToAxial(FIntVector CubeCoordinate);

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FIntVector AxialToCube(FIntPoint AxialCoordinate);

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FIntPoint WorldToAxial(FVector WorldCoordinate, double Size, EHexOrientation Orientation = EHexOrientation::Pointy);

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FVector AxialToWorld(FIntPoint AxialCoordinate, double Size); 

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FVector GetHexCenterPoint(FVector PointOnHex, double Size);

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FVector GetHexCorner(
		FVector Center,
		double Size,
		int32 CornerIndex,
		EHexOrientation Orientation = EHexOrientation::Pointy
	);

	// Distance from center.
	UFUNCTION(BlueprintPure, Category="Hex Math")
	static int32 CubeLength(FIntVector CubeCoord);

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static int32 AxialLength(FIntPoint AxialCoord);

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static int32 CubeDistance(FIntVector CubeCoordA, FIntVector CubeCoordB);

	UFUNCTION(BlueprintPure, Category="Hex Math")
	static int32 AxialDistance(FIntPoint AxialCoordA, FIntPoint AxialCoordB);
	
	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FIntVector DirectionToCube(EHexDirection Direction);
	
	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FIntVector CubeNeighbor(FIntVector CubeCoord, EHexDirection Direction);
	
	UFUNCTION(BlueprintPure, Category="Hex Math")
	static FIntPoint AxialNeighbor(FIntPoint AxialCoord, EHexDirection Direction);

// Helpers and non-BP exposed functions -------------------------------------------------------------------------------
public:
	static FVector VectorFloor(FVector Vector);
	static FVector VectorCeil(FVector Vector);
	static FIntVector CubeRound(FVector FractionalCubeCoordinate);
	static FIntPoint AxialRound(FVector2d FractionalAxialCoordinate);
	
	static FIntPoint CartesianToAxial(
		FVector CartesianCoordinate,
		double Size,
		EHexOrientation Orientation = EHexOrientation::Pointy
	);

	static FVector AxialToCartesian(FIntPoint AxialCoordinate, double Size);
	
	// Reflection
	static FIntVector ReflectCube_Q(FIntVector CubeCoord);
	static FIntVector ReflectCube_R(FIntVector CubeCoord);
	static FIntVector ReflectCube_S(FIntVector CubeCoord);
	static FIntPoint ReflectAxial_Q(FIntPoint AxialCoord);
	static FIntPoint ReflectAxial_R(FIntPoint AxialCoord);
	static FIntPoint ReflectAxial_S(FIntPoint AxialCoord);

	static float CenterAngle(FVector HexCenter, FVector WorldCoord);
	static uint8 NeighborEdgeIndex(FIntPoint HexCoord, FIntPoint NeighborCoord, double Size, EHexOrientation Orientation = EHexOrientation::Pointy);
	static bool AxialToTexture(
		FIntPoint AxialCoord,
		int32 TextureSizeX,
		int32 TextureSizeY,
		FIntPoint& OutTextureCoord,
		uint32 BoundaryOffset = 2
	);
	static int32 TextureToPixelBuffer(FIntPoint TextureCoords, int32 BufferSizeX);
	static bool AxialToPixelBuffer(FIntPoint AxialCoord, int32 TextureSizeX, int32 TextureSizeY, int32& BufferIndex, uint32 BoundaryOffset = 2); // TODO(): unit test this

	// for when you want to walk through each direction in order
	static FIntVector DirectionIndexToCube(int32 Index);
	
protected:
	static FVector GetHexCornerHelper(FVector Center, double Size, int32 CornerIndex, double OffsetDeg);

// End helpers and non-BP exposed functions ----------------------------------------------------------------------------
};
