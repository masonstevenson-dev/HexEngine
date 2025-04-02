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
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Actor.h"

#include "HxlbHexActor.generated.h"

class UHxlbHex;
class AHxlbHexManager;

UCLASS()
class AHxlbHexActor : public AActor, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AHxlbHexActor(const FObjectInitializer& Initializer);

	void InitializeHexActor(UHxlbHex* NewHex);
	void SyncLocationAndScale();

	//~GameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	//~End GameplayTagAssetInterface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void SetDebugColor(FLinearColor DebugColor);
	virtual void SetHighlightColor(FLinearColor HighlightColor);
#endif
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Developer")
	double GridMeshScale = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "HexGame.Map"), Category="Hex Actor")
	FGameplayTagContainer GameplayTags;
	
	UPROPERTY(VisibleAnywhere, Category="Hex Actor")
	TObjectPtr<UStaticMeshComponent> GridMeshComponent;
	
protected:
	UPROPERTY()
	TObjectPtr<USceneComponent> TransformComponent;
	
	// by default, we assume that the proxy mesh is 1m.
	double HexSize = 100.0;

	// TODO(): should this be a regular pointer for serialization purposes?
	UPROPERTY()
	TWeakObjectPtr<UHxlbHex> Hex;

	UPROPERTY()
	TObjectPtr<UStaticMesh> GridMesh;
};
