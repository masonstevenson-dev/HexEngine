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

#include "Tools/HxlbSelectionToolBase.h"

#include "Actor/HxlbHexManager.h"
#include "HexLibEditorLoggingDefs.h"
#include "BaseBehaviors/MouseHoverBehavior.h"
#include "Drawing/LineSetComponent.h"
#include "Drawing/PreviewGeometryActor.h"
#include "EngineUtils.h"
#include "HexLibEditorModule.h"
#include "HxlbEditorConstants.h"
#include "Foundation/HxlbHexMap.h"
#include "FunctionLibraries/HxlbMath.h"
#include "InteractiveToolManager.h"
#include "Landscape.h"
#include "ToolTargetManager.h"
#include "Foundation/HxlbHexIterators.h"
#include "Macros/HexLibLoggingMacros.h"
#include "Subsystems/HxlbEditorMessageChannels.h"
#include "Subsystems/HxlbEditorMessagingSubsystem.h"

#define LOCTEXT_NAMESPACE "UHxlbSelectionToolBase"

using HexMath = UHxlbMath;

namespace HxlbSelectionToolLocals
{
	const FString HoverLineSetID(TEXT("HoverLines"));
	const FString SelectionLineSetID(TEXT("SelectionLines"));
}

#if HXLB_ENABLE_LEGACY_EDITOR_GRID
float FHxlbHighlightContext::GetZOffset() const
{
	float Offset = 0.0;
	
	
	if (Landscape.IsValid())
	{
		Offset += 50.0;
	}
	
	switch (Type)
	{
	case EHxlbHighlightType::HoverSelection:
	case EHxlbHighlightType::HoverRemoval:
		Offset += 3.0;
		break;
	case EHxlbHighlightType::Selected:
	case EHxlbHighlightType::Selecting:
	case EHxlbHighlightType::Removing:
		Offset += 2.0;
		break;
	default:
		HXLB_LOG(LogHxlbEditor, Error, TEXT("FHxlbHighlightContext::GetZOffset(): Unsupported selection type: %s"), *UEnum::GetValueAsString(Type));
		break;
	}

	return Offset;
}
#endif

void FHxlbHighlightContext::ExtractContextInfoFromHexMap(UHxlbHexMapComponent* HexMap, FIntPoint HexCoords)
{
	if (!HexMap)
	{
		return;
	}
	
	// If any additional data has been associated with this hex, we can grab the hex position that is cached on the
	// corresponding data object. Otherwise, we fall back on computing this info based on the hex coordinate.
	UHxlbHex* HexDataPtr = HexMap->GetHexData(HexCoords);
	if (HexDataPtr)
	{
		HexData = HexDataPtr;
	}

#if HXLB_ENABLE_LEGACY_EDITOR_GRID
	HexSize = HexMap->MapSettings.HexSize;
	
	if (HexDataPtr)
	{
		WorldCoords = HexDataPtr->GetWorldCoords();
	}
	else
	{
		WorldCoords = HexMath::AxialToWorld(HexCoords, HexSize);
	}

	if (HexMap->MapSettings.bEnableOverlay)
	{
		Landscape = HexMap->MapSettings.OverlaySettings.TargetLandscape.Get();
	}
#endif
}

void UHxlbSelectionToolActionsBase::Initialize(UHxlbSelectionToolBase* NewParentTool)
{
	ParentTool = NewParentTool;
}

void UHxlbSelectionToolActionsBase::RequestActionFromParent(EHxlbSelectionToolAction Action)
{
	if (ParentTool.IsValid())
	{
		ParentTool->RequestAction(Action);
	}
}

bool UHxlbSelectionToolBuilderBase::CanBuildTool(const FToolBuilderState& SceneState) const
{
	bool MeetsTargetRequirements = SceneState.TargetManager->CountSelectedAndTargetable(SceneState, GetTargetRequirements()) <= 1;
	bool WorldHasHexManager = false;
	
	if (SceneState.World || SceneState.World->WorldType == EWorldType::Editor)
	{
		for (TActorIterator<AHxlbHexManager> ActorItr(SceneState.World); ActorItr; ++ActorItr)
		{
			AHxlbHexManager* ManagerActor = *ActorItr;
			if (!ManagerActor || !ManagerActor->MapComponent)
			{
				continue;
			}

			WorldHasHexManager = true;
			break;
		}
	}

	return MeetsTargetRequirements && WorldHasHexManager;
}

void UHxlbSelectionToolBase::Setup()
{
	Super::Setup();

	if (!bIsInitialized)
	{
		return;
	}

	ConfigureActionProperties();
	ConfigureSettingsProperties();
	SetupInputBehaviors();

	if (auto* MessageSystem = UHxlbEditorMessagingSubsystem::Get())
	{
		MessageSystem->Subscribe(HxlbEditorMessageChannel::CommitBulkEdits, this);
	}
}

void UHxlbSelectionToolBase::Shutdown(EToolShutdownType ShutdownType)
{
	ClearSelection();
	
	if (ToolSettings)
	{
		ToolSettings->SaveProperties(this);
	}

	if (auto* MessageSystem = UHxlbEditorMessagingSubsystem::Get())
	{
		MessageSystem->Unsubscribe(HxlbEditorMessageChannel::CommitBulkEdits, this);
	}
	
	Super::Shutdown(ShutdownType);
}

void UHxlbSelectionToolBase::Render(IToolsContextRenderAPI* RenderAPI)
{
	Super::Render(RenderAPI);
}

FInputRayHit UHxlbSelectionToolBase::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos)
{
	FInputRayHit HitResult;

	if (!bIsInitialized)
	{
		HitResult.bHit = false;
		return HitResult;
	}

	// The user is currently click+dragging a selection
	if (bSelectionInProgress)
	{
		HitResult.bHit = false;
		return HitResult;
	}
	
	HitGridHexCoords = GetHitGridHex(PressPos.WorldRay);
	HitResult.bHit = HitGridHexCoords.IsSet();
	return HitResult;
}

void UHxlbSelectionToolBase::OnBeginHover(const FInputDeviceRay& DevicePos)
{
	UpdateHover();
}

bool UHxlbSelectionToolBase::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	return UpdateHover();	
}

void UHxlbSelectionToolBase::OnEndHover()
{
	ClearHover();
	HitGridHexCoords.Clear();
}

void UHxlbSelectionToolBase::OnUpdateModifierState(int ModifierID, bool bIsOn)
{
	// NOTE: This fn seems to run on tick. Leaving this test in for posterity.
	// FString sIsOn = bIsOn ? "true" : "false";
	// HXLB_LOG(LogHxlbEditor, Error, TEXT("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^: %d %s"), ModifierID, *sIsOn);
	
	if (ModifierID == ShiftModifierID)
	{
		bShiftKeyIsPressed = bIsOn;
	}
	else if (ModifierID == CtrlModifierID)
	{
		bool bOldVal = bCtrlKeyIsPressed;
		bCtrlKeyIsPressed = bIsOn;

		if (bCtrlKeyIsPressed != bOldVal)
		{
			HoverState.ClearCurrent(); // forces hover lines to refresh.
		}
	}
}

FInputRayHit UHxlbSelectionToolBase::CanBeginClickDragSequence(const FInputDeviceRay& PressPos)
{
	FInputRayHit HitResult;
	HitResult.bHit = true;
	return HitResult;
}

void UHxlbSelectionToolBase::OnClickPress(const FInputDeviceRay& PressPos)
{
	if (!bIsInitialized)
	{
		return;
	}
	if (bSelectionInProgress)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("Tried to start a selection but selection was already in progress."));
		bSelectionInProgress = false;
		UpdateAndPublishSelection();
	}
	if (!SelectionState.SelectingHexes.IsEmpty() || !SelectionState.RemovingHexes.IsEmpty())
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("SelectionState has pending temporary selection states."));
		UpdateAndPublishSelection();
	}
	
	// Turn off hover while selection is in progress
	bSelectionInProgress = true;
	ClearHover();

	// Get starting hex, draw it.
	FHxlbHexCoord HexCoord = GetHitGridHex(PressPos.WorldRay);
	if (!HexCoord.IsSet())
	{
		return;
	}
	
	if (bCtrlKeyIsPressed)
	{
		RemoveHex(HexCoord.Get());
	}
	else
	{
		if (!bShiftKeyIsPressed)
		{
			SelectionState.SelectedHexes.Empty();
		}
	
		SelectHex(HexCoord.Get());
	}
	
	PublishSelection();

	LastFocusedHex = HexCoord.Get();
}

void UHxlbSelectionToolBase::OnClickDrag(const FInputDeviceRay& DragPos)
{
	if (!bIsInitialized || !bSelectionInProgress)
	{
		return;
	}

	if (!ToolSettings)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbSelectionToolBase::OnClickDrag(): Tool settings are not initialized"));
	}
	
	bool bForceIgnoreGrid = true; // true because the user is allowed to drag the mouse outside the grid, and select stuff only inside the grid.
	FHxlbHexCoord HitGridResult = GetHitGridHex(DragPos.WorldRay, bForceIgnoreGrid);
	if (!HitGridResult.IsSet())
	{
		return;
	}
	FIntPoint HexCoords = HitGridResult.Get();
	bool bIsRemoving = !SelectionState.RemovingHexes.IsEmpty();

	if (HexCoords == LastFocusedHex)
	{
		// we only need to re-run this logic if the focused hex has changed.
		return;
	}

	switch (ToolSettings->SelectionMode)
	{
		case (EHxlbGridToolSelectionMode::Single):
			{
				if (bIsRemoving)
				{
					if (RemoveHex(HexCoords))
					{
						TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_SinglePublishRemoval);
						PublishSelection();
					}
				}
				else if (SelectHex(HexCoords))
				{
					TRACE_CPUPROFILER_EVENT_SCOPE(HXLB_SinglePublishSelection);
					PublishSelection();
				}
				
				break;
			}
		case (EHxlbGridToolSelectionMode::Hexagonal):
			{
				if (bIsRemoving)
				{
					int32 Radius = HexMath::AxialDistance(SelectionState.FirstSelectedHex, HexCoords);
					auto Iterator = FHxlbRadialIterator(SelectionState.FirstSelectedHex, Radius);
					UpdateRemovingHexes(Iterator);
				}
				else if (!SelectionState.SelectingHexes.IsEmpty())
				{
					int32 Radius = HexMath::AxialDistance(SelectionState.FirstSelectedHex, HexCoords);
					auto Iterator = FHxlbRadialIterator(SelectionState.FirstSelectedHex, Radius);
					UpdateSelectingHexes(Iterator);
				}
				
				PublishSelection();
				
				break;
			}
		case (EHxlbGridToolSelectionMode::Rectangular):
			{
				if (bIsRemoving)
				{
					if (LastFocusedHex == HexCoords)
					{
						// we only need to re-run this logic if the focused hex has changed.
						break;
					}

					auto Iterator = FHxlbRectangularIterator(SelectionState.FirstSelectedHex, HexCoords);
					UpdateRemovingHexes(Iterator);
				}
				else if (!SelectionState.SelectingHexes.IsEmpty())
				{
					auto Iterator = FHxlbRectangularIterator(SelectionState.FirstSelectedHex, HexCoords);
					UpdateSelectingHexes(Iterator);
				}
				
				PublishSelection();
				
				break;
			}
	default:
			HXLB_LOG(LogHxlbEditor, Error, TEXT("Unknown selection mode: %s"), *UEnum::GetValueAsString(ToolSettings->SelectionMode));
	}

	LastFocusedHex = HexCoords;
}

void UHxlbSelectionToolBase::OnClickRelease(const FInputDeviceRay& ReleasePos)
{
	bSelectionInProgress = false;
	UpdateAndPublishSelection();
}

void UHxlbSelectionToolBase::OnTerminateDragSequence()
{
	bSelectionInProgress = false;
}

bool UHxlbSelectionToolBase::RouteHexEditorMessage(FName Channel, FInstancedStruct& MessagePayload)
{
	bool bKnownMessageType = false;
	
	if (Channel == HxlbEditorMessageChannel::CommitBulkEdits)
	{
		bKnownMessageType = true;
		
		if (AHxlbHexManager* Manager = FindHexManager())
		{
			Manager->MapComponent->CommitBulkEdits();
			Manager->MapComponent->ClearBulkEditProxy();
			ClearSelection();
		}
	}
	// Don't forget to subscribe when you add new message routing here!

	return bKnownMessageType;
}

void UHxlbSelectionToolBase::OnTick(float DeltaTime)
{
	Super::OnTick(DeltaTime);
	
	ApplyPendingAction();
}

void UHxlbSelectionToolBase::RequestAction(EHxlbSelectionToolAction Action)
{
	if (PendingAction != EHxlbSelectionToolAction::NoAction)
	{
		HXLB_LOG(
			LogHxlbEditor,
			Warning,
			TEXT("UHxlbSelectionToolBase::RequestAction() requested action %s but action %s is pending"),
			*UEnum::GetValueAsString(Action),
			*UEnum::GetValueAsString(PendingAction)
		)
		return;
	}

	PendingAction = Action;
}

UPreviewGeometry* UHxlbSelectionToolBase::InitializeGridPreview()
{
	UPreviewGeometry* NewGridPreview = Super::InitializeGridPreview();

	if (NewGridPreview)
	{
		HoverLines = NewGridPreview->AddLineSet(HxlbSelectionToolLocals::HoverLineSetID);
		SelectionLines = NewGridPreview->AddLineSet(HxlbSelectionToolLocals::SelectionLineSetID);
	}

	return NewGridPreview;
}

void UHxlbSelectionToolBase::ShutdownGridPreview()
{
	HoverLines = nullptr;
	SelectionLines = nullptr;
	
	Super::ShutdownGridPreview();
}

void UHxlbSelectionToolBase::ConfigureSettingsProperties()
{
	ToolSettings = NewObject<UHxlbSelectionToolSettings>(this);
	ToolSettings->RestoreProperties(this);
	AddToolPropertySource(ToolSettings);
}

void UHxlbSelectionToolBase::ApplyPendingAction()
{
	bool bFlushPending = true;
	
	switch (PendingAction)
	{
	case EHxlbSelectionToolAction::NoAction:
		bFlushPending = false;
		break;
	// case  EHxlbSelectionToolAction::SelectAll:
	//	SelectAll();
	//	break;
	case EHxlbSelectionToolAction::CreateHexProxies:
		CreateProxiesFromSelection();
		break;
	case EHxlbSelectionToolAction::NextSelectionType:
		CycleSelectionType();
		break;
	case EHxlbSelectionToolAction::PrevSelectionType:
		CycleSelectionType(true);
		break;
	default:
		HXLB_LOG(LogHxlbEditor, Warning, TEXT("Unknown HxlbSelectionToolAction: %s"), *UEnum::GetValueAsString(PendingAction));
	}

	if (bFlushPending)
	{
		PendingAction = EHxlbSelectionToolAction::NoAction;
	}
}

bool UHxlbSelectionToolBase::SelectHex(const FIntPoint& NewSelection, bool bIsRefresh)
{
	if (!bSelectionInProgress)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("Tried to select a hex but selection is not in progress"));
		return false;
	}
	if (!ValidateHex(NewSelection))
	{
		return false;
	}

	bool bAlreadyInSet = false;
	if (SelectionState.SelectingHexes.IsEmpty() && !bIsRefresh)
	{
		SelectionState.FirstSelectedHex = NewSelection;
	}
	SelectionState.SelectingHexes.Add(NewSelection, &bAlreadyInSet);

	return !bAlreadyInSet;
}

void UHxlbSelectionToolBase::UpdateSelectingHexes(FHxlbHexIterator& Iterator)
{
	// We have to flush the selection set and re-add everything because it may have shrunk.
	SelectionState.SelectingHexes.Empty();
	while (Iterator.Next())
	{
		SelectHex(Iterator.Get(), true);
	}
}

void UHxlbSelectionToolBase::SelectAll()
{
	if (bSelectionInProgress)
	{
		return;
	}
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return;
	}

	UHxlbHexMapComponent* HexMap = HexManager->MapComponent;
	SelectionState = FHxlbSelectionState();
	bool NewHexAdded = false;

	if (HexMap->MapSettings.Shape != EHexMapShape::Unbounded)
	{
		auto* Iterator = HexMap->GetGridIterator(HexMap->GetGridOrigin());
		while (Iterator->Next())
		{
			NewHexAdded |= SelectHex(Iterator->Get());
		}
	}

	if (NewHexAdded)
	{
		PublishSelection();
	}
}

bool UHxlbSelectionToolBase::RemoveHex(const FIntPoint& NewSelection, bool bIsRefresh)
{
	if (!ValidateHex(NewSelection))
	{
		return false;
	}

	if (SelectionState.RemovingHexes.IsEmpty() && !bIsRefresh)
	{
		SelectionState.FirstSelectedHex = NewSelection;
	}

	bool bAlreadyInSet = false;
	SelectionState.RemovingHexes.Add(NewSelection, &bAlreadyInSet);
	return !bAlreadyInSet;
}

void UHxlbSelectionToolBase::UpdateRemovingHexes(FHxlbHexIterator& Iterator)
{
	// We have to flush the selection set and re-add everything because it may have shrunk.
	SelectionState.RemovingHexes.Empty();
	while (Iterator.Next())
	{
		RemoveHex(Iterator.Get(), true);
	}
}

void UHxlbSelectionToolBase::CreateProxiesFromSelection()
{
	// For now, no creating proxies while also selecting.
	if (bSelectionInProgress)
	{
		return;
	}
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbSelectionToolBase::CreateProxiesFromSelection(): HexManager is missing."));
		return;
	}

	for (FIntPoint AxialCoords : SelectionState.SelectedHexes)
	{
		HexManager->AddHex(AxialCoords);
	}
}

void UHxlbSelectionToolBase::CycleSelectionType(bool bReverse)
{
	if (bSelectionInProgress)
	{
		return;
	}

	int32 Increment = 1;
	if (bReverse)
	{
		Increment = -1;
	}

	int32 Current = static_cast<int32>(ToolSettings->SelectionMode);
	int32 Max = static_cast<int32>(EHxlbGridToolSelectionMode::MAX);

	int32 Incremented = (Current + Increment) % Max;
	ToolSettings->SelectionMode = static_cast<EHxlbGridToolSelectionMode>((Incremented + Max) % Max);
}


void UHxlbSelectionToolBase::SetupInputBehaviors()
{
	HoverBehavior = NewObject<UMouseHoverBehavior>();
	HoverBehavior->Modifiers.RegisterModifier(ShiftModifierID, FInputDeviceState::IsShiftKeyDown);
	HoverBehavior->Modifiers.RegisterModifier(CtrlModifierID, FInputDeviceState::IsCtrlKeyDown);
	HoverBehavior->Initialize(this);
	AddInputBehavior(HoverBehavior, this);

	ClickDragBehavior = NewObject<UClickDragInputBehavior>();
	ClickDragBehavior->Initialize(this);
	AddInputBehavior(ClickDragBehavior, this);

	HoverState.Clear();
	HitGridHexCoords.Clear();
}

bool UHxlbSelectionToolBase::UpdateHover()
{
	if (!bIsInitialized)
	{
		return false;
	}
	AHxlbHexManager* HexManager = FindHexManager(/*bCachedOnly=*/true);
	if (!HexManager)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbSelectionToolBase::UpdateHover(): HexManager is missing."));
		return false;
	}

	// The user is currently click+dragging a selection
	if (bSelectionInProgress)
	{
		return false;
	}
	if (!HitGridHexCoords.IsSet())
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbSelectionToolBase::UpdateHover(): HitGridHexCoords is invalid."));
		return false;
	}
	
	if (HoverState.HasCurrent() && HitGridHexCoords.Get() == HoverState.GetCurrent())
	{
		// Nothing has changed
		return true;
	}

	HoverState.SetCurrent(HitGridHexCoords.Get());

	if (HexManager->MapComponent->MapSettings.bEnableOverlay)
	{
		HexManager->MapComponent->HoverHex(HoverState, !bCtrlKeyIsPressed);
	}
#if HXLB_ENABLE_LEGACY_EDITOR_GRID
	else
	{
		FHxlbHighlightContext HighlightContext;
		HighlightContext.Type = EHxlbHighlightType::HoverSelection;
		ClearHighlight(HighlightContext);
		
		HighlightContext.ExtractContextInfoFromHexMap(HexManager->MapComponent, HoverState.GetCurrent());
		
		if (bCtrlKeyIsPressed)
		{
			HighlightContext.HighlightColor = FColor(100, 100, 100); // Dark grey
		}
		else
		{
			HighlightContext.HighlightColor = FColor::FromHex("#FFE662"); // Pale yellow
		}

		HighlightHex(HighlightContext);
	}
#endif
	
	return true;
}

void UHxlbSelectionToolBase::ClearHover()
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return;
	}

	HoverState.ClearCurrent();

	if (HexManager->MapComponent->MapSettings.bEnableOverlay)
	{
		HexManager->MapComponent->HoverHex(HoverState);
	}
#if HXLB_ENABLE_LEGACY_EDITOR_GRID
	else
	{
		FHxlbHighlightContext HighlightContext;
		HighlightContext.Type = EHxlbHighlightType::HoverSelection;
		ClearHighlight(HighlightContext);
	}
#endif

	HoverState.Clear();
}

void UHxlbSelectionToolBase::UpdateAndPublishSelection()
{
	bool bSelectionChanged = false;
	
	if (SelectionState.SelectingHexes.IsEmpty() && SelectionState.RemovingHexes.IsEmpty())
	{
		// If we aren't selecting or removing, that means the user clicked off in empty space, in which case we want to
		// just clear the selection state entirely.
		SelectionState = FHxlbSelectionState();
		bSelectionChanged = true;
	}
	else
	{
		if (!SelectionState.SelectingHexes.IsEmpty())
		{
			SelectionState.SelectedHexes.Append(SelectionState.SelectingHexes);
			SelectionState.SelectingHexes.Empty();
			bSelectionChanged = true;
		}
		
		for (FIntPoint HexCoord : SelectionState.RemovingHexes)
		{
			SelectionState.SelectedHexes.Remove(HexCoord);
			bSelectionChanged = true;
		}
		SelectionState.RemovingHexes.Empty();
	}

	PublishSelection();

	if (bSelectionChanged)
	{
		AHxlbHexManager* HexManager = FindHexManager();
		if (HexManager)
		{
			FHexLibEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FHexLibEditorModule>("HexLibEditor");
			TArray<UObject*> Hexes;

			for (FIntPoint HexCoord : SelectionState.SelectingHexes)
			{
				if (!HexManager->MapComponent->GetOrCreateHex(HexCoord))
				{
					HXLB_LOG(LogHxlbEditor, Error, TEXT("Got invalid hex data while pre-allocating hex data."));
				}
			}
			for (FIntPoint HexCoord : SelectionState.SelectedHexes)
			{
				UHxlbHex* HexData = HexManager->MapComponent->GetOrCreateHex(HexCoord);
				if (!HexData)
				{
					HXLB_LOG(LogHxlbEditor, Error, TEXT("Got invalid hex data while selecting hexes."));
					continue;
				}
				Hexes.Add(HexData);
			}
			
			UHxlbHex* BulkEditProxy = nullptr;
			if (Hexes.Num() > HxlbEditorConstants::BulkEditThreshold)
			{
				BulkEditProxy = HexManager->MapComponent->CreateBulkEditProxy();
			}
			EditorModule.SetDetailsObjects(Hexes, BulkEditProxy);
		}
	}
}

void UHxlbSelectionToolBase::ClearSelection()
{
	SelectionState = FHxlbSelectionState();
	PublishSelection();

	AHxlbHexManager* HexManager = FindHexManager();
	if (HexManager)
	{
		FHexLibEditorModule& EditorModule = FModuleManager::LoadModuleChecked<FHexLibEditorModule>("HexLibEditor");
		TArray<UObject*> EmptyHexes;
		EditorModule.SetDetailsObjects(EmptyHexes, nullptr);
	}
}

void UHxlbSelectionToolBase::PublishSelection()
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return;
	}

#if HXLB_ENABLE_LEGACY_EDITOR_GRID
	if (!HexManager->MapComponent->MapSettings.bEnableOverlay)
	{
		DrawSelection();
		return;
	}
#endif

	HexManager->MapComponent->UpdateSelection(SelectionState);
}

#if HXLB_ENABLE_LEGACY_EDITOR_GRID
void UHxlbSelectionToolBase::DrawSelection()
{
	if (!bIsInitialized)
	{
		return;
	}
	
	if (SelectionLines == nullptr)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbSelectionToolBase::DrawSelection(): SelectionLines is invalid."));
		return;
	}
	
	{
		FHxlbHighlightContext HighlightContext(EHxlbHighlightType::Selected);
		ClearHighlight(HighlightContext);
	}
	
	if (SelectionState.SelectedHexes.IsEmpty() && SelectionState.SelectingHexes.IsEmpty())
	{
		return;
	}

	AHxlbHexManager* HexManager = FindHexManager(/*bCachedOnly=*/true);
	if (!HexManager)
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbGridTool::DrawSelection(): HexManager is missing."));
		return;
	}
	
	TObjectPtr<UHxlbHexMapComponent> HexMap = HexManager->MapComponent.Get();

	const FColor FirstCellColor(255, 145, 19);
	const FColor CellColor(255, 255, 128);
	const FColor RemovingCellColor(100, 100, 100);
	
	FHxlbHexCoord FirstHex;
	
	for (FIntPoint HexCoords : SelectionState.SelectedHexes)
	{
		if (SelectionState.RemovingHexes.Contains(HexCoords))
		{
			continue;	
		}

		FHxlbHighlightContext HighlightContext(EHxlbHighlightType::Selected);
		HighlightContext.ExtractContextInfoFromHexMap(HexMap, HexCoords);
		HighlightContext.HighlightColor = CellColor;

		HighlightHex(HighlightContext);
	}
	for (FIntPoint HexCoords : SelectionState.SelectingHexes)
	{
		if (SelectionState.RemovingHexes.Contains(HexCoords))
		{
			continue;	
		}
		
		if (SelectionState.SelectedHexes.Contains(HexCoords))
		{
			// No point in drawing this twice.
			continue;
		}

		FHxlbHighlightContext HighlightContext(EHxlbHighlightType::Selecting);
		HighlightContext.ExtractContextInfoFromHexMap(HexMap, HexCoords);
		
		if (SelectionState.SelectingHexes.Num() > 1 && HexCoords == SelectionState.FirstSelectedHex && !SelectionState.RemovingHexes.Contains(HexCoords))
		{
			HighlightContext.HighlightColor = FirstCellColor;
			FirstHex.Set(HexCoords);
		}
		else
		{
			HighlightContext.HighlightColor = CellColor;
			HighlightHex(HighlightContext);
		}
	}

	if (FirstHex.IsSet())
	{
		FHxlbHighlightContext HighlightContext(EHxlbHighlightType::Selecting);
		HighlightContext.ExtractContextInfoFromHexMap(HexMap, FirstHex.Get());
		HighlightContext.HighlightColor = FirstCellColor;
		
		HighlightHex(HighlightContext);
	}

	for (FIntPoint HexCoords : SelectionState.RemovingHexes)
	{
		FHxlbHighlightContext HighlightContext(EHxlbHighlightType::Removing);
		HighlightContext.ExtractContextInfoFromHexMap(HexMap, HexCoords);
		HighlightContext.HighlightColor = RemovingCellColor;
		
		HighlightHex(HighlightContext);
	}
}

bool UHxlbSelectionToolBase::HighlightHex(FHxlbHighlightContext& Context)
{
	ULineSetComponent* LineSet = HighlightContextToLineSet(Context);
	if (LineSet == nullptr)
	{
		return false;
	}
	
	ALandscape* TargetLandscape = Context.GetLandscape();
	DrawHex(*LineSet, Context.GetWorldCoords(), Context.GetHexSize(), Context.HighlightColor, Context.GetZOffset(), TargetLandscape);
	
	return true;
}

bool UHxlbSelectionToolBase::ClearHighlight(FHxlbHighlightContext& Context)
{
	ULineSetComponent* ToClear = HighlightContextToLineSet(Context);
	if (ToClear == nullptr)
	{
		return false;
	}

	ToClear->Clear();
	return true;
}
#endif

#if HXLB_ENABLE_LEGACY_EDITOR_GRID
ULineSetComponent* UHxlbSelectionToolBase::HighlightContextToLineSet(FHxlbHighlightContext& Context)
{
	switch (Context.Type)
	{
	case EHxlbHighlightType::HoverRemoval:
	case EHxlbHighlightType::HoverSelection:
		return HoverLines;
	case EHxlbHighlightType::Selecting:
	case EHxlbHighlightType::Selected:
	case EHxlbHighlightType::Removing:
		return SelectionLines;
	default:
		HXLB_LOG(LogHxlbEditor, Error, TEXT("UHxlbSelectionToolBase::HighlightContextToLineSet(): Unknown HighlightType: %s"), *UEnum::GetValueAsString(Context.Type));
	}

	return nullptr;
}
#endif

#undef LOCTEXT_NAMESPACE