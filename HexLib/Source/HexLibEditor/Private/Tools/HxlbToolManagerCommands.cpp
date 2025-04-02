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

#include "Tools/HxlbToolManagerCommands.h"

#include "Styles/HxlbStyle.h"
#include "Tools/HxlbLayoutTool.h"
#include "Tools/HxlbMapSettingsTool.h"
#include "Tools/HxlbSelectionTool.h"

#define LOCTEXT_NAMESPACE "HxlbToolManagerCommands"

// NOTE: When adding new tools, make sure you add something at every ***************** comment

FHxlbToolManagerCommands::FHxlbToolManagerCommands() :
	TCommands<FHxlbToolManagerCommands>(
		"HxlbToolManagerCommands",
		NSLOCTEXT("Contexts", "HxlbToolManagerCommands", "Hex Map Designer - Tools"),
		NAME_None,
		FHxlbStyle::Get()->GetStyleSetName()
	)
{
}

void FHxlbToolManagerCommands::RegisterCommands()
{
	UI_COMMAND(BeginHexMapSettingsTool, "Settings", "Hex Map Settings", EUserInterfaceActionType::RadioButton, FInputChord());
	UI_COMMAND(BeginHexLayoutTool, "Layout", "Layout Hexes", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(BeginHexSelectionTool, "Selection", "Select Hexes", EUserInterfaceActionType::ToggleButton, FInputChord());
	// *****************************************************************************************************************

	UI_COMMAND(BeginEmptyTool, "Empty", "Empty Tool", EUserInterfaceActionType::None, FInputChord());
}

FHxlbEditorToolsActionCommands::FHxlbEditorToolsActionCommands() :
		TInteractiveToolCommands<FHxlbEditorToolsActionCommands>(
		"HxlbEditorTools", // Context name for fast lookup
		NSLOCTEXT("Contexts", "HxlbEditorTools", "HexLib - Shared Shortcuts"), // Localized context name for displaying
		NAME_None, // Parent
		FHxlbStyle::GetStyleSetName() // Icon Style Set
	)
{
}

void FHxlbEditorToolsActionCommands::GetToolDefaultObjectList(TArray<UInteractiveTool*>& ToolCDOs)
{}

void FHxlbEditorToolsActionCommands::RegisterAllToolActions()
{
	FHxlbMapSettingsToolActionCommands::Register();
	FHxlbLayoutToolActionCommands::Register();
	FHxlbHexSelectionActionCommands::Register();
	// *****************************************************************************************************************
}

void FHxlbEditorToolsActionCommands::UnregisterAllToolActions()
{
	FHxlbMapSettingsToolActionCommands::Unregister();
	FHxlbLayoutToolActionCommands::Unregister();
	FHxlbHexSelectionActionCommands::Unregister();
	// *****************************************************************************************************************
}

void FHxlbEditorToolsActionCommands::UpdateToolCommandBinding(UInteractiveTool* Tool, TSharedPtr<FUICommandList> UICommandList, bool bUnbind)
{
#define UPDATE_BINDING(CommandsType)  if (!bUnbind) CommandsType::Get().BindCommandsForCurrentTool(UICommandList, Tool); else CommandsType::Get().UnbindActiveCommands(UICommandList);

	if (ExactCast<UHxlbMapSettingsTool>(Tool) != nullptr)
	{
		UPDATE_BINDING(FHxlbMapSettingsToolActionCommands);
	}
	else if (ExactCast<UHxlbLayoutTool>(Tool) != nullptr)
	{
		UPDATE_BINDING(FHxlbLayoutToolActionCommands);
	}
	else if (ExactCast<UHxlbSelectionTool>(Tool) != nullptr)
	{
		UPDATE_BINDING(FHxlbHexSelectionActionCommands);
	}
	// *****************************************************************************************************************
}

#define DEFINE_TOOL_ACTION_COMMANDS(CommandsClassName, ContextNameString, SettingsDialogString, ToolClassName ) \
CommandsClassName::CommandsClassName() : TInteractiveToolCommands<CommandsClassName>( \
ContextNameString, NSLOCTEXT("Contexts", ContextNameString, SettingsDialogString), NAME_None, FAppStyle::GetAppStyleSetName()) {} \
void CommandsClassName::GetToolDefaultObjectList(TArray<UInteractiveTool*>& ToolCDOs) \
{\
ToolCDOs.Add(GetMutableDefault<ToolClassName>()); \
}

DEFINE_TOOL_ACTION_COMMANDS(FHxlbMapSettingsToolActionCommands, "HxlbMapSettingsTool", "HexEngine - Map Settings Tool", UHxlbMapSettingsTool);
DEFINE_TOOL_ACTION_COMMANDS(FHxlbLayoutToolActionCommands, "HxlbLayoutTool", "HexEngine - Layout Tool", UHxlbLayoutTool);
DEFINE_TOOL_ACTION_COMMANDS(FHxlbHexSelectionActionCommands, "HxlbSelectionTool", "HexEngine - Selection Tool", UHxlbSelectionTool);
// *****************************************************************************************************************


#undef LOCTEXT_NAMESPACE