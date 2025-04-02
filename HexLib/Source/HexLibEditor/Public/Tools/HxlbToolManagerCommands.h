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

#include "Framework/Commands/Commands.h"
#include "Tools/InteractiveToolsCommands.h"

// NOTE: When adding new tools, make sure you add something at every ***************** comment

class HEXLIBEDITOR_API FHxlbToolManagerCommands : public TCommands<FHxlbToolManagerCommands>
{
public:
	FHxlbToolManagerCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> BeginEmptyTool;
	TSharedPtr<FUICommandInfo> BeginHexMapSettingsTool;
	TSharedPtr<FUICommandInfo> BeginHexLayoutTool;
	TSharedPtr<FUICommandInfo> BeginHexSelectionTool;
	// *****************************************************************************************************************
};

// This is the main command registration class. It registers all individual tool command classes.
class FHxlbEditorToolsActionCommands : public TInteractiveToolCommands<FHxlbEditorToolsActionCommands>
{
public:
	FHxlbEditorToolsActionCommands();

	// TInteractiveToolCommands
	virtual void GetToolDefaultObjectList(TArray<UInteractiveTool*>& ToolCDOs) override;
	
	static void RegisterAllToolActions();
	static void UnregisterAllToolActions();
	static void UpdateToolCommandBinding(UInteractiveTool* Tool, TSharedPtr<FUICommandList> UICommandList, bool bUnbind = false);
};

#define DECLARE_TOOL_ACTION_COMMANDS(CommandsClassName) \
class CommandsClassName : public TInteractiveToolCommands<CommandsClassName> \
{\
public:\
CommandsClassName();\
virtual void GetToolDefaultObjectList(TArray<UInteractiveTool*>& ToolCDOs) override;\
};\

// Command registrations for the map settings tool.
DECLARE_TOOL_ACTION_COMMANDS(FHxlbMapSettingsToolActionCommands);

// Command registrations for the layout tool.
DECLARE_TOOL_ACTION_COMMANDS(FHxlbLayoutToolActionCommands);

// Command registrations for the hex selection tool.
DECLARE_TOOL_ACTION_COMMANDS(FHxlbHexSelectionActionCommands);

// Command registrations for the hex actor tool.
DECLARE_TOOL_ACTION_COMMANDS(FHxlbHexActorSelectionActionCommands);

// *****************************************************************************************************************