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
#include "InteractiveToolBuilder.h"
#include "Actor/HxlbHexManager.h"
#include "Foundation/HxlbHexMap.h"
#include "Subsystems/HxlbEditorMessageSubscriber.h"
#include "Tools/HxlbGridTool.h"

#include "HxlbMapSettingsTool.generated.h"

struct FHxlbEditorMessage_EnableOverlay;

UENUM()
enum class EHxlbMapSettingsToolAction: uint8
{
	NoAction,
	CreateHexManager
};

UCLASS()
class HEXLIBEDITOR_API UHxlbMapSettingsToolActions : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	void Initialize(UHxlbMapSettingsTool* NewParentTool);

	UFUNCTION(CallInEditor, Category = Actions, meta = (DisplayPriority = 1))
	void CreateHexManager() { RequestActionFromParent(EHxlbMapSettingsToolAction::CreateHexManager); }
	
protected:
	void RequestActionFromParent(EHxlbMapSettingsToolAction Action);
	
	UPROPERTY()
	TWeakObjectPtr<UHxlbMapSettingsTool> ParentTool;
};

UCLASS()
class HEXLIBEDITOR_API UHxlbMapSettingsToolSettings : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Tool Settings")
	TSubclassOf<AHxlbHexManager> ManagerClass = ADefaultHexManager::StaticClass();

	// This property is marked as "TransientToolProperty" because it is loaded and saved from the active HexManager in
	// the scene.
	UPROPERTY(EditAnywhere, Category = "Tool Settings", meta = (TransientToolProperty))
	FHxlbMapSettings MapSettings;
};

UCLASS()
class HEXLIBEDITOR_API UHxlbMapSettingsToolBuilder : public UInteractiveToolWithToolTargetsBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

UCLASS()
class HEXLIBEDITOR_API UHxlbMapSettingsTool : public UHxlbGridTool, public IHxlbEditorMessageSubscriberInterface
{
	GENERATED_BODY()

public:
	//~ Begin UInteractiveTool interface.
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void OnPropertyModified(UObject* PropertySet, FProperty* Property) override;
	virtual void RegisterActions(FInteractiveToolActionSet& ActionSet) override;
	virtual void OnTick(float DeltaTime) override;
	//~ End UInteractiveTool interface.

	//~ Begin HxlbEditorMessageSubscriberInterface
	virtual bool RouteHexEditorMessage(FName Channel, FInstancedStruct& MessagePayload) override;
	//~ End HxlbEditorMessageSubscriberInterface

	void RequestAction(EHxlbMapSettingsToolAction Action);

	UPROPERTY()
	TObjectPtr<UHxlbMapSettingsToolActions> ToolActions; 
	
	UPROPERTY()
	TObjectPtr<UHxlbMapSettingsToolSettings> ToolSettings;

	DECLARE_MULTICAST_DELEGATE(OnHexManagerCreatedDelegate);
	OnHexManagerCreatedDelegate OnHexManagerCreated;

protected:
	void HandleRefreshLandscapeRT();
	void HandleLandscapeOverlay();
	void HandleSpawnGridActors();
	void SetOverlayMaterialOnPPV();
	void RefreshOverlayMaterialParameters();
	
	bool PullMapSettings();
	bool PushMapSettings(FHxlbHexMapUpdateOptions UpdateOptions = FHxlbHexMapUpdateOptions());
	void ApplyPendingAction();
	void CreateHexManager();

	void SetupOverlay(AHxlbHexManager* HexManager);
	void TearDownOverlay(AHxlbHexManager* HexManager, APostProcessVolume* OverlayPPV);
	
	EHxlbMapSettingsToolAction PendingAction = EHxlbMapSettingsToolAction::NoAction;
};