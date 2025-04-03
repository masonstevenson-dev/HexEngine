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

#include "Tools/HxlbGridTool.h"

#include "Actor/HxlbHexManager.h"
#include "Drawing/LineSetComponent.h"
#include "Drawing/PreviewGeometryActor.h"
#include "EngineUtils.h"
#include "Foundation/HxlbHexIterators.h"
#include "FunctionLibraries/HxlbMath.h"
#include "HexLibEditorLoggingDefs.h"
#include "HxlbEditorConstants.h"
#include "InteractiveToolManager.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "BaseGizmos/GizmoMath.h"
#include "Foundation/HxlbHexMap.h"
#include "FunctionLibraries/HxlbEditorUtils.h"
#include "FunctionLibraries/HxlbLandscapeUtil.h"
#include "Macros/HexLibLoggingMacros.h"

using namespace UE::Geometry;

using HexMath = UHxlbMath;

#define LOCTEXT_NAMESPACE "UHxlbGridTool"

namespace HxlbGridToolLocals
{
	const FString GridLineSetID(TEXT("GridLines"));
}

void UHxlbGridTool::Setup()
{
	Super::Setup();

	bClearShowFlagOverrides = true;

	if (!FindHexManager())
	{
		bIsInitialized = false;
		return;
	}

	FinishSetup();
}

void UHxlbGridTool::Shutdown(EToolShutdownType ShutdownType)
{
	HXLB_LOG(LogHxlbEditor, Warning, TEXT("HxlbGridTool: Shutting down."));
	
	ShutdownGridPreview();

	if (CachedManager.IsValid())
	{
		CachedManager->OnMapEditorUpdate.RemoveDynamic(this, &ThisClass::OnEditorMapChange);
	}
	
	UInteractiveTool::Shutdown(ShutdownType);
}

void UHxlbGridTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	if (!bIsInitialized)
	{
		return;
	}
	DrawHexGrid();
}

void UHxlbGridTool::OnTick(float DeltaTime)
{
	Super::OnTick(DeltaTime);

	// This is to fix a problem where entering this tool causes gridlines to be much less visible. It turns out this is
	// caused by UEditorInteractiveToolsContext::SetEditorStateForTool() overriding engine showflags (TemporalAA
	// and MotionBlur) due to a conflict with the legacy PrimitiveDrawInterface (PDI). AFAICT, tools that use the
	// newer InteractiveToolFramework don't even use the PDI, so it should be safe to reenable.
	if (bClearShowFlagOverrides)
	{
		auto& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();

		if (LevelEditor.IsValid())
		{
			for (const TSharedPtr<SLevelViewport>& Viewport : LevelEditor->GetViewports())
			{
				if (Viewport.IsValid())
				{
					Viewport->GetAssetViewportClient().DisableOverrideEngineShowFlags();
				}
			}
			
			bClearShowFlagOverrides = false;
		}
	}
	
	AHxlbHexManager* HexManager = FindHexManager(true);
	bool bHasHexManager = HexManager != nullptr;
	bool bHasLandscape = bHasHexManager && HexManager->MapSettings().OverlaySettings.TargetLandscape.IsValid();
	
	if (bHasHexManager)
	{
		FText ClearMessage = HxlbEditorUtil::ComposeToolWarning(EHxlbToolWarningAction::CLEAR, EHxlbToolWarningId::HexManagerMissing);
		GetToolManager()->DisplayMessage(ClearMessage, EToolMessageLevel::UserWarning);
	}
	else
	{
		FText WarningText = HxlbEditorUtil::ComposeToolWarning(EHxlbToolWarningAction::DISPLAY, EHxlbToolWarningId::HexManagerMissing);
		GetToolManager()->DisplayMessage(WarningText, EToolMessageLevel::UserWarning);
	}
	if (bHasLandscape)
	{
		FText ClearMessage = HxlbEditorUtil::ComposeToolWarning(EHxlbToolWarningAction::CLEAR, EHxlbToolWarningId::LandscapeMissing);
		GetToolManager()->DisplayMessage(ClearMessage, EToolMessageLevel::UserWarning);
	}
	else
	{
		FText WarningText = HxlbEditorUtil::ComposeToolWarning(EHxlbToolWarningAction::DISPLAY, EHxlbToolWarningId::LandscapeMissing);
		GetToolManager()->DisplayMessage(WarningText, EToolMessageLevel::UserWarning);
	}
}

AHxlbHexManager* UHxlbGridTool::FindHexManager(bool bCachedOnly)
{
	AHxlbHexManager* CurrentManager = CachedManager.Get();
	
	if (CurrentManager && CurrentManager->MapComponent != nullptr)
	{
		return CurrentManager;
	}
	CachedManager = nullptr;

	if (bCachedOnly)
	{
		return nullptr;
	}

	if (!TargetWorld || TargetWorld->WorldType != EWorldType::Editor)
	{
		return nullptr;
	}

	// Assigns the first valid HexManager we find.
	for (TActorIterator<AHxlbHexManager> ActorItr(TargetWorld); ActorItr; ++ActorItr)
	{
		AHxlbHexManager* ManagerActor = *ActorItr;
		if (!ManagerActor || !ManagerActor->MapComponent)
		{
			continue;
		}

		CachedManager = ManagerActor;
		return ManagerActor;
	}

	return nullptr;
}

UPreviewGeometry* UHxlbGridTool::InitializeGridPreview()
{
	if (!GridPreviewInternal)
	{
		GridPreviewInternal = NewObject<UPreviewGeometry>();
		GridPreviewInternal->CreateInWorld(TargetWorld, CachedManager->GetTransform());
	}

	GridPreviewInternal->AddLineSet(HxlbGridToolLocals::GridLineSetID);
	bIsDirty = true;

	return GridPreviewInternal;
}

void UHxlbGridTool::ClearGridPreview()
{
	if (!GridPreviewInternal)
	{
		return;
	}

	for (const auto& LineSetKV : GridPreviewInternal->LineSets)
	{
		LineSetKV.Value->Clear();	
	}
}

void UHxlbGridTool::ShutdownGridPreview()
{
	if (!GridPreviewInternal)
	{
		return;
	}

	GridPreviewInternal->Disconnect();
	GridPreviewInternal = nullptr;
}

void UHxlbGridTool::FinishSetup()
{
	if (bIsInitialized)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("Attempted to re-initialize a UHxlbGridTool."));
		return;
	}
	
	InitializeGridPreview();

	CachedManager->OnMapEditorUpdate.AddUniqueDynamic(this, &ThisClass::OnEditorMapChange);
	
	HXLB_LOG(LogHxlbEditor, Log, TEXT("HxlbGridTool: finished setup."));
	bIsInitialized = true;
}

bool UHxlbGridTool::ValidateHex(const FIntPoint& HexCoords, const bool bForceIgnoreGrid)
{
	AHxlbHexManager* HexManager = CachedManager.Get();
	if (!HexManager || !CachedManager->MapComponent)
	{
		return false;
	}
	
	return bForceIgnoreGrid || HexManager->MapComponent->IsValidAxialCoord(HexCoords);
}

FHxlbHexCoord UHxlbGridTool::GetHitGridHex(const FRay& WorldRay, const bool bForceIgnoreGrid)
{
	FHxlbHexCoord HexCoord;
	
	if (!bIsInitialized)
	{
		return HexCoord;
	}
	AHxlbHexManager* HexManager = FindHexManager(/*bCachedOnly=*/true);
	if (!HexManager)
	{
		return HexCoord;
	}
	
	FHxlbMapSettings& MapSettings = HexManager->MapComponent->MapSettings;
	bool bTryLandscapeTrace = MapSettings.bEnableOverlay;
	bool bTryFallbackTrace = false;
	
	if (bTryLandscapeTrace)
	{
		FHxlbLandscapeTraceContext TraceContext;
		TraceContext.Landscape = MapSettings.OverlaySettings.TargetLandscape.Get();
		TraceContext.TraceStart = WorldRay.Origin;
		TraceContext.TraceDirection = WorldRay.Direction;

		FVector IntersectionPoint;
		bool bHitLandscape = HxlbLandscapeUtil::LandscapeTrace(TraceContext, IntersectionPoint);
		
		if (bHitLandscape)
		{
			FIntPoint RawHexCoord = HexMath::WorldToAxial(IntersectionPoint, HexManager->MapComponent->GetHexSize());
			if(ValidateHex(RawHexCoord, bForceIgnoreGrid))
			{
				HexCoord.Set(RawHexCoord);
			}
		}
		else
		{
			bTryFallbackTrace = true;
		}
	}

	if (!bTryLandscapeTrace || bTryFallbackTrace)
	{
		FTransform ManagerTransform = HexManager->GetTransform();

		bool bHitPlane = false;
		FVector IntersectionPoint;
		GizmoMath::RayPlaneIntersectionPoint(
			ManagerTransform.GetLocation(),
			ManagerTransform.GetLocation().UpVector,
			WorldRay.Origin,
			WorldRay.Direction,
			bHitPlane,
			IntersectionPoint
		);

		if (bHitPlane)
		{
			FIntPoint RawHexCoord = HexMath::WorldToAxial(IntersectionPoint, HexManager->MapComponent->GetHexSize());
			if(ValidateHex(RawHexCoord, bForceIgnoreGrid))
			{
				HexCoord.Set(RawHexCoord);
			}
		}
	}

	return HexCoord;
}

void UHxlbGridTool::DrawHexGrid()
{
	if (!bIsInitialized)
	{
		return;
	}
	if (!GridPreviewInternal)
	{
		return;
	}

	AHxlbHexManager* HexManager = FindHexManager(/*bCachedOnly=*/true);
	if (!HexManager)
	{
		return;
	}
	if (HexManager->MapComponent->MapSettings.bEnableOverlay)
	{
		// If the overlay is enabled, we don't draw the grid using preview geometry.
		return;
	}

	GetToolManager()->GetContextQueriesAPI()->GetCurrentViewState(CameraState);
	
	TObjectPtr<UHxlbHexMapComponent> HexMap = HexManager->MapComponent.Get();
	FIntPoint AxialCoordinates = HexMath::WorldToAxial(CameraState.Position, HexMap->GetHexSize());
	double HexSize = HexMap->GetHexSize();

	bool bShouldRedraw = bIsDirty;

	// For unbounded maps, we redraw whenever the camera position changes to a new hex. Otherwise, we only redraw when
	// something changes on the HexMap.
	if (!bShouldRedraw && HexMap->MapSettings.Shape == EHexMapShape::Unbounded)
	{
		if (AxialCoordinates != CurrentCameraAxialCoords)
		{
			CurrentCameraAxialCoords = AxialCoordinates;
			bShouldRedraw = true;
		}
	}

	if (!bShouldRedraw)
	{
		return;
	}

	ULineSetComponent* LineSet = GridPreviewInternal->FindLineSet(HxlbGridToolLocals::GridLineSetID);
	if (!LineSet)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbGridTool::DrawHexGrid(): LineSet %s is missing."), *HxlbGridToolLocals::GridLineSetID);
		return;
	}
	LineSet->Clear();

	const FColor GridLineColor(190, 190, 190);

	auto Iterator = HexMap->GetGridIterator(CurrentCameraAxialCoords);
	while(Iterator && Iterator->Next())
	{
		FIntPoint HexCoords = Iterator->Get();
		DrawHex(*LineSet, HexMath::AxialToWorld(HexCoords, HexSize), HexSize, GridLineColor);
	}

	bIsDirty = false;
}


void UHxlbGridTool::DrawHex(
	ULineSetComponent& LineSet,
	const FVector& Center,
	const double HexSize,
	const FColor CellColor,
	const float ZOffset,
	ALandscape* Landscape
)
{
	TArray<FVector> Corners;
	
	for (int CornerIndex = 0; CornerIndex < 6; CornerIndex++)
	{
		FVector CurrentCorner = HexMath::GetHexCorner(Center, HexSize, CornerIndex);
		if (Landscape)
		{
			TOptional<float> LandscapeHeight = Landscape->GetHeightAtLocation(CurrentCorner, EHeightfieldSource::Editor);
			if (LandscapeHeight.IsSet())
			{
				CurrentCorner.Z = LandscapeHeight.GetValue();
			}
			else
			{
				HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbGridTool::DrawHex(): Unable to sample landscape height."));
			}
		}

		CurrentCorner.Z += ZOffset;
		Corners.Add(CurrentCorner);
	}

	const double CellThickness = 2;
	const double CellDepthBias = 0.1;
	
	for (int StartCorner = 0; StartCorner < 6; StartCorner++)
	{
		int EndCorner = (StartCorner + 1) % 6;
		LineSet.AddLine(Corners[StartCorner], Corners[EndCorner], CellColor, CellThickness, CellDepthBias);
	}
}

void UHxlbGridTool::OnEditorMapChange()
{
	bIsDirty = true;
}

#undef LOCTEXT_NAMESPACE