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
#include "HxlbMaterialExpressionBase.h"
#include "FunctionLibraries/HxlbMath.h"

#include "HxlbMEGetHexCenter.generated.h"

class UMaterialExpressionCustom;

UCLASS(MinimalAPI)
class UHxlbMEGetHexCenter : public UHxlbMaterialExpressionBase
{
	GENERATED_BODY()

public:
	UHxlbMEGetHexCenter(const FObjectInitializer& Initializer);

#if WITH_EDITOR
	//~ Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;

	// Note:
	//   The "new" HLSL generator has been in development since 2021 and is still labeled as WIP/Experimental as of
	//   UE5.5 (see r.MaterialEnableNewHLSLGenerator). Since it is unclear how stable this new system is, our preference
	//   is to keep this code commented out for now.
	// TODO(): Uncomment if the new HLSL generator ever becomes the default.
	// virtual bool GenerateHLSLExpression(FMaterialHLSLGenerator& Generator, UE::HLSLTree::FScope& Scope, int32 OutputIndex, UE::HLSLTree::FExpression const*& OutExpression) const override;
	
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual FText GetCreationName() const override { return FText::FromString(TEXT("Get Hex Center")); }
	virtual void GetExpressionToolTip(TArray<FString>& OutToolTip) override;
	//~ End UMaterialExpression Interface

	//~ Begin UHxlbMaterialExpressionBase interface
	virtual void InitializeExpression(UMaterialExpressionCustom* NewExpression) override;
	//~ End UHxlbMaterialExpressionBase interface
#endif // WITH_EDITOR

	UPROPERTY(meta = (RequiredInput = "true", ToolTip = "A coordinate in world space."))
	FExpressionInput WorldCoord;

	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "The size of an individual hex in the hex grid. Defaults to DefaultHexSize when this pin is not wired."))
	FExpressionInput HexSize;
	
	/** Currently only pointy hexes are supported*/
	UPROPERTY(EditAnywhere, Category=MaterialExpressionGetHexCenter)
	EHexOrientation HexOrientation = EHexOrientation::Pointy;

	/** only used if the HexSize pin is not wired. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionGetHexCenter, meta=(OverridingInputProperty = "HexSize"))
	float DefaultHexSize;
};
