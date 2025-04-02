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

#include "HxlbEditorConstants.h"
#include "StatusBarSubsystem.h"
#include "Toolkits/BaseToolkit.h"

class HEXLIBEDITOR_API FHxlbToolkit: public FModeToolkit
{
public:
	FHxlbToolkit();

	//~ Begin FModeToolkit
	virtual void Init(const TSharedPtr<IToolkitHost>& Host, TWeakObjectPtr<UEdMode> OwningMode) override;
	virtual void InvokeUI() override;
	virtual void OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	virtual void OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool) override;
	//~ End FModeToolkit

	//~ Begin IToolkit interface.
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override;

	virtual void GetToolPaletteNames(TArray<FName>& PaletteNames) const override;
	virtual FText GetToolPaletteDisplayName(FName Palette) const override;
	virtual void BuildToolPalette(FName Palette, FToolBarBuilder& ToolbarBuilder) override;
	virtual void OnToolPaletteChanged(FName PaletteName) override;
	//~ End IToolkit interface.

	void ActivateDefaultToolForPalette(FName PaletteName);

protected:
	virtual void PostNotification(const FText& Message);
	virtual void ClearNotification();
	virtual void PostWarning(const FText& Message);
	virtual void ClearWarning();
	virtual void RefreshActiveToolMessage();

	void RegisterPalettes();
	
	void OnHexManagerChanged();

	TSharedPtr<SWidget> ToolkitWidget;
	bool bSkipDefaultToolActivation;

	TSet<EHxlbToolWarningId> ActiveWarnings;
	FText ActiveToolMessage;
	
	FStatusBarMessageHandle ActiveToolMessageHandle;
	TSharedPtr<STextBlock> ToolWarningArea;
};