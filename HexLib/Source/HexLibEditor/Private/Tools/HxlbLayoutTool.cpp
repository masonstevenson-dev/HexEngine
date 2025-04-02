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

#include "Tools/HxlbLayoutTool.h"

#include "Actor/HxlbHexManager.h"
#include "EngineUtils.h"
#include "InteractiveToolManager.h"
#include "ToolTargetManager.h"
#include "Foundation/HxlbHexMap.h"

#define LOCTEXT_NAMESPACE "UHxlbLayoutTool"

bool UHxlbLayoutToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	bool MeetsTargetRequirements = SceneState.TargetManager->CountSelectedAndTargetable(SceneState, GetTargetRequirements()) <= 1;
	AHxlbHexManager* ManagerActor = nullptr;

	if (!MeetsTargetRequirements)
	{
		return false;
	}
	
	if (SceneState.World || SceneState.World->WorldType == EWorldType::Editor)
	{
		for (TActorIterator<AHxlbHexManager> ActorItr(SceneState.World); ActorItr; ++ActorItr)
		{
			ManagerActor = *ActorItr;
			if (!ManagerActor || !ManagerActor->MapComponent)
			{
				continue;
			}
			
			break;
		}
	}

	if (ManagerActor && ManagerActor->MapComponent->MapSettings.Shape == EHexMapShape::Manual)
	{
		return true;
	}

	return false;
}

UInteractiveTool* UHxlbLayoutToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	auto* Tool = NewObject<UHxlbLayoutTool>(SceneState.ToolManager);
	TObjectPtr<UToolTarget> TargetOrNull = SceneState.TargetManager->BuildFirstSelectedTargetable(SceneState, GetTargetRequirements());
	Tool->SetTarget(TargetOrNull);
	Tool->SetTargetWorld(SceneState.World);

	return Tool;
}

void UHxlbLayoutTool::Setup()
{
	Super::Setup();
}

void UHxlbLayoutTool::Shutdown(EToolShutdownType ShutdownType)
{
	Super::Shutdown(ShutdownType);
}

void UHxlbLayoutTool::RegisterActions(FInteractiveToolActionSet& ActionSet)
{
	Super::RegisterActions(ActionSet);

	ActionSet.RegisterAction(this, NextActionID++,
		TEXT("CreateProxies"),
		LOCTEXT("CreateProxiesAction", "Add"),
		LOCTEXT("CreateProxiesTooltip", "Adds the selected hexes to the hex management system."),
		EModifierKey::None,
		EKeys::C,
		[this](){ RequestAction(EHxlbSelectionToolAction::CreateHexProxies); });
}

void UHxlbLayoutTool::ConfigureActionProperties()
{
	Super::ConfigureActionProperties();
	
	ToolActions = NewObject<UHxlbLayoutToolActions>(this);
	ToolActions->Initialize(this);
	AddToolPropertySource(ToolActions);
}

#undef LOCTEXT_NAMESPACE