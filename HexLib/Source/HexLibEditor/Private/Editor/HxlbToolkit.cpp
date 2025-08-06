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

#include "Editor/HxlbToolkit.h"

#include "Actor/HxlbHexManager.h"
#include "HexLibEditorLoggingDefs.h"
#include "Editor/HxlbEditorMode.h"
#include "EdModeInteractiveToolsContext.h"
#include "StatusBarSubsystem.h"
#include "Toolkits/AssetEditorModeUILayer.h"
#include "ToolkitBuilder.h"
#include "FunctionLibraries/HxlbEditorUtils.h"
#include "Macros/HexLibLoggingMacros.h"
#include "Tools/HxlbMapSettingsTool.h"
#include "Tools/HxlbToolManagerCommands.h"

#define LOCTEXT_NAMESPACE "FHxlbToolKit"

namespace HxlbPaletteNames
{
	static const FName UnifedPalette(TEXT("Hex Engine"));
	// static const FName MapSettingsPalette(TEXT("Grid Setup"));
	// static const FName ToolsPalette(TEXT("Tools"));
}

FHxlbToolkit::FHxlbToolkit()
{
}

void FHxlbToolkit::Init(const TSharedPtr<IToolkitHost>& Host, TWeakObjectPtr<UEdMode> OwningMode)
{
	// ToolkitBuilder seems to be the new way to build a custom tool UI. Currently this codepath isn't working for me.
	bUsesToolkitBuilder = false;

	// Have to create the ToolkitWidget here because FModeToolkit::Init() is going to ask for it and add
	// it to the Mode panel, and not ask again afterwards. However we have to call Init() to get the 
	// ModeDetailsView created, that we need to add to the ToolkitWidget. So, we will create the Widget
	// here but only add the rows to it after we call Init()
	TSharedPtr<SVerticalBox> ToolkitWidgetVBox = SNew(SVerticalBox);
	if (!bUsesToolkitBuilder)
	{
		SAssignNew(ToolkitWidget, SBorder)
		.HAlign(HAlign_Fill)
		.Padding(4)
		[
			ToolkitWidgetVBox->AsShared()
		];
	}

	FModeToolkit::Init(Host, OwningMode);
	
	ToolWarningArea = SNew(STextBlock)
		.AutoWrapText(true)
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
		.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.7f, 0.0f)));
	
	if (bUsesToolkitBuilder)
	{
		RegisterPalettes();
	}
	else
	{
		ToolkitWidgetVBox->AddSlot().AutoHeight().HAlign(HAlign_Fill).Padding(5)
		[
			ToolWarningArea->AsShared()
		];
	}

	ClearNotification();
	ClearWarning();
	
	if (HasToolkitBuilder())
	{
		ToolkitSections->ToolWarningArea = ToolWarningArea;

		SAssignNew(ToolkitWidget, SBorder)
		.HAlign(HAlign_Fill)
		.Padding(0)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			ToolkitBuilder->GenerateWidget()->AsShared()
		];
	}
	
	ActiveToolMessage = FText::GetEmpty();

	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolNotificationMessage.AddSP(this, &FHxlbToolkit::PostNotification);
	GetScriptableEditorMode()->GetInteractiveToolsContext(EToolsContextScope::EdMode)->OnToolWarningMessage.AddSP(this, &FHxlbToolkit::PostWarning);
	
	// Set which tab should be selected when you switch to Hex Editor mode.
	// skip default tool activation because the activation commands have not been registered yet
	bSkipDefaultToolActivation = true;
	SetCurrentPalette(HxlbPaletteNames::UnifedPalette);
	bSkipDefaultToolActivation = false;
}

void FHxlbToolkit::InvokeUI()
{
	FModeToolkit::InvokeUI();
	InlineContentHolder->SetContent(GetInlineContent().ToSharedRef());
}

void FHxlbToolkit::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	FModeToolkit::OnToolStarted(Manager, Tool);
	
	if (auto* SettingsTool = Cast<UHxlbMapSettingsTool>(Tool))
	{
		// TODO(): OnHexManagerChanged should also be called if the user manually deletes the HexManager from the scene.
		SettingsTool->OnHexManagerCreated.AddSP(this, &FHxlbToolkit::OnHexManagerChanged);
	}
}

void FHxlbToolkit::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	ClearNotification();
	ClearWarning();
	
	if (auto* SettingsTool = Cast<UHxlbMapSettingsTool>(Tool))
	{
		SettingsTool->OnHexManagerCreated.RemoveAll(this);
	}
}

FName FHxlbToolkit::GetToolkitFName() const
{
	return FName("HexLib");
}

FText FHxlbToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("HexLibToolkit", "DisplayName", "Hex Map Designer");
}

TSharedPtr<class SWidget> FHxlbToolkit::GetInlineContent() const
{
	auto BaseContent = FModeToolkit::GetInlineContent();
	
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		// .FillHeight(1.0f)
		//.VAlign(VAlign_Fill)
		//.Padding(0)
		.AutoHeight()
		.Padding(0)
		[
			ToolkitWidget.ToSharedRef()
		]
		+ SVerticalBox::Slot()
		[
			BaseContent.ToSharedRef()
		];
}

void FHxlbToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(HxlbPaletteNames::UnifedPalette);
	// PaletteNames.Add(HxlbPaletteNames::MapSettingsPalette);
	// PaletteNames.Add(HxlbPaletteNames::ToolsPalette);
}

FText FHxlbToolkit::GetToolPaletteDisplayName(FName Palette) const
{
	return FText::FromName(Palette);
}

void FHxlbToolkit::BuildToolPalette(FName Palette, FToolBarBuilder& ToolbarBuilder)
{
	const auto& ToolCommands = FHxlbToolManagerCommands::Get();

	if (Palette == HxlbPaletteNames::UnifedPalette)
	{
		ToolbarBuilder.AddToolBarButton(ToolCommands.BeginHexMapSettingsTool);

		// The layout tool is meant to be a "manual" mode where users can layout their own custom map shape.
		// It is disabled for now.
		// ToolbarBuilder.AddToolBarButton(ToolCommands.BeginHexLayoutTool);

		ToolbarBuilder.AddToolBarButton(ToolCommands.BeginHexSelectionTool);
	}
}

void FHxlbToolkit::OnToolPaletteChanged(FName PaletteName)
{
	ActivateDefaultToolForPalette(PaletteName);
}

void FHxlbToolkit::ActivateDefaultToolForPalette(FName PaletteName)
{
	if (bSkipDefaultToolActivation)
	{
		return;
	}
	
	const auto& ToolCommands = FHxlbToolManagerCommands::Get();
	bool bActivationSuccessful = false;
	
	if (PaletteName == HxlbPaletteNames::UnifedPalette)
	{
		if (ToolkitCommands->TryExecuteAction(ToolCommands.BeginHexMapSettingsTool.ToSharedRef()))
		{
			bActivationSuccessful = true;
		}
		else
		{
			HXLB_LOG(LogHxlbEditor, Warning, TEXT("Tried to switch to HexMapSettingsTool, but the action failed."));
		}
	}

	// Fallback: switch to a completly empty tool.
	if (!bActivationSuccessful)
	{
		if (!ToolkitCommands->TryExecuteAction(ToolCommands.BeginEmptyTool.ToSharedRef()))
		{
			HXLB_LOG(LogHxlbEditor, Warning, TEXT("Tried to switch to EmptyTool, but the action failed."));
		}
	}
}

void FHxlbToolkit::PostNotification(const FText& Message)
{
	ClearNotification();

	ActiveToolMessage = Message;

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		ActiveToolMessageHandle = GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PushStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessage);
	}
}

void FHxlbToolkit::ClearNotification()
{
	ActiveToolMessage = FText::GetEmpty();

	if (ModeUILayer.IsValid())
	{
		TSharedPtr<FAssetEditorModeUILayer> ModeUILayerPtr = ModeUILayer.Pin();
		GEditor->GetEditorSubsystem<UStatusBarSubsystem>()->PopStatusBarMessage(ModeUILayerPtr->GetStatusBarName(), ActiveToolMessageHandle);
	}
	ActiveToolMessageHandle.Reset();
}

void FHxlbToolkit::PostWarning(const FText& Message)
{
	if (Message.IsEmpty())
	{
		ClearWarning();
		return;
	}
	
	// We encode message ids directly into the warning text like: "ACTION_ID:MESSAGE_ID". Not ideal, but this is the
	// simplest way to pass more advanced messages to the toolkit message area without building out a completely
	// separate messaging system.
	FHxlbToolWarning NewWarning;
	if (HxlbEditorUtil::DecomposeToolWaring(Message, NewWarning))
	{
		if (NewWarning.Action == EHxlbToolWarningAction::DISPLAY)
		{
			ActiveWarnings.Add(NewWarning.Id);
		}
		else if (NewWarning.Action == EHxlbToolWarningAction::CLEAR)
		{
			ActiveWarnings.Remove(NewWarning.Id);
		}
		else
		{
			ActiveWarnings.Empty();
		}
	}
	else
	{
		// If the message doesn't follow the protocol, we just fall back on the old system where only a single message
		// is displayed at a time.
		ToolWarningArea->SetText(Message);
		ToolWarningArea->SetVisibility(EVisibility::Visible);
	}

	RefreshActiveToolMessage();
}

void FHxlbToolkit::ClearWarning()
{
	ToolWarningArea->SetText(FText());
	ToolWarningArea->SetVisibility(EVisibility::Collapsed);
}

void FHxlbToolkit::RefreshActiveToolMessage()
{
	if (ActiveWarnings.IsEmpty())
	{
		ClearWarning();
		return;
	}

	bool bFirstMessage = true;
	FString Message;
	if (ActiveWarnings.Contains(EHxlbToolWarningId::HexManagerMissing))
	{
		if (bFirstMessage)
		{
			bFirstMessage = false;
		}
		else
		{
			Message += "\n\n";
		}
		Message += "HexManager is missing.";
	}
	else if (ActiveWarnings.Contains(EHxlbToolWarningId::LandscapeMissing))
	{
		if (bFirstMessage)
		{
			bFirstMessage = false;
		}
		else
		{
			Message += "\n\n";
		}
		Message += "Landscape is missing.";
	}
	
	ToolWarningArea->SetText(FText::FromString(Message));
	ToolWarningArea->SetVisibility(EVisibility::Visible);
}

void FHxlbToolkit::RegisterPalettes()
{
	// Under construction
	/*
	ToolkitSections = MakeShared<FToolkitSections>();
	ToolkitBuilder = MakeShared<FToolkitBuilder>(FName("MyToolbarName"), GetToolkitCommands(), ToolkitSections);
	*/
}

void FHxlbToolkit::OnHexManagerChanged()
{
	if (DetailsView)
	{
		// We force the details view to refresh here so that the HexManager settings details can unhide itself.
		// See HxlbHexMapSettingsDetails.cpp for the visibility logic.
		DetailsView->ForceRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
