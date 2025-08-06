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

#include "DetailCustomizations/HxlbHexMapSettingsDetails.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "Foundation/HxlbHexMap.h"
#include "Subsystems/HxlbEditorMessageChannels.h"
#include "Subsystems/HxlbEditorMessagingSubsystem.h"

#define LOCTEXT_NAMESPACE "HxlbHexMapSettingsDetails"

TSharedRef<IPropertyTypeCustomization> FHxlbHexMapSettingsDetails::MakeInstance()
{
	return MakeShareable(new FHxlbHexMapSettingsDetails);
}

void FHxlbHexMapSettingsDetails::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TAttribute<EVisibility> VisibilityAttribute = TAttribute<EVisibility>::CreateLambda([this]
	{
		if (UHxlbEditorMode* HexEditorMode = GetEditorMode())
		{
			// TODO(): Maybe there's a better way to do this? Currently this will only be valid if the HexManager EdMode is open.
			if (HexEditorMode->FindHexManager())
			{
				return EVisibility::Visible; 
			}
		}

		return EVisibility::Collapsed;
	});
	
	HeaderRow.
	NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		PropertyHandle->CreatePropertyValueWidget()
	]
	.Visibility(VisibilityAttribute);
}

void FHxlbHexMapSettingsDetails::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (!PropertyHandle->IsValidHandle())
	{
		return;
	}
	
	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);

	TMap<FString, IDetailGroup*> CategoryGroups;
	
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		FString Category = ChildHandle->GetMetaData("Category");

		if (Category != "" && !CategoryGroups.Contains(Category))
		{
			IDetailGroup& NewGroup = ChildBuilder.AddGroup(FName(Category), FText::FromString(Category));
			NewGroup.SetDisplayMode(EDetailGroupDisplayMode::Category);
			CategoryGroups.Add(Category, &NewGroup);
		}

		if (CategoryGroups.Contains(Category))
		{
			CategoryGroups.FindChecked(Category)->AddPropertyRow(ChildHandle);
		}
		else
		{
			if (!CategoryGroups.Contains("UNCATEGORIZED"))
			{
				IDetailGroup& NewGroup = ChildBuilder.AddGroup("UNCATEGORIZED", FText::FromString("UNCATEGORIZED"));
				CategoryGroups.Add("UNCATEGORIZED", &NewGroup);
			}
			
			CategoryGroups.FindChecked("UNCATEGORIZED")->AddPropertyRow(ChildHandle);
		}
	}
	
	if (TSharedPtr<IPropertyHandle> OverlayToggleProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHxlbMapSettings, GridMode)))
	{
		TAttribute<EVisibility> VisibilityAttribute = TAttribute<EVisibility>::CreateLambda([OverlayToggleProperty]
		{
			uint8 OverlayEnabled;
			if (OverlayToggleProperty->GetValue(OverlayEnabled) == FPropertyAccess::Success && static_cast<EHexGridMode>(OverlayEnabled) == EHexGridMode::Landscape)
			{
				return EVisibility::Visible;
			}

			return EVisibility::Collapsed;
		});
		
		if (IDetailGroup** LandscapeCategory = CategoryGroups.Find("Landscape"))
		{
			(*LandscapeCategory)->AddWidgetRow()
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshLandscapeRTText", "Refresh Landscape RT"))
				.ToolTipText(LOCTEXT("RefreshOverlayTooltip", "Forces a recapture of the landscape heightmap. The results are written to the overlay Render Target."))
				.HAlign(HAlign_Center)
				.OnClicked(this, &FHxlbHexMapSettingsDetails::RefreshLandscapeRT)
			].Visibility(VisibilityAttribute);
		}
	}
	
	// Adds a generic button that we can use for various tests.
	if (!CategoryGroups.Contains("DEV"))
	{
		IDetailGroup& NewGroup = ChildBuilder.AddGroup("DEV", FText::FromString("DEV"));
		CategoryGroups.Add("DEV", &NewGroup);
	}
	CategoryGroups.FindChecked("DEV")->AddWidgetRow()
	[
		SNew(SButton)
		.Text(LOCTEXT("TestButtonText", "Test Button"))
		.ToolTipText(LOCTEXT("TestButtonTooltip", "Triggers test code."))
		.HAlign(HAlign_Center)
		.OnClicked(this, &FHxlbHexMapSettingsDetails::ExecuteTestButton)
	];
}

FReply FHxlbHexMapSettingsDetails::RefreshLandscapeRT()
{
	if (auto* MessageSystem = UHxlbEditorMessagingSubsystem::Get())
	{
		MessageSystem->Publish(HxlbEditorMessageChannel::RefreshLandscapeRT);
	}
	
	return FReply::Handled();
}

FReply FHxlbHexMapSettingsDetails::ExecuteTestButton()
{
	if (auto* MessageSystem = UHxlbEditorMessagingSubsystem::Get())
	{
		MessageSystem->Publish(HxlbEditorMessageChannel::EmptyTestMessage);
	}
	
	return FReply::Handled();	
}

#undef LOCTEXT_NAMESPACE