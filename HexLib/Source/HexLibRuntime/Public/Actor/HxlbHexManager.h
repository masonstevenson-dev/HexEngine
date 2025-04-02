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
#include "Engine/PostProcessVolume.h"
#include "GameFramework/Info.h"
#include "HxlbHexActor.h"
#include "Foundation/HxlbHexMap.h"
#include "FunctionLibraries/HxlbMath.h"

#include "HxlbHexManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapEditorUpdate);

UCLASS(Blueprintable, meta=(DisplayName="Hex Manager"), showcategories=(Rendering))
class HEXLIBRUNTIME_API AHxlbHexManager : public AInfo
{
	GENERATED_BODY()

public:
	AHxlbHexManager(const FObjectInitializer& Initializer);
	
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostActorCreated() override;
	virtual void BeginPlay() override;

	virtual void AddHex(FIntPoint& AxialCoords, bool bCreateProxy=true);

	virtual APostProcessVolume* GetOverlayPPV() { return OverlayPPV.Get(); }
	virtual void SetOverlayPPV(APostProcessVolume* NewPPV) { OverlayPPV = NewPPV; }

	// Note, don't call this without validating MapComponent via function such as FindHexManager(), which guarantees
	// that both the HexManager and its MapComponent exist and are valid.
	virtual FHxlbMapSettings& MapSettings() { return MapComponent->MapSettings; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HexEngine")
	TObjectPtr<UHxlbHexMapComponent> MapComponent;
	
	FOnMapEditorUpdate OnMapEditorUpdate;

	/** Arrow component to indicate forward direction of start */
#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<class UArrowComponent> ArrowComponent;
public:
#endif
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="HexEngine")
	TObjectPtr<USceneComponent> TransformComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HexEngine")
	TSubclassOf<AHxlbHexActor> HexActorClass;

	UPROPERTY()
	TWeakObjectPtr<APostProcessVolume> OverlayPPV;
};