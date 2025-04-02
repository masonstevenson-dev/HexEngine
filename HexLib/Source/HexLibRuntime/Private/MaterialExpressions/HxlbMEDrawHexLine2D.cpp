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

#include "MaterialExpressions/HxlbMEDrawHexLine2D.h"

#include "MaterialCompiler.h"
#include "Materials/MaterialExpressionCustom.h"

#define LOCTEXT_NAMESPACE "HexMataterial_MEDrawHexLine2D"

UHxlbMEDrawHexLine2D::UHxlbMEDrawHexLine2D(const FObjectInitializer& Initializer): Super(Initializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FText HexMathCategory;
		FConstructorStatics(): HexMathCategory(LOCTEXT( "HexUtilitiesCategory", "Hex Utilities" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(ConstructorStatics.HexMathCategory);
#endif
}

#if WITH_EDITOR
int32 UHxlbMEDrawHexLine2D::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	UMaterialExpressionCustom* InternalExpression = GetInternalExpression();
	if (!InternalExpression)
	{
		return Compiler->Errorf(TEXT("Internal expression is null."));
	}
	
	if (HexOrientation == EHexOrientation::Pointy)
	{
		InternalExpression->Code =
			TEXT(R"(
				return P_HexLine(WorldCoord, HexSize, HexInfo, LineWidth, EdgeFalloff);
			)");
	}
	else
	{
		return Compiler->Errorf(TEXT("Unsupported hex orientation: %s"), *UEnum::GetValueAsString(HexOrientation));
	}

	// Just to be safe, clear out the InternalExpression input. This should only be used by GenerateHLSLExpression
	// TODO(): Uncomment if the new HLSL generator ever becomes the default.
	// InternalExpression->Inputs[0].Input = FExpressionInput();
	// InternalExpression->Inputs[1].Input = FExpressionInput();
	// InternalExpression->Inputs[2].Input = FExpressionInput();
	// InternalExpression->Inputs[3].Input = FExpressionInput();
	// InternalExpression->Inputs[4].Input = FExpressionInput();

	int32 IndexWorldCoord = WorldCoord.Compile(Compiler);
	int32 IndexHexSize = HexSize.Compile(Compiler);
	int32 IndexHexInfo = HexInfo.Compile(Compiler);
	int32 IndexLineWidth = LineWidth.Compile(Compiler);
	int32 IndexEdgeFalloff = EdgeFalloff.Compile(Compiler);
	TArray<int32> Inputs{ IndexWorldCoord, IndexHexSize, IndexHexInfo, IndexLineWidth, IndexEdgeFalloff };
	
	return Compiler->CustomExpression(InternalExpression, 0, Inputs);
}

void UHxlbMEDrawHexLine2D::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Draw Hex Line 2D"));
}

void UHxlbMEDrawHexLine2D::GetExpressionToolTip(TArray<FString>& OutToolTip)
{
	ConvertToMultilineToolTip(TEXT("Given a world coordinate, draws the corresponding hex grid line."), 40, OutToolTip);
}

void UHxlbMEDrawHexLine2D::InitializeExpression(UMaterialExpressionCustom* NewExpression)
{
	NewExpression->Inputs[0].InputName = TEXT("WorldCoord");
	NewExpression->Inputs.Add({ TEXT("HexSize") });
	NewExpression->Inputs.Add({ TEXT("HexInfo") });
	NewExpression->Inputs.Add({ TEXT("LineWidth") });
	NewExpression->Inputs.Add({ TEXT("EdgeFalloff") });
	NewExpression->OutputType = ECustomMaterialOutputType::CMOT_Float1;
	NewExpression->IncludeFilePaths.Add("/HexEngine/Shaders/HexMath.ush");
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE