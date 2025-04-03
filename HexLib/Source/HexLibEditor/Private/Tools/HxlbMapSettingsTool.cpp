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

#include "Tools/HxlbMapSettingsTool.h"

#include "HexLibEditorLoggingDefs.h"
#include "Actor/HxlbHexManager.h"
#include "EngineUtils.h"
#include "HxlbConstants.h"
#include "HxlbDefines.h"
#include "InputCoreTypes.h"
#include "InteractiveToolManager.h"
#include "Landscape.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ToolTargetManager.h"
#include "Editor/MaterialEditor/Public/MaterialEditingLibrary.h"
#include "FunctionLibraries/HxlbEditorUtils.h"
#include "Macros/HexLibLoggingMacros.h"
#include "Subsystems/HxlbEditorMessageChannels.h"
#include "Subsystems/HxlbEditorMessagingSubsystem.h"

#define LOCTEXT_NAMESPACE "UHxlbMapSettingsTool"

void UHxlbMapSettingsToolActions::Initialize(UHxlbMapSettingsTool* NewParentTool)
{
	ParentTool = NewParentTool;
}

void UHxlbMapSettingsToolActions::RequestActionFromParent(EHxlbMapSettingsToolAction Action)
{
	if (ParentTool.IsValid())
	{
		ParentTool->RequestAction(Action);
	}
}

bool UHxlbMapSettingsToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return SceneState.TargetManager->CountSelectedAndTargetable(SceneState, GetTargetRequirements()) <= 1;
}

UInteractiveTool* UHxlbMapSettingsToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	auto* Tool = NewObject<UHxlbMapSettingsTool>(SceneState.ToolManager);
	TObjectPtr<UToolTarget> TargetOrNull = SceneState.TargetManager->BuildFirstSelectedTargetable(SceneState, GetTargetRequirements());
	Tool->SetTarget(TargetOrNull);
	Tool->SetTargetWorld(SceneState.World);

	return Tool;
}

void UHxlbMapSettingsTool::Setup()
{
	Super::Setup();

	ToolActions = NewObject<UHxlbMapSettingsToolActions>(this);
	ToolActions->Initialize(this);
	AddToolPropertySource(ToolActions);
	
	ToolSettings = NewObject<UHxlbMapSettingsToolSettings>(this);

	// Note, the use case for using RestoreProperties(this, CacheIdentifier) is as follows: you have two classes that
	// are derived from the same base class, and they both have a settings object with the same name
	// (such as ToolSettings). In this case you need to supply a CacheIdentifier in order to avoid naming conflicts.
	ToolSettings->RestoreProperties(this);
	if (!PullMapSettings())
	{
		bIsInitialized = false;
	}
	
	AddToolPropertySource(ToolSettings);
	SetToolDisplayName(LOCTEXT("ToolName", "Map Settings"));

	if (auto* MessageSystem = UHxlbEditorMessagingSubsystem::Get())
	{
		// Parent is already subscribed to HxlbEditorMessageChannel::EnableOverlay
		MessageSystem->Subscribe(HxlbEditorMessageChannel::RefreshLandscapeRT, this);
	}
}

void UHxlbMapSettingsTool::Shutdown(EToolShutdownType ShutdownType)
{
	ToolSettings->SaveProperties(this);
	PushMapSettings();

	if (auto* MessageSystem = UHxlbEditorMessagingSubsystem::Get())
	{
		MessageSystem->Unsubscribe(HxlbEditorMessageChannel::RefreshLandscapeRT, this);
	}
	
	Super::Shutdown(ShutdownType);
}

void UHxlbMapSettingsTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	Super::Render(RenderAPI);
}

void UHxlbMapSettingsTool::OnPropertyModified(UObject* PropertySet, FProperty* Property)
{
	//	When dragging a value slider, this actually gets called once for each new value, plus again once the user
	//  lets go of the LMB. It might be worth checking if the property actually changed value and skipping this if it
	//  did not. Alternatively, maybe there's a way to stop this from firing on the LMB release. 
	
	FHxlbHexMapUpdateOptions UpdateOptions;
	TSet<FName> TagSettingsNames;
	TagSettingsNames.Add(GET_MEMBER_NAME_CHECKED(FHxlbMapSettings, DebugTag));
	TagSettingsNames.Add(GET_MEMBER_NAME_CHECKED(FHxlbMapSettings, TagSettings));

	bool bNeedsMaterialRefresh = false;
	
	if (TagSettingsNames.Contains(Property->GetFName()) || (Property->Owner.GetFName() == FHxlbHexTagInfo_Editor::StaticStruct()->GetFName()))
	{
		UpdateOptions.bRefreshTagSettings = true;
	}
	if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FHxlbMapSettings, bEnableOverlay))
	{
		HandleEnableOverlay();
		
		UpdateOptions.bForceLandscapeRTRefresh = true;
		UpdateOptions.bRefreshGridlines = true;
		bNeedsMaterialRefresh = true;
	}

	TSet<FName> ShapeSettingsNames;
	ShapeSettingsNames.Add(FHexagonalHexMapSettings::StaticStruct()->GetFName());
	ShapeSettingsNames.Add(FRectangularHexMapSettings::StaticStruct()->GetFName());
	
	if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FHxlbMapSettings, Shape) || ShapeSettingsNames.Contains(Property->Owner.GetFName()))
	{
		UpdateOptions.bRefreshGridlines = true;
		bNeedsMaterialRefresh = true;
	}
	
	PushMapSettings(UpdateOptions);

	TSet<FName> MaterialUpdateNames;
	MaterialUpdateNames.Add(GET_MEMBER_NAME_CHECKED(FHexMapOverlaySettings, OverlayMaterial));
	MaterialUpdateNames.Add(GET_MEMBER_NAME_CHECKED(FHxlbMapSettings, HexSize));

	if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FHexMapOverlaySettings, OverlayMaterial))
	{
		SetOverlayMaterialOnPPV();
	}
	if (bNeedsMaterialRefresh || MaterialUpdateNames.Contains(Property->GetFName()))
	{
		RefreshOverlayMaterialParameters();
	}
}

void UHxlbMapSettingsTool::RegisterActions(FInteractiveToolActionSet& ActionSet)
{
	Super::RegisterActions(ActionSet);

	int32 ActionID = (int32)EStandardToolActions::BaseClientDefinedActionID + 1;
	ActionSet.RegisterAction(this, ActionID++,
		TEXT("CreateHexManager"),
		LOCTEXT("CreateHexManagerAction", "Create Hex Manager"),
		LOCTEXT("CreateHexManagerTooltip", ""),
		EModifierKey::Shift,
		EKeys::N,
		[this](){ RequestAction(EHxlbMapSettingsToolAction::CreateHexManager); });
}

bool UHxlbMapSettingsTool::RouteHexEditorMessage(FName Channel, FInstancedStruct& MessagePayload)
{
	bool bKnownMessageType = false;
	
	if (Channel == HxlbEditorMessageChannel::RefreshLandscapeRT)
	{
		bKnownMessageType = true;
		HandleRefreshLandscapeRT();
	}

	// Don't forget to subscribe when you add new message routing here!

	return bKnownMessageType;
}

void UHxlbMapSettingsTool::OnTick(float DeltaTime)
{
	Super::OnTick(DeltaTime);
	
	ApplyPendingAction();
}

void UHxlbMapSettingsTool::RequestAction(EHxlbMapSettingsToolAction Action)
{
	if (PendingAction != EHxlbMapSettingsToolAction::NoAction)
	{
		HXLB_LOG(
			LogHxlbEditor,
			Warning,
			TEXT("UHxlbMapSettingsTool::RequestAction() requested action %s but action %s is pending"),
			*UEnum::GetValueAsString(Action),
			*UEnum::GetValueAsString(PendingAction)
		)
		return;
	}

	PendingAction = Action;
}

void UHxlbMapSettingsTool::HandleRefreshLandscapeRT()
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return;
	}

	HexManager->MapComponent->RefreshLandscapeData();
}

void UHxlbMapSettingsTool::HandleEnableOverlay()
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return;
	}
	
	bool bWantsOverlayEnabled = ToolSettings->MapSettings.bEnableOverlay;
	
	APostProcessVolume* OverlayPPV = HexManager->GetOverlayPPV();
	if (bWantsOverlayEnabled && !OverlayPPV)
	{
#if HXLB_ENABLE_LEGACY_EDITOR_GRID
		ClearGridPreview();
#endif
		SetupOverlay(HexManager);
	}
	else if (!bWantsOverlayEnabled && OverlayPPV)
	{
		TearDownOverlay(HexManager, OverlayPPV);
#if HXLB_ENABLE_LEGACY_EDITOR_GRID
		bIsDirty = true;
#endif
	}
}

void UHxlbMapSettingsTool::SetOverlayMaterialOnPPV()
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return;
	}
		
	APostProcessVolume* OverlayPPV = HexManager->GetOverlayPPV();
	UMaterialInterface* OverlayMaterial = HexManager->MapComponent->MapSettings.OverlaySettings.OverlayMaterial;
	if (OverlayPPV && (
		OverlayPPV->Settings.WeightedBlendables.Array.Num() == 0 ||
		OverlayPPV->Settings.WeightedBlendables.Array[0].Object != OverlayMaterial
		)
	)
	{
		OverlayPPV->Settings.WeightedBlendables.Array.Empty();
		OverlayPPV->Settings.WeightedBlendables.Array.Emplace(1.0f, OverlayMaterial);
	}
}

void UHxlbMapSettingsTool::RefreshOverlayMaterialParameters()
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return;
	}

	FHxlbMapSettings& MapSettings = HexManager->MapComponent->MapSettings;
	UMaterialInterface* OverlayMaterial = MapSettings.OverlaySettings.OverlayMaterial;
	
	auto* OverlayMaterialInstance = Cast<UMaterialInstanceConstant>(OverlayMaterial);
	if (!OverlayMaterialInstance)
	{
		return;
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
	
		LandscapeResolution += 1;

		FVector LandscapeActorScale = TargetLandscape->GetActorScale();
		double LandscapeLengthCm = LandscapeActorScale.X * LandscapeResolution;
		double LandscapeHeightCm = LandscapeActorScale.Z * 512;
		double HeightOffsetCm = (LandscapeHeightCm / 2) - TargetLandscape->GetActorLocation().Z;

		// HXLB_LOG(LogHxlbRuntime, Warning, TEXT("Updating landscape material params."));
		// HXLB_LOG(LogHxlbRuntime, Warning, TEXT("Landscape resolution is: %d"), LandscapeResolution);
		// HXLB_LOG(LogHxlbRuntime, Warning, TEXT("Landscape length in meters is: %lf"), LandscapeLengthCm / 100.0);
		// HXLB_LOG(LogHxlbRuntime, Warning, TEXT("Landscape height in meters is: %lf"), LandscapeHeightCm / 100.0);
		// HXLB_LOG(LogHxlbRuntime, Warning, TEXT("Landscape height offset in meters is: %lf"), HeightOffsetCm / 100.0);
	
		OverlayMaterialInstance->SetScalarParameterValueEditorOnly(HxlbConstants::HexOverlayParam_LandscapeLength, LandscapeLengthCm);
		OverlayMaterialInstance->SetScalarParameterValueEditorOnly(HxlbConstants::HexOverlayParam_LandscapeHeight, LandscapeHeightCm);
		OverlayMaterialInstance->SetScalarParameterValueEditorOnly(HxlbConstants::HexOverlayParam_HeightOffset, HeightOffsetCm);
	}

	OverlayMaterialInstance->SetStaticSwitchParameterValueEditorOnly(HxlbConstants::HexOverlayParam_UseSimpleGridAlgo, MapSettings.Shape == EHexMapShape::Unbounded);
	OverlayMaterialInstance->SetScalarParameterValueEditorOnly(HxlbConstants::HexOverlayParam_HexSize, HexManager->MapComponent->MapSettings.HexSize);
	
	UMaterialEditingLibrary::UpdateMaterialInstance(OverlayMaterialInstance);
	UMaterialEditingLibrary::RebuildMaterialInstanceEditors(OverlayMaterialInstance->GetMaterial());
	OverlayMaterialInstance->MarkPackageDirty();
}

bool UHxlbMapSettingsTool::PullMapSettings()
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return false;
	}
	
	ToolSettings->MapSettings = HexManager->MapComponent->MapSettings;
	bIsDirty = true;

	return true;
}


bool UHxlbMapSettingsTool::PushMapSettings(FHxlbHexMapUpdateOptions UpdateOptions)
{
	AHxlbHexManager* HexManager = FindHexManager();
	if (!HexManager)
	{
		return false;
	}
	
	// copy map settings to the HexManager.
	HexManager->MapComponent->Update(ToolSettings->MapSettings, UpdateOptions);
	bIsDirty = true;

	return true;
}

void UHxlbMapSettingsTool::ApplyPendingAction()
{
	switch(PendingAction)
	{
	case EHxlbMapSettingsToolAction::NoAction:
		return;
	case EHxlbMapSettingsToolAction::CreateHexManager:
		CreateHexManager();
		PendingAction = EHxlbMapSettingsToolAction::NoAction;
		break;
	default:
		HXLB_LOG(LogHxlbEditor, Warning, TEXT("Unknown HxlbSettingsToolAction: %s"), *UEnum::GetValueAsString(PendingAction));
		PendingAction = EHxlbMapSettingsToolAction::NoAction;
	}
}

void UHxlbMapSettingsTool::CreateHexManager()
{
	if (CachedManager.Get())
	{
		return;
	}

	if (TargetWorld)
	{
		auto* HexManager = TargetWorld->SpawnActor<AHxlbHexManager>(ToolSettings->ManagerClass, FVector::Zero(), FRotator::ZeroRotator);
		CachedManager = HexManager;
		PullMapSettings();
		
#if WITH_EDITOR
		// NOTE: Without this, the UE editor does not seem to realize that the level has been updated.
		HexManager->SetActorLabel("HexManager");
#endif

		// Notify our parent toolkit so that it can force-refresh its views.
		OnHexManagerCreated.Broadcast();
		
		FinishSetup();
	}
}

void UHxlbMapSettingsTool::SetupOverlay(AHxlbHexManager* HexManager)
{
	auto* NewPPV = TargetWorld->SpawnActor<APostProcessVolume>();
	if (!NewPPV)
	{
		return;
	}

	for (TActorIterator<ALandscape> Iterator(TargetWorld); Iterator; ++Iterator)
	{
		ToolSettings->MapSettings.OverlaySettings.TargetLandscape = *Iterator;
		FHxlbHexMapUpdateOptions UpdateOptions;
		UpdateOptions.bForceLandscapeRTRefresh = true;
		PushMapSettings(UpdateOptions);
		RefreshOverlayMaterialParameters();
		break;
	}
	
	NewPPV->bUnbound = true;
	NewPPV->bEnabled = true;
	NewPPV->Priority = -1.0;

#if WITH_EDITOR
	NewPPV->SetActorLabel("HexOverlayVolume");
#endif
	
	UMaterialInterface* OverlayMaterial = HexManager->MapComponent->MapSettings.OverlaySettings.OverlayMaterial;
	if (OverlayMaterial)
	{
		NewPPV->Settings.WeightedBlendables.Array.Emplace(1.0f, OverlayMaterial);
	}
	HexManager->SetOverlayPPV(NewPPV);
}

void UHxlbMapSettingsTool::TearDownOverlay(AHxlbHexManager* HexManager, APostProcessVolume* OverlayPPV)
{
	TargetWorld->DestroyActor(OverlayPPV);

	HexManager->SetOverlayPPV(nullptr);
}

#undef LOCTEXT_NAMESPACE