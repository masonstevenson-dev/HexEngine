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

#include "HexLibEditorModule.h"

#include "Actor/HxlbHexManager.h"
#include "DetailCustomizations/HxlbHexMapSettingsDetails.h"
#include "Framework/Docking/LayoutExtender.h"
#include "HexLibEditorLoggingDefs.h"
#include "HxlbEditorConstants.h"
#include "IPlacementModeModule.h"
#include "LevelEditor.h"
#include "Styles/HxlbStyle.h"
#include "Testing/Manual/HxlbCornerTestTargetActor.h"
#include "Tools/HxlbToolManagerCommands.h"
#include "ToolMenus.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "Macros/HexLibLoggingMacros.h"
#include "Subsystems/HxlbEditorMessageChannels.h"
#include "Subsystems/HxlbEditorMessagingSubsystem.h"

#define LOCTEXT_NAMESPACE "FHexLibEditorModule"

void FHexLibEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	HXLB_LOG(LogHxlbEditor, Log, TEXT("Starting HexLibEditorModule."));

	if (GEngine && GEngine->IsInitialized())
	{
		DoStartup();
	}
	else
	{
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FHexLibEditorModule::DoStartup);
	}
}

void FHexLibEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	HXLB_LOG(LogHxlbEditor, Log, TEXT("Shutting Down HexLibEditorModule"));

	if (!IsRunningGame())
	{
		if (auto* LevelEditorModule = FModuleManager::Get().GetModulePtr<FLevelEditorModule>("LevelEditor"))
		{
			LevelEditorModule->OnRegisterTabs().RemoveAll(this);
			LevelEditorModule->OnRegisterLayoutExtensions().RemoveAll(this);

			if (LevelEditorModule->GetLevelEditorTabManager())
			{
				LevelEditorModule->GetLevelEditorTabManager()->UnregisterTabSpawner(FHxlbEditorConstants::HexDetailsTabId);
			}
		}
	}

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout("HexMapSettingsDetails");
	}
	
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	FHxlbEditorToolsActionCommands::UnregisterAllToolActions();
	FHxlbToolManagerCommands::Unregister();

	if (IPlacementModeModule::IsAvailable())
	{
		if (MainPlacementModeHandle != "")
		{
			IPlacementModeModule::Get().UnregisterPlacementCategory(MainPlacementModeHandle);
		}
		if (DevPlacementModeHandle != "")
		{
			IPlacementModeModule::Get().UnregisterPlacementCategory(DevPlacementModeHandle);
		}
	}

	UToolMenus::UnRegisterStartupCallback(this);
	FHxlbStyle::Shutdown();
}

void FHexLibEditorModule::SetDetailsObjects(const TArray<UObject*>& Objects, UHxlbHex* BulkEditProxy)
{
	auto& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	LevelEditorModule.GetLevelEditorTabManager()->TryInvokeTab(FHxlbEditorConstants::HexDetailsTabId);
	
	if (!DetailsView.IsValid())
	{
		return;
	}
	
	if (BulkEditProxy)
	{
		bBulkEditModeEnabled = true;
		DetailsView->SetObject(BulkEditProxy);
	}
	else
	{
		bBulkEditModeEnabled = false;
		DetailsView->SetObjects(Objects, true);
	}

	if (DetailsHeaderImage.IsValid())
	{
		if (Objects.Num() > 0)
		{
			DetailsHeaderImage->SetVisibility(EVisibility::Visible);
		}
		else
		{
			DetailsHeaderImage->SetVisibility(EVisibility::Collapsed);
		}
	}

	if (DetailsHeaderText.IsValid())
	{
		if (Objects.Num() > 0)
		{
			UObject* HexObject = Objects[0];
			FText ObjectName = HexObject->GetClass()->GetDisplayNameText();
			
			if (BulkEditProxy)
			{
				DetailsHeaderText->SetText(FText::Format(LOCTEXT("HexBulkEditLabelFormat", "Bulk Edit ({0} selected)"), Objects.Num()));
			}
			else
			{
				DetailsHeaderText->SetText(FText::Format(LOCTEXT("HexSelectionLabelFormat", "{0} ({1} selected)"), ObjectName, Objects.Num()));
			}
		}
		else
		{
			DetailsHeaderText->SetText(LOCTEXT("SetDetailsEmptyHexSelectionLabel", ""));
		}
	}
}

void FHexLibEditorModule::DoStartup()
{
	FHxlbStyle::Initialize();

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FHexLibEditorModule::RegisterMenus));

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout("HxlbMapSettings", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FHxlbHexMapSettingsDetails::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged();
	
	FHxlbEditorToolsActionCommands::RegisterAllToolActions();
	FHxlbToolManagerCommands::Register();
	
	MainPlacementModeHandle = FName("HxlbPlacementCategory");
	DevPlacementModeHandle = FName("HxlbDevPlacementCategory");

	const FPlacementCategoryInfo AddMenuInfo(
	LOCTEXT("HxlbMainCategoryDisplayName", "Hex Engine"),
		FSlateIcon(FHxlbStyle::GetStyleSetName(), "HexLib.Icons.GenericHex"),
		MainPlacementModeHandle,
		TEXT("HexEngine")
	);

	const FPlacementCategoryInfo AddDevMenuInfo(
	LOCTEXT("HxlbDevCategoryDisplayName", "Hex Engine (Dev)"),
		FSlateIcon(FHxlbStyle::GetStyleSetName(), "HexLib.Icons.GenericHex"),
		DevPlacementModeHandle,
		TEXT("HexEngineDev")
	);

	if (IPlacementModeModule::IsAvailable())
	{
		auto& PlacementModeModule = IPlacementModeModule::Get();
		PlacementModeModule.RegisterPlacementCategory(AddMenuInfo);
		PlacementModeModule.RegisterPlacementCategory(AddDevMenuInfo);
	
		TArray<UClass*> HxlbClasses = {
			// The Hex Manager is now placeable via the HexEdit mode. Uncomment this if direct placement is needed.
			// AHxlbHexManager::StaticClass()
		};
		TArray<UClass*> HxlbDevClasses = {
			AHxlbCornerTestTargetActor::StaticClass()
		};
		
		for (UClass* HxlbClass : HxlbClasses)
		{
			PlacementModeModule.RegisterPlaceableItem(MainPlacementModeHandle, MakeShared<FPlaceableItem>(nullptr, FAssetData(HxlbClass)));
		}
		for (UClass* HxlbClass : HxlbDevClasses)
		{
			PlacementModeModule.RegisterPlaceableItem(DevPlacementModeHandle, MakeShared<FPlaceableItem>(nullptr, FAssetData(HxlbClass)));
		}
	}
	else
	{
		HXLB_LOG(LogHxlbEditor, Error, TEXT("IPlacementModeModule is not available!"))
	}
}

void FHexLibEditorModule::RegisterMenus()
{
	auto& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.OnRegisterTabs().AddRaw(this, &FHexLibEditorModule::RegisterTabs);
	LevelEditorModule.OnRegisterLayoutExtensions().AddRaw(this, &FHexLibEditorModule::RegisterLayout);
}

void FHexLibEditorModule::RegisterTabs(TSharedPtr<FTabManager> InTabManager)
{
	const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();

	const FSlateIcon HexDetailsIcon(FHxlbStyle::GetStyleSetName(), "HexLib.Icons.GenericHex");

	InTabManager->RegisterTabSpawner(FHxlbEditorConstants::HexDetailsTabId,
		FOnSpawnTab::CreateRaw(this, &FHexLibEditorModule::SpawnHexDetailsTab))
		.SetDisplayName(NSLOCTEXT("LevelEditorTabs", "HexDetailsTabName", "Hex Details"))
		.SetTooltipText(NSLOCTEXT("LevelEditorTabs", "HexDetailsTabTooltip", "Open the Hex Details Tab"))
		.SetGroup(MenuStructure.GetLevelEditorDetailsCategory())
		.SetIcon(HexDetailsIcon);
}

void FHexLibEditorModule::RegisterLayout(FLayoutExtender& Extender)
{
	Extender.ExtendLayout(
		LevelEditorTabIds::LevelEditorSelectionDetails,
		ELayoutExtensionPosition::After,
		FTabManager::FTab(FHxlbEditorConstants::HexDetailsTabId, ETabState::ClosedTab)
	);
}

TSharedRef<SDockTab> FHexLibEditorModule::SpawnHexDetailsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailArgs.bShowObjectLabel = false;
	DetailsView = PropertyModule.CreateDetailView(DetailArgs);

	// FSlateIcon(FHxlbStyle::GetStyleSetName(), ).GetIcon()
	
	DetailsHeaderImage = SNew(SImage)
		.Image(FHxlbStyle::Get()->GetBrush("HexLib.Icons.GenericHex"))
		.Visibility(EVisibility::Collapsed);
	
	DetailsHeaderText = SNew(STextBlock)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
		.ColorAndOpacity(FSlateColor(EStyleColor::White))
		.Text(LOCTEXT("DefaultHexSelectionLabel", ""));

	TAttribute<EVisibility> BulkEditVisibility = TAttribute<EVisibility>::CreateLambda([this]
	{
		return bBulkEditModeEnabled ? EVisibility::Visible : EVisibility::Collapsed;
	});
	
	TSharedRef<SDockTab> NewTab =
		SNew(SDockTab)
		.Label(NSLOCTEXT("LevelEditor", "HexDetailsTabTitle", "Hex Details"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.0f, 12.0f, 3.0f, 3.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0.0f)
				[
					DetailsHeaderImage.ToSharedRef()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					DetailsHeaderText.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				DetailsView.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SButton)
				.Text(LOCTEXT("CommitBulkEditChangesText", "Save Bulk Edits"))
				.HAlign(HAlign_Center)
				.Visibility(BulkEditVisibility)
				.OnClicked_Lambda([]()
				{
					if (auto* MessageSystem = UHxlbEditorMessagingSubsystem::Get())
					{
						MessageSystem->Publish(HxlbEditorMessageChannel::CommitBulkEdits);
					}

					return FReply::Handled();
				})
			]
		];

	return NewTab;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHexLibEditorModule, HexLibEditorModule);