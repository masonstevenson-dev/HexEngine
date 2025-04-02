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

#include "Testing/Manual/HxlbCornerTestTargetActor.h"

#include "Engine/EngineTypes.h"
#include "Engine/Scene.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HexLibRuntimeLoggingDefs.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

AHxlbCornerTestTargetActor::AHxlbCornerTestTargetActor(const FObjectInitializer& Initializer): Super(Initializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorScale3D(FVector(0.25, 0.25, 0.25));
	
#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube"));
	UStaticMeshComponent* InternalStaticMeshComponent = GetStaticMeshComponent();
	InternalStaticMeshComponent->SetStaticMesh(CubeMeshFinder.Object);
	
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TestMaterial_Finder(TEXT("/Game/HxlbTests/Materials/M_AngleTest.M_AngleTest"));
	InternalStaticMeshComponent->SetMaterial(0, TestMaterial_Finder.Object);
#endif
}

void AHxlbCornerTestTargetActor::PostActorCreated()
{
	Super::PostActorCreated();
	
	// try to find the PPV
	for (TActorIterator<APostProcessVolume> Iterator(GetWorld()); Iterator; ++Iterator)
	{
		TargetPPV = *Iterator;
		break;
	}
}

void AHxlbCornerTestTargetActor::Tick(float DeltaSeconds)
{
	// Super::Tick(DeltaSeconds);
	
	if (GetWorld()->WorldType != EWorldType::Editor)
	{
		return; // Only tick in editor.
	}

	if (FPlatformTime::Seconds() - LastRefreshTimeSeconds >= RefreshRateSeconds)
	{
		RefreshPPVMaterialParams();
		LastRefreshTimeSeconds = FPlatformTime::Seconds();
	}
}

void AHxlbCornerTestTargetActor::RefreshPPVMaterialParams()
{
	APostProcessVolume* OverlayVolume = TargetPPV.Get();
	if (!OverlayVolume)
	{
		return;
	}

	for (FWeightedBlendable& WeightedBlendable : OverlayVolume->Settings.WeightedBlendables.Array)
	{
		if (auto* PPVMaterial = Cast<UMaterial>(WeightedBlendable.Object))
		{
			WeightedBlendable.Object = UMaterialInstanceDynamic::Create(PPVMaterial, OverlayVolume);
		}
		if (auto* MID0 = Cast<UMaterialInstanceDynamic>(WeightedBlendable.Object))
		{
			MID0->SetScalarParameterValue("HexSize", HexSize);
			MID0->SetVectorParameterValue("WorldCoord", GetActorLocation());
			break;
		}
	}
}
