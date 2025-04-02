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

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "BaseBehaviors/ClickDragBehavior.h"
#include "HxlbDefines.h"
#include "HxlbGridTool.h"
#include "InteractiveTool.h"
#include "Foundation/HxlbTypes.h"
#include "Subsystems/HxlbEditorMessageSubscriber.h"

#include "HxlbSelectionToolBase.generated.h"

struct FHxlbHexIterator;
class UHxlbHexMapComponent;
class ALandscape;
class UHxlbHex;

UENUM(BlueprintType)
enum class EHxlbGridToolSelectionMode: uint8
{
	Single,
	Rectangular,
	Hexagonal,
	MAX UMETA(Hidden),
};

UENUM()
enum class EHxlbSelectionToolAction: uint8
{
	NoAction,
	SelectAll,
	CreateHexProxies,
	NextSelectionType,
	PrevSelectionType
};

USTRUCT()
struct FHxlbHighlightContext
{
	GENERATED_BODY()

public:
	FHxlbHighlightContext() = default;
	FHxlbHighlightContext(EHxlbHighlightType NewType) : Type(NewType) {}
	
	void ExtractContextInfoFromHexMap(UHxlbHexMapComponent* HexMap, FIntPoint HexCoords);
	UHxlbHex* GetHexData() { return HexData.Get(); }

	EHxlbHighlightType Type = EHxlbHighlightType::None;
	FColor HighlightColor = FColor::Black;
	
protected:
	// Null if no additional hex data has been associated with this hex.
	TWeakObjectPtr<UHxlbHex> HexData = nullptr;


#if HXLB_ENABLE_LEGACY_EDITOR_GRID
public:
	float GetZOffset() const;
	FVector GetWorldCoords() { return WorldCoords; }
	double GetHexSize() { return HexSize; }
	ALandscape* GetLandscape() { return Landscape.Get(); }

protected:
	FVector WorldCoords = FVector::ZeroVector;
	double HexSize = 100.0;
	
	// Null if overlay mode is disabled.
	TWeakObjectPtr<ALandscape> Landscape = nullptr;
#endif
	
};

UCLASS()
class HEXLIBEDITOR_API UHxlbSelectionToolActionsBase : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	virtual void Initialize(UHxlbSelectionToolBase* NewParentTool);

protected:
	virtual void RequestActionFromParent(EHxlbSelectionToolAction Action);
	
	UPROPERTY()
	TWeakObjectPtr<UHxlbSelectionToolBase> ParentTool;
};

UCLASS()
class HEXLIBEDITOR_API UHxlbSelectionToolSettings : public UInteractiveToolPropertySet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = Options, meta = (TransientToolProperty))
	EHxlbGridToolSelectionMode SelectionMode = EHxlbGridToolSelectionMode::Single;
};

UCLASS()
class HEXLIBEDITOR_API UHxlbSelectionToolBuilderBase : public UInteractiveToolWithToolTargetsBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
};

UCLASS()
class UHxlbSelectionToolBase : public UHxlbGridTool,
	public IHoverBehaviorTarget,
	public IClickDragBehaviorTarget,
	public IHxlbEditorMessageSubscriberInterface
{
	GENERATED_BODY()

public:
	//~ Begin UInteractiveTool interface.
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void OnTick(float DeltaTime) override;
	//~ End UInteractiveTool interface.
	
	//~ Begin IHoverBehaviorTarget interface.
	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;
	virtual void OnUpdateModifierState(int ModifierID, bool bIsOn) override;
	//~ End IHoverBehaviorTarget interface.

	//~ Begin IClickDragBehaviorTarget
	virtual FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos) override;
	virtual void OnClickPress(const FInputDeviceRay& PressPos) override;
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override;
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override;
	virtual void OnTerminateDragSequence() override;
	//~ End IClickDragBehaviorTarget

	//~ Begin HxlbEditorMessageSubscriberInterface
	virtual bool RouteHexEditorMessage(FName Channel, FInstancedStruct& MessagePayload) override;
	//~ End HxlbEditorMessageSubscriberInterface
	
	void RequestAction(EHxlbSelectionToolAction Action);
	
protected:
	virtual UPreviewGeometry* InitializeGridPreview() override;
	virtual void ShutdownGridPreview() override;

	virtual void ConfigureActionProperties() {}
	virtual void ConfigureSettingsProperties();
	void ApplyPendingAction();
	
	bool SelectHex(const FIntPoint& NewSelection, bool bIsRefresh = false);
	void UpdateSelectingHexes(FHxlbHexIterator& Iterator);
	void SelectAll();
	
	bool RemoveHex(const FIntPoint& NewSelection, bool bIsRefresh = false);
	void UpdateRemovingHexes(FHxlbHexIterator& Iterator);

	virtual void CreateProxiesFromSelection();
	virtual void CycleSelectionType(bool bReverse = false);
	
	void SetupInputBehaviors();
	bool UpdateHover();
	void ClearHover();

	void UpdateAndPublishSelection();
	void ClearSelection();
	void PublishSelection();
	
#if HXLB_ENABLE_LEGACY_EDITOR_GRID
	void DrawSelection();
	virtual bool HighlightHex(FHxlbHighlightContext& Context);
	virtual bool ClearHighlight(FHxlbHighlightContext& Context);
#endif

#if HXLB_ENABLE_LEGACY_EDITOR_GRID
	ULineSetComponent* HighlightContextToLineSet(FHxlbHighlightContext& Context);
#endif
	
	UPROPERTY()
	TObjectPtr<UHxlbSelectionToolSettings> ToolSettings;

	UPROPERTY()
	TObjectPtr<UMouseHoverBehavior> HoverBehavior = nullptr;

	UPROPERTY()
	TObjectPtr<UClickDragInputBehavior> ClickDragBehavior = nullptr;

	UPROPERTY()
	TObjectPtr<ULineSetComponent> HoverLines;

	UPROPERTY()
	TObjectPtr<ULineSetComponent> SelectionLines;

	static const int32 ShiftModifierID = 1;
	bool bShiftKeyIsPressed = false;
	static const int32 CtrlModifierID = 2;
	bool bCtrlKeyIsPressed = false;

	FHxlbHexCoord HitGridHexCoords;
	bool bSelectionInProgress = false;
	bool bNewSelectionStarted = false;
	FHxlbSelectionState SelectionState;
	FHxlbHexCoordDelta HoverState;
	FIntPoint LastFocusedHex;

	int32 NextActionID;
	EHxlbSelectionToolAction PendingAction = EHxlbSelectionToolAction::NoAction;
};
