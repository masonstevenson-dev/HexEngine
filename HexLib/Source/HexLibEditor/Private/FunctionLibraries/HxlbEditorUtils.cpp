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

#include "FunctionLibraries/HxlbEditorUtils.h"

#define LOCTEXT_NAMESPACE "HxlbEditorUtils"

FText HxlbEditorUtil::ComposeToolWarning(EHxlbToolWarningAction Action, EHxlbToolWarningId Warning)
{
	return FText::Format(LOCTEXT("ToolWarningFormat", "{0}:{1}"), static_cast<uint8>(Action), static_cast<uint8>(Warning));
}

bool HxlbEditorUtil::DecomposeToolWaring(const FText& Message, FHxlbToolWarning& InToolWarning)
{
	TArray<FString> Tokens;
	Message.ToString().ParseIntoArray(Tokens, TEXT(":"), true);

	if (Tokens.Num() != 2)
	{
		return false;
	}

	InToolWarning.Action = static_cast<EHxlbToolWarningAction>(FCString::Atoi(*Tokens[0]));
	InToolWarning.Id = static_cast<EHxlbToolWarningId>(FCString::Atoi(*Tokens[1]));;

	return true;
}

#undef LOCTEXT_NAMESPACE