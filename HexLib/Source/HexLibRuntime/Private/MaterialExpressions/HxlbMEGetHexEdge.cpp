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

#include "MaterialExpressions/HxlbMEGetHexEdge.h"

#include "MaterialCompiler.h"
#include "Materials/MaterialExpressionCustom.h"

#define LOCTEXT_NAMESPACE "HexMataterial_GetHexEdge"

UHxlbMEGetHexEdge::UHxlbMEGetHexEdge(const FObjectInitializer& Initializer): Super(Initializer)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FText HexMathCategory;
		FConstructorStatics(): HexMathCategory(LOCTEXT( "HexMathCategory", "Hex Math" ))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	DefaultHexSize = 100.0f;

#if WITH_EDITORONLY_DATA
	MenuCategories.Add(ConstructorStatics.HexMathCategory);
#endif
}

#if WITH_EDITOR
int32 UHxlbMEGetHexEdge::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
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
				return P_GetHexEdge(WorldCoord, HexSize);
			)");
	}
	else
	{
		return Compiler->Errorf(TEXT("Unsupported hex orientation: %s"), *UEnum::GetValueAsString(HexOrientation));
	}

	// Just to be safe, clear out the InternalExpression input. This should only be used by GenerateHLSLExpression
	// Uncomment if the new HLSL generator ever becomes the default.
	// InternalExpression->Inputs[0].Input = FExpressionInput();
	// InternalExpression->Inputs[1].Input = FExpressionInput();

	int32 IndexWorldCoord = WorldCoord.GetTracedInput().Expression ? WorldCoord.Compile(Compiler) : Compiler->WorldPosition(WPT_Default);
	int32 IndexHexSize = HexSize.GetTracedInput().Expression ? HexSize.Compile(Compiler) : Compiler->Constant(DefaultHexSize);
	TArray<int32> Inputs{ IndexWorldCoord, IndexHexSize };
	
	return Compiler->CustomExpression(InternalExpression, 0, Inputs);
}

void UHxlbMEGetHexEdge::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Hex Edge"));
}

void UHxlbMEGetHexEdge::GetExpressionToolTip(TArray<FString>& OutToolTip)
{
	ConvertToMultilineToolTip(TEXT("Given a 3D world coordinate, finds the associated hexgrid location and returns a 4D vector representing the 2D world coordinates of the two nearest edge points."), 40, OutToolTip);
}

void UHxlbMEGetHexEdge::InitializeExpression(UMaterialExpressionCustom* NewExpression)
{
	NewExpression->Inputs[0].InputName = TEXT("WorldCoord");
	NewExpression->Inputs.Add({ TEXT("HexSize") });
	NewExpression->OutputType = ECustomMaterialOutputType::CMOT_Float4;
	NewExpression->IncludeFilePaths.Add("/HexEngine/Shaders/HexMath.ush");
}

#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE