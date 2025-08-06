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
#include "GameplayTagContainer.h"
#include "UObject/Object.h"

#include "HxlbHex.generated.h"

class UHxlbHexMapComponent;
class AHxlbHexActor;

// Fundamentally, a "Hex" is just a set of 2D axial coordinates. However, the user or the system may have decided to
// associate additional information with these coordinates (such as an actor or some other game-specific data). If such
// information exists, a UHxlbHex will be instantiated to track it.
//
// Note on performance:
//   The plugin currently expects that users will create on average no more than 10,000 hexes per map. The current hard
//   limit is ~65,536 hexes since we have chosen to use render target sizes of 256px x 256px. From manual testing,
//   creating individual UObjects on the order of 1000s seems to be reasonable. If in the future, we ever need to
//   support creating 100k+ hexes, we may need to revamp this entire system to store data in contiguous blocks and then
//   create proxy objects for user editing.
UCLASS(Blueprintable, DisplayName="DefaultHex")
class UHxlbHex : public UObject
{
	GENERATED_BODY()

public:
	void InitialzeHex(UHxlbHexMapComponent* NewHexMap, FIntPoint& NewCoords);

	FIntPoint GetHexCoords() { return AxialCoords; }
	FVector GetWorldCoords() { return WorldCoords; }
	double GetHexSize() { return HexSize; }
	
	void SetHexCoords(FIntPoint& NewCoords);
	void SetHexSize(double NewSize);
	
	TWeakObjectPtr<UHxlbHexMapComponent> GetHexMap() { return HexMap; }
	
	AHxlbHexActor* GetHexActor() { return HexActor; }
	void SetHexActor(AHxlbHexActor* NewActor);
	void ClearHexActor();

	// Searches HexActor for certain owned gameplay tags and then uses them for various updates.
	void ProcessGameplayTags();

// Hex data available in hex details editor
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "HexGame.Map"), Category="Hex Data")
	FGameplayTagContainer GameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hex Data")
	int32 TestVal = 0;
	
protected:
	FIntPoint AxialCoords = FIntPoint::ZeroValue;

	// Cached for convenience. 
	FVector WorldCoords = FVector::ZeroVector;

	// Cached for convenience.
	double HexSize = 100.0;
	
	UPROPERTY()
	TWeakObjectPtr<UHxlbHexMapComponent> HexMap;
	
	UPROPERTY()
	TObjectPtr<AHxlbHexActor> HexActor;
};
