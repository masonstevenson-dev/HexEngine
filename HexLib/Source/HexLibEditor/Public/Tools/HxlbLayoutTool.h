﻿// Copyright © Mason Stevenson
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
#include "HxlbSelectionToolBase.h"
#include "InteractiveTool.h"

#include "HxlbLayoutTool.generated.h"

UCLASS()
class HEXLIBEDITOR_API UHxlbLayoutToolActions : public UHxlbSelectionToolActionsBase
{
	GENERATED_BODY()

public:
	
	/** Can also be invoked with E. */
	UFUNCTION(CallInEditor, Category = Layout, meta = (DisplayPriority = 1))
	void Add() { RequestActionFromParent(EHxlbSelectionToolAction::CreateHexProxies); }
};

UCLASS()
class UHxlbLayoutToolBuilder : public UHxlbSelectionToolBuilderBase
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

UCLASS()
class UHxlbLayoutTool : public UHxlbSelectionToolBase
{
	GENERATED_BODY()

public:
	//~ Begin UInteractiveTool interface.
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void RegisterActions(FInteractiveToolActionSet& ActionSet) override;
	//~ End UInteractiveTool interface.

protected:
	virtual void ConfigureActionProperties() override;
	
	UPROPERTY()
	TObjectPtr<UHxlbLayoutToolActions> ToolActions;
};
