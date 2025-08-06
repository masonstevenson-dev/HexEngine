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

#include "Actor/HxlbHexManager.h"

#include "Engine/World.h"
#include "HexLibRuntimeLoggingDefs.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Foundation/HxlbHexMap.h"
#include "Macros/HexLibLoggingMacros.h"

AHxlbHexManager::AHxlbHexManager(const FObjectInitializer& Initializer): Super(Initializer)
{
	TransformComponent = CreateDefaultSubobject<USceneComponent>(TEXT("TransformComponent"));
	SetRootComponent(TransformComponent);
	
	MapComponent = CreateDefaultSubobject<UHxlbHexMapComponent>("Hex Map Component");
	HexActorClass = AHxlbHexActor::StaticClass();
	
#if WITH_EDITORONLY_DATA
	// No idea what the point of creating this just to do nothing with it if we are running a commandlet is. But this
	// is how every other UE codepath does it.
	ArrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	UBillboardComponent* InternalSpriteComponent = GetSpriteComponent();
	
	if (!IsRunningCommandlet() && (GetSpriteComponent() != NULL))
	{
		if (ArrowComponent)
		{
			ArrowComponent->ArrowColor = FColor(150, 200, 255);

			ArrowComponent->ArrowSize = 1.0f;
			ArrowComponent->bTreatAsASprite = true;
			ArrowComponent->SetupAttachment(RootComponent);
			ArrowComponent->bIsScreenSizeScaled = true;
		}

		if (InternalSpriteComponent)
		{
			// Supposedly, we should be able to just use ClassThumbnail.HxlbHexManager in the editor style defs file to set
			// the sprite for this actor, but I can't seem to figure out how to get it working.
			InternalSpriteComponent->SetSprite(LoadObject<UTexture2D>(nullptr, TEXT("/HxLib/Textures/hex_manager_v4_128.hex_manager_v4_128")));
			InternalSpriteComponent->SetupAttachment(RootComponent);
		}
	}
#endif
}

void AHxlbHexManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (MapComponent)
	{
		// AHxlbHexActor* DefaultHexActor = HexActorClass.GetDefaultObject();
		// MapComponent->MapSettings.HexSize = DefaultHexActor->Size;
		MapComponent->InitGridData(this->GetActorLocation());

#if WITH_EDITOR
		OnMapEditorUpdate.Broadcast();
#endif
		
	}
	else
	{
		HXLB_LOG(LogHxlbRuntime, Warning, TEXT("Expected valid map component."));
	}
}

void AHxlbHexManager::PostActorCreated()
{
	Super::PostActorCreated();
}


void AHxlbHexManager::BeginPlay()
{
	Super::BeginPlay();
}

void AHxlbHexManager::AddHex(FIntPoint& AxialCoords, bool bCreateProxy)
{
	if (!MapComponent)
	{
		return;
	}
	
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Name = MakeUniqueObjectName(this, HexActorClass, FName("HexTile"));
	SpawnInfo.Owner = this;

	UHxlbHex* Hex = MapComponent->GetOrCreateHex(AxialCoords);
	if (Hex && bCreateProxy && Hex->GetHexActor() == nullptr)
	{
		auto* NewHexActor = GetWorld()->SpawnActor<AHxlbHexActor>(HexActorClass, SpawnInfo);
		Hex->SetHexActor(NewHexActor);

#if WITH_EDITOR
		// NOTE: Without this, the UE editor does not seem to realize that the level has been updated.
		Hex->GetHexActor()->SetActorLabel(SpawnInfo.Name.ToString());
#endif
		
		Hex->GetHexActor()->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
}