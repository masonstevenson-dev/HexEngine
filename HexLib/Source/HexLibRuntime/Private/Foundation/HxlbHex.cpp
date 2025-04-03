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

#include "Foundation/HxlbHex.h"

#include "Actor/HxlbHexActor.h"
#include "HexLibRuntimeLoggingDefs.h"
#include "Foundation/HxlbHexMap.h"
#include "FunctionLibraries/HxlbMath.h"
#include "Landscape.h"
#include "Macros/HexLibLoggingMacros.h"

using HexMath = UHxlbMath;

void UHxlbHex::InitialzeHex(UHxlbHexMapComponent* NewHexMap, FIntPoint& NewCoords)
{
	HexMap = NewHexMap;
	AxialCoords = NewCoords;

	if (HexMap.IsValid())
	{
		SetHexSize(HexMap->GetHexSize());
	}
	else
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("UHxlbHex::InitialzeHex(): HexMap is invalid. Initializing hex to the default size."));
		SetHexSize(100.0);
	}
}

void UHxlbHex::SetHexCoords(FIntPoint& NewCoords)
{
	AxialCoords = NewCoords;
	WorldCoords = HexMath::AxialToWorld(AxialCoords, HexSize);

	if (HexActor != nullptr)
	{
		HexActor->SyncLocationAndScale();
	}
}

void UHxlbHex::SetHexSize(double NewSize)
{
	HexSize = NewSize;
	SetHexCoords(AxialCoords); // forces refresh of world cords and updates hex actor.
}

void UHxlbHex::SetHexActor(AHxlbHexActor* NewActor)
{
	if (!NewActor)
	{
		return;
	}
	if (HexActor)
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("UHxlbHex::SetHexActor(): tried to set HexActor, but an existing actor has already been set."));
		return;
	}

	HexActor = NewActor;
	HexActor->InitializeHexActor(this);
}

void UHxlbHex::ProcessGameplayTags()
{
	if (!HexMap.IsValid())
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("UHxlbHex::ProcessGameplayTags(): HexMap is invalid."));
		return;
	}
	if (!HexActor)
	{
		return;
	}
	
	FGameplayTagContainer HexTags;
	HexActor->GetOwnedGameplayTags(HexTags);

#if WITH_EDITOR
	FGameplayTagContainer DebugTags = HexTags.Filter(FGameplayTagContainer(HexMap->MapSettings.DebugTag));
	if (!DebugTags.IsEmpty())
	{
		FHxlbHexTagInfo_Editor SelectedTagInfo;
		int32 HighestPri = -1;
				
		for (FGameplayTag FilteredTag : DebugTags)
		{
			FHxlbHexTagInfo_Editor* TagInfo = HexMap->MapSettings.TagSettings.Find(FilteredTag);
				
			if (!TagInfo)
			{
				continue;
			}

			if (TagInfo->Priority > HighestPri)
			{
				SelectedTagInfo = *TagInfo;
				HighestPri = TagInfo->Priority;
			}
		}

		if (HighestPri >= 0)
		{
			HexActor->SetDebugColor(SelectedTagInfo.DebugColor);
		}
	}
#endif
}