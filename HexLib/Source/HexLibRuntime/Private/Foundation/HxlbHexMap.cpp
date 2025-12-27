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

#include "Foundation/HxlbHexMap.h"

#include "HexLibRuntimeLoggingDefs.h"
#include "HxlbGameplayTags.h"
#include "Actor/HxlbHexActor.h"
#include "Foundation/HxlbHexIterators.h"
#include "FunctionLibraries/HxlbMath.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#include "Foundation/HxlbTypes.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TextureResource.h"
#include "Macros/HexLibLoggingMacros.h"
#include "UObject/ConstructorHelpers.h"

using HexMath = UHxlbMath;

FHxlbMapSettings::FHxlbMapSettings()
{
	DebugTag = HxlbGameplayTags::TAG_HEXGAME_MAP_ZONE;	
}

UHxlbHexMapComponent::UHxlbHexMapComponent(const FObjectInitializer& Initializer): Super(Initializer)
{
#if WITH_EDITORONLY_DATA
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OverlayMaterial_Finder(TEXT("/HxLib/Materials/MI_DefaultHexOverlay.MI_DefaultHexOverlay"));
	MapSettings.OverlaySettings.OverlayMaterial = OverlayMaterial_Finder.Object;
	
	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> LandscapeRT_Finder(TEXT("/HxLib/RenderTargets/RT_DefaultHexOverlay_2K.RT_DefaultHexOverlay_2K"));
	MapSettings.OverlaySettings.LandscapeHeightRT = LandscapeRT_Finder.Object;

	static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> PerHexDataRT_Finder(TEXT("/HxLib/RenderTargets/RT_HexInfo_256.RT_HexInfo_256"));
	MapSettings.OverlaySettings.PerHexDataRT = PerHexDataRT_Finder.Object;
#endif
}

void UHxlbHexMapComponent::InitGridData(FVector NewGridOrigin)
{
	GridOrigin = HexMath::WorldToAxial(NewGridOrigin, MapSettings.HexSize, MapSettings.HexOrientation);
}

bool UHxlbHexMapComponent::IsValidAxialCoord(FIntPoint AxialCoord)
{
	if (MapSettings.GridMode == EHexGridMode::Landscape)
	{
		// Check valid in texture
		UTextureRenderTarget2D* HexInfoRT = GetHexInfoRT();
		if (!HexInfoRT)
		{
			return false;
		}

		FIntPoint TextureCoord;
		if (!HexMath::AxialToTexture(AxialCoord, HexInfoRT->SizeX, HexInfoRT->SizeY, TextureCoord))
		{
			return false;
		}
		
		// Check valid in landscape (note: this assumes 0,0 is in the center of the landscape)
		if (LandscapeHalfLengthCm > 0)
		{
			FVector WorldCoord = HexMath::AxialToWorld(AxialCoord, MapSettings.HexSize);
			if (
				WorldCoord.X < -LandscapeHalfLengthCm ||
				WorldCoord.X > LandscapeHalfLengthCm ||
				WorldCoord.Y < -LandscapeHalfLengthCm ||
				WorldCoord.Y > LandscapeHalfLengthCm
			)
			{
				return false;
			}
		}
		else
		{
			HXLB_LOG(LogHxlbRuntime, Error, TEXT("LandscapeHalfLengthCm is not initialized. Skipping landscape bounds check."));
		}
	}
	
	switch (MapSettings.Shape)
	{
	case EHexMapShape::Unbounded:
		return true;
	case EHexMapShape::Hexagonal:
		return HexMath::AxialLength(AxialCoord) <= MapSettings.HaxagonalMapSettings.Radius;
	case EHexMapShape::Rectangular:
		{
			int32 MinCol = -MapSettings.RectangularHexMapSettings.Width - FMath::Floor(HEX_R(AxialCoord) / 2.0);
			int32 MaxCol = MapSettings.RectangularHexMapSettings.Width - FMath::Floor(HEX_R(AxialCoord) / 2.0);
			int32 MinRow = -MapSettings.RectangularHexMapSettings.Height;
			int32 MaxRow = MapSettings.RectangularHexMapSettings.Height;
			return (
				HEX_Q(AxialCoord) >= MinCol &&
				HEX_Q(AxialCoord) <= MaxCol &&
				HEX_R(AxialCoord) >= MinRow &&
				HEX_R(AxialCoord) <= MaxRow
			);
		}
	default:
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("Unknown map shape."));		
	}
	
	return false;
}

UHxlbHexIteratorWrapper* UHxlbHexMapComponent::GetGridIterator(FIntPoint CameraCoord)
{
	switch (MapSettings.Shape)
	{
	case EHexMapShape::Unbounded:
		{
			int32 CameraGridRadius = 50;
			auto IteratorWrapper = NewObject<UHxlbRadialIW>();
			IteratorWrapper->Iterator = FHxlbRadialIterator(CameraCoord, CameraGridRadius);
			return IteratorWrapper;
		}
	case EHexMapShape::Hexagonal:
		{
			auto IteratorWrapper = NewObject<UHxlbRadialIW>();
			IteratorWrapper->Iterator = FHxlbRadialIterator(GridOrigin, MapSettings.HaxagonalMapSettings.Radius);
			return IteratorWrapper;
		}
	case EHexMapShape::Rectangular:
		{
			auto IteratorWrapper = NewObject<UHxlbRectangularIW>();
			IteratorWrapper->Iterator = FHxlbRectangularIterator(GridOrigin, MapSettings.RectangularHexMapSettings.Width, MapSettings.RectangularHexMapSettings.Height);
			return IteratorWrapper;
		}
	default:
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("Unknown map shape."));		
	}

	return NewObject<UHxlbHexIteratorWrapper>();
}

UHxlbHex* UHxlbHexMapComponent::GetOrCreateHex(FIntPoint AxialCoord)
{
	UHxlbHex* HexPtr = HexData.FindRef(AxialCoord);

	if (HexPtr)
	{
		return HexPtr;
	}
	
	auto* NewHex = NewObject<UHxlbHex>(this, MapSettings.DefaultHexClass.Get());
	NewHex->InitialzeHex(this, AxialCoord);
	HexData.Add(AxialCoord, NewHex);
	return NewHex;
}

void UHxlbHexMapComponent::ClearHexActors()
{
	for (auto HexIterator = HexData.CreateIterator(); HexIterator; ++HexIterator)
	{
		UHxlbHex* Hex = HexIterator.Value();

		if (!Hex)
		{
			continue;
		}

		Hex->ClearHexActor();
	}
}


UHxlbHex* UHxlbHexMapComponent::CreateBulkEditProxy()
{
	BulkEditProxy = NewObject<UHxlbHex>(this, MapSettings.DefaultHexClass.Get());
	return BulkEditProxy;
}

void UHxlbHexMapComponent::ClearBulkEditProxy()
{
	BulkEditProxy = nullptr;
}

void UHxlbHexMapComponent::CommitBulkEdits()
{
	if (SelectionState.SelectedHexes.Num() == 0)
	{
		return;
	}
	if (!BulkEditProxy)
	{
		return;
	}

	for (FIntPoint AxialCoord : SelectionState.SelectedHexes)
	{
		UHxlbHex* NewHex = DuplicateObject(BulkEditProxy, this);
		NewHex->InitialzeHex(this, AxialCoord);
		HexData.Add(AxialCoord, NewHex);
	}
}

#if WITH_EDITOR
void UHxlbHexMapComponent::Update(FHxlbMapSettings& NewMapSettings, FHxlbHexMapUpdateOptions UpdateOptions)
{
	MapSettings = NewMapSettings;
	
	if (UpdateOptions.bForceLandscapeRTRefresh)
	{
		RefreshLandscapeData();
	}
	if (UpdateOptions.bRefreshGridlines)
	{
		RefreshGridlines();
	}
	
	ALandscape* TargetLandscape = MapSettings.OverlaySettings.TargetLandscape.Get();
	if (TargetLandscape)
	{
		int32 LandscapeResolution;
		if (ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo())
		{
			FIntRect LandscapeRect;
			LandscapeInfo->GetLandscapeExtent(LandscapeRect.Min.X, LandscapeRect.Min.Y, LandscapeRect.Max.X, LandscapeRect.Max.Y);
			LandscapeResolution = LandscapeRect.Size().X;
		}
		else
		{
			LandscapeResolution = TargetLandscape->GetBoundingRect().Size().X;
		}

		FVector LandscapeActorScale = TargetLandscape->GetActorScale();
		LandscapeHalfLengthCm = (LandscapeActorScale.X * LandscapeResolution) / 2;
	}
	
	for (auto HexIterator = HexData.CreateIterator(); HexIterator; ++HexIterator)
	{
		UHxlbHex* Hex = HexIterator.Value();

		if (!Hex)
		{
			continue;
		}
		
		Hex->SetHexSize(MapSettings.HexSize);

		if (!Hex->GetHexActor())
		{
			continue;
		}
		
		if (UpdateOptions.bRefreshTagSettings)
		{
			Hex->ProcessGameplayTags();
		}
	}
}

void UHxlbHexMapComponent::RefreshLandscapeData()
{
	ALandscape* TargetLandscape = MapSettings.OverlaySettings.TargetLandscape.Get();
	if (!TargetLandscape)
	{
		return;
	}
	
	RefreshLandscapeRT(TargetLandscape);
}

void UHxlbHexMapComponent::HoverHex(FHxlbHexCoordDelta& HoverState, bool bIsSelecting)
{
	if (HoverState.HasPrev())
	{
		FIntPoint Prev = HoverState.GetPrev();

		if (SelectionState.SelectedHexes.Contains(Prev) && SelectionState.bWriteSelectedToRT)
		{
			SetHexHighlightType(HoverState.GetPrev(), EHxlbHighlightType::Selected);
		}
		else
		{
			SetHexHighlightType(HoverState.GetPrev(), EHxlbHighlightType::None);
		}
	}
	if (HoverState.HasCurrent())
	{
		if (bIsSelecting)
		{
			SetHexHighlightType(HoverState.GetCurrent(), EHxlbHighlightType::HoverSelection);
		}
		else
		{
			SetHexHighlightType(HoverState.GetCurrent(), EHxlbHighlightType::HoverRemoval);
		}
	}
}

void UHxlbHexMapComponent::UpdateSelection(FHxlbSelectionState& NewSelectionState)
{
	TArray<FIntPoint> HexCoords;
	TArray<uint16> HexInfos;
	TSet<FIntPoint> HexesToClear;
	HexesToClear.Append(SelectionState.SelectingHexes);
	HexesToClear.Append(SelectionState.RemovingHexes);
	HexesToClear.Append(SelectionState.SelectedHexes);

	// possibilities: A hex was newly added (in NewSelectionState but not in SelectionState)
	//                A hex has moved (in both, but selection set has changed)
	//                A hex has been removed (in SelectionState, but not in NewSelectionState)
	//
	// Theoretically, Selecting, Removing, and Selected should all be unique. However, if this is not the case, the
	// resulting priority will be (from high to low):
	//   1) Removing
	//   2) Selected
	//   3) Selecting
	for (FIntPoint HexCoord : NewSelectionState.SelectingHexes)
	{
		HxlbPackedData::FHexInfo Info{};
		if (NewSelectionState.bWriteSelectingToRT)
		{
			Info.HighlightType = static_cast<uint8>(EHxlbHighlightType::Selecting);
		}
		else
		{
			Info.HighlightType = static_cast<uint8>(EHxlbHighlightType::None);
		}

		HexCoords.Add(HexCoord);
		HexInfos.Add(Info.Raw);
		HexesToClear.Remove(HexCoord);
	}
	for (FIntPoint HexCoord : NewSelectionState.SelectedHexes)
	{
		HxlbPackedData::FHexInfo Info{};
		if (NewSelectionState.bWriteSelectedToRT)
		{
			Info.HighlightType = static_cast<uint8>(EHxlbHighlightType::Selected);
		}
		else
		{
			Info.HighlightType = static_cast<uint8>(EHxlbHighlightType::None);
		}

		HexCoords.Add(HexCoord);
		HexInfos.Add(Info.Raw);
		HexesToClear.Remove(HexCoord);
	}
	for (FIntPoint HexCoord : NewSelectionState.RemovingHexes)
	{
		HxlbPackedData::FHexInfo Info{};
		if (NewSelectionState.bWriteRemovingToRT)
		{
			Info.HighlightType = static_cast<uint8>(EHxlbHighlightType::Removing);
		}
		else
		{
			Info.HighlightType = static_cast<uint8>(EHxlbHighlightType::None);
		}

		HexCoords.Add(HexCoord);
		HexInfos.Add(Info.Raw);
		HexesToClear.Remove(HexCoord);
	}
	for (FIntPoint HexCoord : HexesToClear)
	{
		HxlbPackedData::FHexInfo Info{};
		Info.HighlightType = static_cast<uint8>(EHxlbHighlightType::None);

		HexCoords.Add(HexCoord);
		HexInfos.Add(Info.Raw);
	}
	
	uint16 InfoMask = HxlbPackedData::FHexInfo(/*R=*/0, /*G=*/HxlbPackedData::FM_HighlightType).Raw;
	
	UTextureRenderTarget2D* PerHexDataRT = GetHexInfoEditorRT();
	WriteHexInfo_Bulk16(PerHexDataRT, HexCoords, HexInfos, InfoMask);

	SelectionState = NewSelectionState;
}
#endif // WITH_EDITOR

void UHxlbHexMapComponent::RefreshLandscapeRT(ALandscape* TargetLandscape)
{
	UTextureRenderTarget2D* LandscapeRT = MapSettings.OverlaySettings.LandscapeHeightRT;

	if (LandscapeRT == nullptr)
	{
		return;
	}

	if (MapSettings.GridMode != EHexGridMode::Landscape || !MapSettings.OverlaySettings.TargetLandscape.IsValid())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(GetOwner(), LandscapeRT);
		return;
	}
	
	TargetLandscape->RenderHeightmap(FTransform::Identity, FBox2d(ForceInit), LandscapeRT);
}

UTextureRenderTarget2D* UHxlbHexMapComponent::GetHexInfoRT()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	if (World->WorldType == EWorldType::Editor)
	{
		return GetHexInfoEditorRT();
	}
	else if (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE)
	{
		return GetHexInfoGameRT();
	}
	
	return nullptr;
}

UTextureRenderTarget2D* UHxlbHexMapComponent::GetHexInfoEditorRT()
{
	UTextureRenderTarget2D* PerHexDataRT = MapSettings.OverlaySettings.PerHexDataRT.Get();
	if (!PerHexDataRT)
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("PerHexDataRT is invalid."));
		return nullptr;
	}
	if (PerHexDataRT->RenderTargetFormat != HxlbPackedData::RequiredRTF)
	{
		HXLB_LOG(LogHxlbRuntime, Error,
			TEXT("Expected PerHexData render target to have a format of %s. Instead, got %s"),
			*UEnum::GetValueAsString(HxlbPackedData::RequiredRTF),
			*UEnum::GetValueAsString(PerHexDataRT->RenderTargetFormat));
		return nullptr;
	}
	
	int32 RTFormatSizeBytes = GPixelFormats[PerHexDataRT->GetFormat()].BlockBytes;
	int32 HexInfoSizeBytes = sizeof(HxlbPackedData::FHexInfo);
	if (HexInfoSizeBytes != RTFormatSizeBytes)
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("Expected HexInfo struct size in bytes (%d) to match the %s render target format size in bytes (%d)."),
			HexInfoSizeBytes, *UEnum::GetValueAsString(PerHexDataRT->RenderTargetFormat), RTFormatSizeBytes)
		return nullptr;
	}

	return PerHexDataRT;
}

UTextureRenderTarget2D* UHxlbHexMapComponent::GetHexInfoGameRT()
{
	// stubbed out. Override this fn to define a custom RT for your game.
	return nullptr;
}

void UHxlbHexMapComponent::RefreshGridlines()
{
	UTextureRenderTarget2D* PerHexDataRT = GetHexInfoRT();
	if (!PerHexDataRT)
	{
		return;
	}
	
	TArray<HxlbPackedData::FHexInfo> HexInfoBuffer;
	int32 BufferSize = PerHexDataRT->SizeX * PerHexDataRT->SizeY;

	if (MapSettings.Shape == EHexMapShape::Unbounded)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_PrepPerHexData_Unbounded);

		HxlbPackedData::FHexInfo DefaultHexInfo{};
		DefaultHexInfo.EdgeFlags = HxlbPackedData::FM_EdgeFlags;
		HexInfoBuffer.Init(DefaultHexInfo, BufferSize);
	}
	else
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_PrepPerHexData_Bounded);
		
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_HexDataBufferReservation_Bounded);

			// Note that AddZeroed() calls ResizeGrow() which does dynamic allocation vs Init() which calls ResizeTo()
			// (the same thing that Array.Reserve() calls under the hood). However, from manual testing, AddZeroed seems
			// to result in the same allocation size for our RT buffer and is WAY faster (2.6ms vs 38.2ms for RTF_R8, 4K)
			// due to it calling MemZero() instead of setting each element in a loop.
			HexInfoBuffer.AddZeroed(BufferSize);

			// HXLB_LOG(LogHxlbRuntime, Error, TEXT("HexDataBuffer Size: %d,  Max: %d"), HexDataBuffer.Num(), HexDataBuffer.Max());
		}
		
		TArray<FIntPoint> FullHexes;

		// Find valid hex coords and write them into the buffer
		{
			// 1) reserve and emplace every time				| 285 ms (RTF_R8, 4K)
			// 2) reserve zeroed and only set for valid hexes	| 267 ms (RTF_R8, 4K)
			// 3) only loop through valid hexes					| 29 us (RTF_R8, 4K)
			TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_HexCoordValidation);

			auto Iterator = GetGridIterator(FIntPoint::ZeroValue);
			int32 InvalidTextureCoords = 0;
			int32 InvalidBufferIndices = 0;
			while(Iterator && Iterator->Next())
			{
				FIntPoint HexCoord = Iterator->Get();
				if (!IsValidAxialCoord(HexCoord))
				{
					continue;
				}
				FIntPoint TextureCoord;
				if (!HexMath::AxialToTexture(HexCoord, PerHexDataRT->SizeX, PerHexDataRT->SizeY, TextureCoord))
				{
					InvalidTextureCoords++;
					continue;
				}

				int32 BufferIndex = HexMath::TextureToPixelBuffer(TextureCoord, PerHexDataRT->SizeX);
				if (BufferIndex < 0 || BufferIndex >= PerHexDataRT->SizeX * PerHexDataRT->SizeY)
				{
					InvalidBufferIndices++;
					continue;
				}

				HexInfoBuffer[BufferIndex].EdgeFlags = HxlbPackedData::FM_EdgeFlags;
				FullHexes.Add(HexCoord);
			}

			if (InvalidTextureCoords > 0)
			{
				HXLB_LOG(LogHxlbRuntime, Error, TEXT("HexCoord converts to invalid texture coord (%d)."), InvalidTextureCoords);
			}
			if (InvalidBufferIndices > 0)
			{
				HXLB_LOG(LogHxlbRuntime, Error, TEXT("invalid buffer index (%d)."), InvalidBufferIndices);
			}
		}

		// find edge hexes and write them into the buffer
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_ComputeNeighborEdgeBoundaries);
			
			for (FIntPoint HexCoord : FullHexes)
			{
				auto AdjacencyIterator = FHxlbRadialIterator(HexCoord, 1);
				int32 InvalidBufferIndices = 0;
			
				while (AdjacencyIterator.Next())
				{
					FIntPoint AdjacentHex = AdjacencyIterator.Get();
					if (AdjacentHex == HexCoord)
					{
						continue;
					}
					
					int32 BufferIndex;
					uint32 BoundaryOffset = 1; // Override the default boundary offset to allow searching neighbors on the "true" boundary
					if (!HexMath::AxialToPixelBuffer(AdjacentHex, PerHexDataRT->SizeX, PerHexDataRT->SizeY, BufferIndex, BoundaryOffset))
					{
						InvalidBufferIndices++;
						continue;
					}
					if ((HexInfoBuffer[BufferIndex].EdgeFlags & HxlbPackedData::FM_EdgeFlags) == HxlbPackedData::FM_EdgeFlags)
					{
						continue;
					}
				
					uint8 AdjacentEdgeIndex = HexMath::NeighborEdgeIndex(HexCoord, AdjacentHex, MapSettings.HexSize);
					if (AdjacentEdgeIndex < 0 || AdjacentEdgeIndex > 5)
					{
						HXLB_LOG(LogHxlbRuntime, Error, TEXT("Invalid edge index"));
						continue;
					}

					HexInfoBuffer[BufferIndex].EdgeFlags |= (1 << AdjacentEdgeIndex);
				}

				if (InvalidBufferIndices > 0)
				{
					HXLB_LOG(LogHxlbRuntime, Error, TEXT("found invalid buffer indices while computing adjacent gridlines (%d)."), InvalidBufferIndices);
				}
			}
		}
	}
	
	if (HexInfoBuffer.Num() != BufferSize)
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("Hex info buffer has invalid size!"));
		return;
	}

	ENQUEUE_RENDER_COMMAND(WriteHexGridDataToRT)(
		[InRT = PerHexDataRT, HexData = MoveTemp(HexInfoBuffer)](FRHICommandListImmediate& RHICmdList)
		{
			if (!InRT)
			{
				return;
			}
			FTextureRenderTargetResource* RTResource = InRT->GetRenderTargetResource();
			if (!RTResource)
			{
				return;
			}
			
			const FTexture2DRHIRef TextureRHI = RTResource->GetTexture2DRHI();
			const int32 BlockBytes = GPixelFormats[TextureRHI->GetFormat()].BlockBytes;
			if (BlockBytes != sizeof(HxlbPackedData::FHexInfo))
			{
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("Mismatch between hex info texture size and hex info packed data struct."));
				return;
			}
			int32 SizeX = TextureRHI->GetDesc().Extent.X;
			int32 SizeY = TextureRHI->GetDesc().Extent.Y;
			if (SizeX != InRT->SizeX || SizeY != InRT->SizeY)
			{
				// Probably not needed to check this, but I'm not sure if it is 100% safe to use these interchangeably.
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("Mismatch between RT size and TextureRHI size."));
				return;
			}
			
			uint32 BufferRowStride = 0;
			void* RTDestBuffer = RHICmdList.LockTexture2D(TextureRHI, 0, RLM_WriteOnly, BufferRowStride, false, false);
			uint32 TextureRowStride = SizeX * BlockBytes;
			if (BufferRowStride == TextureRowStride)
			{
				FMemory::Memcpy(RTDestBuffer, HexData.GetData(), SizeX * SizeY * BlockBytes);
			}
			else if (BufferRowStride > TextureRowStride)
			{
				// Sanity check. if BufferRowStride is ever > TextureRowStride, that means there are extra pad bytes in
				// the buffer that we need to account for. If this error ever pops up, implement a copy-by-row approach
				// that copies the smaller chunk of data (TextureRowStride) and then increments by the larger chunk of
				// data.
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("BufferRowStride > TextureRowStride."));
			}
			else
			{
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("BufferRowStride < TextureRowStride."));
			}
			RHICmdList.UnlockTexture2D(TextureRHI, 0, false, false);
		}
	);
}

void UHxlbHexMapComponent::WriteHexInfo_Bulk16(UTextureRenderTarget2D* PerHexDataRT, TArray<FIntPoint>& HexCoords, TArray<uint16> RawInfoArray, uint16 BitMask)
{
	if (HexCoords.Num() == 0)
	{
		return;
	}
	if (HexCoords.Num() != RawInfoArray.Num())
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("UHxlbHexMapComponent::WriteHexInfo HexCoords.Num() != HexInfos.Num()."));
		return;
	}
	if (!PerHexDataRT)
	{
		return;
	}

	bool bUseBatchedUpdate = HexCoords.Num() > kBatchedUpdateThreshold;
	if (!bUseBatchedUpdate)
	{
		for (int Index = 0; Index < HexCoords.Num(); Index++)
		{
			WriteHexInfo_16(PerHexDataRT, HexCoords[Index], RawInfoArray[Index], BitMask);
		}
	}
	else
	{
		TArray<int32> BufferIndices;
		TArray<uint16> FilteredInfos;
		BufferIndices.Reserve(HexCoords.Num());
		FilteredInfos.Reserve(HexCoords.Num());
		int32 FailedConversions = 0;

		for (int Index = 0; Index < HexCoords.Num(); Index++)
		{
			int32 BufferIndex;
			if (!HexMath::AxialToPixelBuffer(HexCoords[Index], PerHexDataRT->SizeX, PerHexDataRT->SizeY, BufferIndex))
			{
				FailedConversions++;
				continue;
			}

			BufferIndices.Add(BufferIndex);
			FilteredInfos.Add(RawInfoArray[Index]);
		}

		if (FailedConversions > 0)
		{
			HXLB_LOG(LogHxlbRuntime, Error, TEXT("UHxlbHexMapComponent::WriteHexInfo failed buffer index conversion (%d). Check for invalid pixel coordinates or invalid buffer index."), FailedConversions);
		}

		if (BufferIndices.Num() != FilteredInfos.Num())
		{
			HXLB_LOG(LogHxlbRuntime, Error, TEXT("Mismatch between BufferIndices.Num() and HexInfos.Num()"));
			return;
		}
		
		ENQUEUE_RENDER_COMMAND(WriteHexInfo)(
		[InRT = PerHexDataRT, InBufferIndices = BufferIndices, InHexInfos = FilteredInfos, InBitMask = BitMask](FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_WriteHexInfo_Batch);
			
			if (!InRT)
			{
				return;
			}
			auto* RTResource = InRT->GetRenderTargetResource();
			if (!RTResource)
			{
				return;
			}
			
			const FTexture2DRHIRef TextureRHI = RTResource->GetTexture2DRHI();
			const int32 BlockBytes = GPixelFormats[TextureRHI->GetFormat()].BlockBytes;
			if (BlockBytes != sizeof(HxlbPackedData::FHexInfo))
			{
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("Mismatch between hex info texture size and hex info packed data struct."));
				return;
			}
			int32 SizeX = TextureRHI->GetDesc().Extent.X;
			int32 SizeY = TextureRHI->GetDesc().Extent.Y;
			if (SizeX != InRT->SizeX || SizeY != InRT->SizeY)
			{
				// Probably not needed to check this, but I'm not sure if it is 100% safe to use these interchangeably.
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("Mismatch between RT size and TextureRHI size."));
				return;
			}

			// TODO(): If we ever switch to using a full RGBA texture, it should be fine to just read in FColor values
			//         with ReadSurfaceData. For now, this is a waste because it allocates an extra 16 bytes per pixel
			//         that we never actually use.
			// TArray<FColor> SurfaceData;
			// FIntRect SurfaceRect = FIntRect(0, 0, TextureRHI->GetDesc().Extent.X, TextureRHI->GetDesc().Extent.Y);
			// FReadSurfaceDataFlags SurfaceDataFlags(RCM_MinMax);
			// SurfaceDataFlags.SetLinearToGamma(false); 
			// RHICmdList.ReadSurfaceData(TextureRHI, SurfaceRect, SurfaceData, SurfaceDataFlags);
			
			TArray<HxlbPackedData::FHexInfo> UpdateBuffer;
			UpdateBuffer.SetNumUninitialized(SizeX * SizeY);
			
			uint32 BufferRowStride = 0;
			void* RTReadBuffer = RHICmdList.LockTexture2D(TextureRHI, 0, RLM_ReadOnly, BufferRowStride, false, false);
			uint32 TextureRowStride = SizeX * BlockBytes;
			if (BufferRowStride == TextureRowStride)
			{
				FMemory::Memcpy(UpdateBuffer.GetData(), RTReadBuffer, SizeX * SizeY * BlockBytes);
				RHICmdList.UnlockTexture2D(TextureRHI, 0, false, false);
				
				if (UpdateBuffer.Num() < SizeX * SizeY)
				{
					HXLB_LOG(LogHxlbRenderThread, Error, TEXT("UpdateBuffer is smaller than expected (got: %d want: %d)"), UpdateBuffer.Num(), SizeX * SizeY);
					return;
				}
			}
			else if (BufferRowStride > TextureRowStride)
			{
				// See comment in RefreshGridlines()
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("BufferRowStride > TextureRowStride."));
				RHICmdList.UnlockTexture2D(TextureRHI, 0, false, false);
				return;
			}
			else
			{
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("BufferRowStride < TextureRowStride."));
				RHICmdList.UnlockTexture2D(TextureRHI, 0, false, false);
				return;
			}

			for (int LookupIndex = 0; LookupIndex < InBufferIndices.Num(); LookupIndex++)
			{
				int32 BufferIndex = InBufferIndices[LookupIndex];
				UpdateBuffer[BufferIndex].Raw = (UpdateBuffer[BufferIndex].Raw & ~InBitMask) | (InHexInfos[LookupIndex] & InBitMask);
			}

			BufferRowStride = 0;
			void* RTWriteBuffer = RHICmdList.LockTexture2D(TextureRHI, 0, RLM_WriteOnly, BufferRowStride, false, false);
			if (BufferRowStride == TextureRowStride)
			{
				FMemory::Memcpy(RTWriteBuffer, UpdateBuffer.GetData(), SizeX * SizeY * BlockBytes);
			}
			else if (BufferRowStride > TextureRowStride)
			{
				// See comment in RefreshGridlines()
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("BufferRowStride > TextureRowStride."));
			}
			else
			{
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("BufferRowStride < TextureRowStride."));
			}
			RHICmdList.UnlockTexture2D(TextureRHI, 0, false, false);
		}
	);
	}
}

void UHxlbHexMapComponent::WriteHexInfo_16(UTextureRenderTarget2D* PerHexDataRT, FIntPoint HexCoord, uint16 RawInfo, uint16 BitMask)
{
	if (!PerHexDataRT)
	{
		return;
	}
	
	FIntPoint PixelCoords;
	if (!HexMath::AxialToTexture(HexCoord, PerHexDataRT->SizeX, PerHexDataRT->SizeY, PixelCoords))
	{
		HXLB_LOG(LogHxlbRuntime, Error, TEXT("UHxlbHexMapComponent::WriteHexInfoSingle invalid pixel coords."));
		return;
	}

	ENQUEUE_RENDER_COMMAND(WriteHexInfoSingle)(
		[InRT = PerHexDataRT, InPixelCoords = PixelCoords, InHexInfo = RawInfo, InBitMask = BitMask](FRHICommandListImmediate& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_WriteHexInfo_Single);
			
			if (!InRT)
			{
				return;
			}
			auto* RTResource = InRT->GetRenderTargetResource();
			if (!RTResource)
			{
				return;
			}
			
			const FTexture2DRHIRef TextureRHI = RTResource->GetTexture2DRHI();
			const int32 BlockBytes = GPixelFormats[TextureRHI->GetFormat()].BlockBytes;
			if (BlockBytes != sizeof(HxlbPackedData::FHexInfo))
			{
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("Mismatch between hex info texture size and hex info packed data struct."));
				return;
			}
			
			TArray<FColor> SurfaceData;
			FIntRect SurfaceRect = FIntRect(InPixelCoords.X, InPixelCoords.Y, InPixelCoords.X+1, InPixelCoords.Y+1);
			FReadSurfaceDataFlags SurfaceDataFlags(RCM_MinMax);
			SurfaceDataFlags.SetLinearToGamma(false); 
			RHICmdList.ReadSurfaceData(TextureRHI, SurfaceRect, SurfaceData, SurfaceDataFlags);

			if (SurfaceData.Num() != 1)
			{
				HXLB_LOG(LogHxlbRenderThread, Error, TEXT("Expected SurfaceData to contain data for a single pixel. Got: %d"), SurfaceData.Num());
				return;
			}

			auto OldHexInfo = HxlbPackedData::FHexInfo(SurfaceData[0]);
			HxlbPackedData::FHexInfo UpdatedHexInfo;
			UpdatedHexInfo.Raw = (OldHexInfo.Raw & ~InBitMask) | (InHexInfo & InBitMask);
			
			FUpdateTextureRegion2D RegionData(InPixelCoords.X, InPixelCoords.Y, 0, 0, 1, 1);
			uint8* RawData = reinterpret_cast<uint8*>(&UpdatedHexInfo);
			RHICmdList.UpdateTexture2D(TextureRHI, 0, RegionData, BlockBytes, RawData);
		}
	);
}

void UHxlbHexMapComponent::SetHexHighlightType(FIntPoint HexCoord, EHxlbHighlightType HighlightType)
{
	HxlbPackedData::FHexInfo HexInfo{};
	HexInfo.HighlightType = static_cast<uint8>(HighlightType);
	uint16 InfoMask = HxlbPackedData::FHexInfo(0, HxlbPackedData::FM_HighlightType).Raw;
	
	UTextureRenderTarget2D* PerHexDataRT = GetHexInfoRT();
	WriteHexInfo_16(PerHexDataRT, HexCoord, HexInfo.Raw, InfoMask);
}
