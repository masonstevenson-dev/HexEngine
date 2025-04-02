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

#include "Editor/HxlbEditorMode.h"

#include "Actor/HxlbHexManager.h"
#include "EditorModeManager.h"
#include "EngineUtils.h"
#include "HexLibEditorLoggingDefs.h"
#include "ToolContextInterfaces.h"
#include "Editor/HxlbToolkit.h"
#include "Styles/HxlbStyle.h"
#include "Toolkits/ToolkitManager.h"
#include "Tools/HxlbEmptyTool.h"
#include "Tools/HxlbGridTool.h"
#include "Tools/HxlbLayoutTool.h"
#include "Tools/HxlbMapSettingsTool.h"
#include "Tools/HxlbSelectionTool.h"
#include "Tools/HxlbToolManagerCommands.h"

#define LOCTEXT_NAMESPACE "UHxlbEditorMode"

const FEditorModeID UHxlbEditorMode::EM_HxlbEditorModeId = FEditorModeID(TEXT("EM_HexLib"));

UHxlbEditorMode::UHxlbEditorMode()
{
	Info = FEditorModeInfo(
		EM_HxlbEditorModeId,
		LOCTEXT("HxMDEditorModeName", "Hex Map"),
		FSlateIcon(FHxlbStyle::GetStyleSetName(), "HexLib.Icons.HexSelection"),
		true
	);
}

UHxlbEditorMode::~UHxlbEditorMode()
{
	// nothing for now...
}

void UHxlbEditorMode::Enter()
{
	UEdMode::Enter();

	const auto& ToolManagerCommands = FHxlbToolManagerCommands::Get();

	auto HexMapSettingsToolBuilder = NewObject<UHxlbMapSettingsToolBuilder>();
	RegisterTool(ToolManagerCommands.BeginHexMapSettingsTool, TEXT("HexMapSettingsTool"), HexMapSettingsToolBuilder);
	
	auto HexLayoutToolBuilder = NewObject<UHxlbLayoutToolBuilder>();
	RegisterTool(ToolManagerCommands.BeginHexLayoutTool, TEXT("HexLayoutTool"), HexLayoutToolBuilder);

	auto HexSelectionToolBuilder = NewObject<UHxlbSelectionToolBuilder>();
	RegisterTool(ToolManagerCommands.BeginHexSelectionTool, TEXT("HexSelectionTool"), HexSelectionToolBuilder);

	auto EmptyToolBuilder = NewObject<UHxlbEmptyToolBuilder>();
	RegisterTool(ToolManagerCommands.BeginEmptyTool, TEXT("EmptyTool"), EmptyToolBuilder);

	// UEdmode actually has a ActivateDefaultTool() fn that we *could* call here, but this is a bit of a risk since
	// in the future it is possible that the base UEdMode could call this fn (currently nothing calls it). This would
	// result in this fn being called twice.
	//
	// If ActivateDefaultTool() is ever configured to be called automatically by the base class, remove this and override
	// that fn.
	ActivateDefaultToolFromPalette();
}

void UHxlbEditorMode::Exit()
{
	FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	Toolkit.Reset();
	
	UEdMode::Exit();
}

AHxlbHexManager* UHxlbEditorMode::FindHexManager()
{
	if (CachedManager.IsValid() && CachedManager->MapComponent != nullptr)
	{
		return CachedManager.Get();
	}

	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld || CurrentWorld->WorldType != EWorldType::Editor)
	{
		return nullptr;
	}

	// Assigns the first valid HexManager we find.
	for (TActorIterator<AHxlbHexManager> ActorItr(CurrentWorld); ActorItr; ++ActorItr)
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

void UHxlbEditorMode::CreateToolkit()
{
	if (Toolkit.IsValid())
	{
		UE_LOG(LogHxlbEditor, Error, TEXT("Got request to create Toolkit, but Toolkit is already valid."));
		return;
	}

	Toolkit = MakeShareable(new FHxlbToolkit);
}

void UHxlbEditorMode::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	// As documented by Epic:
	// "Disable slate throttling so that Tool background computes responding to sliders can properly be processed
	// on Tool Tick. Otherwise, when a Tool kicks off a background update in a background thread, the computed
	// result will be ignored until the user moves the slider, ie you cannot hold down the mouse and wait to see
	// the result. This apparently broken behavior is currently by-design."
	FSlateThrottleManager::Get().DisableThrottle(true);

	// I *think* this just binds any SHARED tool commands that should be used across all HxLb tools.
	FHxlbEditorToolsActionCommands::UpdateToolCommandBinding(Tool, Toolkit->GetToolkitCommands(), false);
}

void UHxlbEditorMode::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	// re-enable slate throttling (see OnToolStarted)
	FSlateThrottleManager::Get().DisableThrottle(false);

	// I *think* this just unbinds any SHARED tool commands that should be used across all HxLb tools.
	FHxlbEditorToolsActionCommands::UpdateToolCommandBinding(Tool, Toolkit->GetToolkitCommands(), true);
}

void UHxlbEditorMode::ActivateDefaultToolFromPalette()
{
	if (!Toolkit.IsValid())
	{
		UE_LOG(LogHxlbEditor, Error, TEXT("UHxlbEditorMode::ActivateDefaultTool(): Missing Toolkit."));
		return;
	}
	
	TSharedPtr<FHxlbToolkit> HxlbToolkit = StaticCastSharedPtr<FHxlbToolkit>(Toolkit);
	if (!HxlbToolkit.IsValid())
	{
		UE_LOG(LogHxlbEditor, Error, TEXT("UHxlbEditorMode::ActivateDefaultTool(): Expected a HxlbToolkit."));
		return;
	}

	HxlbToolkit->ActivateDefaultToolForPalette(HxlbToolkit->GetCurrentPalette());
}

void UHxlbEditorMode::OnToolPostBuild(
	UInteractiveToolManager* ToolManager,
	EToolSide ToolSide,
	UInteractiveTool* BuiltTool,
	const FToolBuilderState ToolBuilderState
)
{
	// Nothing for now...
}


#undef LOCTEXT_NAMESPACE
