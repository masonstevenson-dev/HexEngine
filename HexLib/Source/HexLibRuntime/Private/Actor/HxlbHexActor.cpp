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

#include "Actor/HxlbHexActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Foundation/HxlbHex.h"
#include "FunctionLibraries/HxlbMath.h"
#include "HexLibRuntimeLoggingDefs.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

using HexMath = UHxlbMath;

AHxlbHexActor::AHxlbHexActor(const FObjectInitializer& Initializer): Super(Initializer)
{
	TransformComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TransformComponent"));
	SetRootComponent(TransformComponent);
	
#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UStaticMesh> OverlayMeshFinder(TEXT("/HxLib/Meshes/SM_HexGridlinesTest_pointy_1m.SM_HexGridlinesTest_pointy_1m"));
	GridMesh = OverlayMeshFinder.Object;
#endif
	
	GridMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("GridMeshComponent");
	GridMeshComponent->SetStaticMesh(GridMesh);
	GridMeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	GridMeshComponent->SetCastShadow(false);

	GridMeshComponent->SetupAttachment(RootComponent);

#if WITH_EDITORONLY_DATA
	// Prevents this actor from being moved in the editor viewport.
	SetLockLocation(true);
#endif
}

void AHxlbHexActor::InitializeHexActor(UHxlbHex* NewHex)
{
	if (!NewHex)
	{
		UE_LOG(LogHxlbRuntime, Error, TEXT("AHxlbHexActor::InitializeHexActor(): NewHex is invalid"));
		return;
	}

	Hex = NewHex;
	if (!Hex->GetHexMap().IsValid())
	{
		UE_LOG(LogHxlbRuntime, Error, TEXT("AHxlbHexActor::InitializeHexActor(): HexMap is invalid"));
		return;
	}
	
	SyncLocationAndScale();

	if (GridMeshComponent != nullptr)
	{
		GridMeshComponent->SetRelativeScale3D(FVector(GridMeshScale, GridMeshScale, GridMeshScale));

		UMaterialInterface* BaseMaterial = GridMeshComponent->GetMaterial(0);

		GridMeshComponent->CreateDynamicMaterialInstance(0, BaseMaterial);
	}
}

void AHxlbHexActor::SyncLocationAndScale()
{
	if (!Hex.IsValid())
	{
		UE_LOG(LogHxlbRuntime, Error, TEXT("AHxlbHexActor::SyncLocation(): Hex is invalid"));
		return;
	}
	if (!Hex->GetHexMap().IsValid())
	{
		UE_LOG(LogHxlbRuntime, Error, TEXT("AHxlbHexActor::SyncLocation(): HexMap is invalid"));
		return;
	}
	
	FVector NewLocation = Hex->GetWorldCoords();
	SetActorLocation(NewLocation);

	double MapSize = Hex->GetHexSize();
	double ScaleRatio = MapSize / HexSize;
	SetActorScale3D(FVector(ScaleRatio, ScaleRatio, ScaleRatio));
}

void AHxlbHexActor::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AppendTags(GameplayTags);
}

#if WITH_EDITOR
void AHxlbHexActor::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AHxlbHexActor, GameplayTags))
	{
		if (Hex.IsValid())
		{
			Hex->ProcessGameplayTags();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AHxlbHexActor::SetDebugColor(FLinearColor DebugColor)
{
	if (GridMeshComponent == nullptr || !GridMeshComponent->IsVisible())
	{
		return;
	}
	
	auto* MID0 = Cast<UMaterialInstanceDynamic>(GridMeshComponent->GetMaterial(0));
	if (MID0)
	{
		MID0->SetVectorParameterValue("InnerColor_Preview", DebugColor);
	}
}

void AHxlbHexActor::SetHighlightColor(FLinearColor HighlightColor)
{
	if (GridMeshComponent == nullptr || !GridMeshComponent->IsVisible())
	{
		return;
	}
	
	auto* MID0 = Cast<UMaterialInstanceDynamic>(GridMeshComponent->GetMaterial(0));
	if (MID0)
	{
		MID0->SetVectorParameterValue("InnerColor_Highlight", HighlightColor);
	}
}
#endif 
