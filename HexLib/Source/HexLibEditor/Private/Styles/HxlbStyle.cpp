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

#include "Styles/HxlbStyle.h"

#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FHxlbStyle::StyleSet = nullptr;
TSharedPtr<ISlateStyle> FHxlbStyle::Get() { return StyleSet; }

FName FHxlbStyle::GetStyleSetName()
{
	static FName HxlbStyleName("HexLib");
	return HxlbStyleName;
}


void FHxlbStyle::Initialize()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	FString ProjectResourceDir = FPaths::ProjectPluginsDir() / TEXT("HexEngine/HexLib/Resources");
	StyleSet->SetContentRoot(ProjectResourceDir);
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	ConfigureStyleSet();

	// Finish init
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FHxlbStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

void FHxlbStyle::ConfigureStyleSet()
{
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);

	// general icons
	StyleSet->Set("HexLib.Icons.HexSelection", new FSlateVectorImageBrush(StyleSet->RootToContentDir("hex_selection", TEXT(".svg")), Icon20x20));
	StyleSet->Set("HexLib.Icons.GenericHex", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex", TEXT(".svg")), Icon20x20));
	
	// class icons
	StyleSet->Set("ClassIcon.HxlbHexManager", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex", TEXT(".svg")), Icon16x16));
	StyleSet->Set("ClassThumbnail.HxlbHexManager", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex", TEXT(".svg")), Icon64x64));

	StyleSet->Set("ClassIcon.HxlbHexMapComponent", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex", TEXT(".svg")), Icon16x16));
	StyleSet->Set("ClassThumbnail.HxlbHexMapComponent", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex", TEXT(".svg")), Icon64x64));

	StyleSet->Set("ClassIcon.TestActor", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex_pink", TEXT(".svg")), Icon16x16));
	StyleSet->Set("ClassThumbnail.TestActor", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex_blue", TEXT(".svg")), Icon64x64));

	// Tool icons
	StyleSet->Set("HxlbToolManagerCommands.BeginHexMapSettingsTool", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex", TEXT(".svg")), Icon20x20));
	StyleSet->Set("HxlbToolManagerCommands.BeginHexMapSettingsTool.Small", new FSlateVectorImageBrush(StyleSet->RootToContentDir("generic_hex", TEXT(".svg")), Icon20x20));
	
	StyleSet->Set("HxlbToolManagerCommands.BeginHexLayoutTool", new FSlateVectorImageBrush(StyleSet->RootToContentDir("hex_layout_2", TEXT(".svg")), Icon20x20));
	StyleSet->Set("HxlbToolManagerCommands.BeginHexLayoutTool.Small", new FSlateVectorImageBrush(StyleSet->RootToContentDir("hex_layout_2", TEXT(".svg")), Icon20x20));

	StyleSet->Set("HxlbToolManagerCommands.BeginHexSelectionTool", new FSlateVectorImageBrush(StyleSet->RootToContentDir("hex_selection", TEXT(".svg")), Icon20x20));
	StyleSet->Set("HxlbToolManagerCommands.BeginHexSelectionTool.Small", new FSlateVectorImageBrush(StyleSet->RootToContentDir("hex_selection", TEXT(".svg")), Icon20x20));
}
