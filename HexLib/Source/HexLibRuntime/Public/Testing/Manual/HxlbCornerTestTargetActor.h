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

#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMeshActor.h"

#include "HxlbCornerTestTargetActor.generated.h"

UCLASS()
class HEXLIBRUNTIME_API AHxlbCornerTestTargetActor : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	AHxlbCornerTestTargetActor(const FObjectInitializer& Initializer);

	//~Begin AActor interface
	virtual void PostActorCreated() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; } // enables editor tick
	//~End AActor interface

	void RefreshPPVMaterialParams();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HexEngine")
	TWeakObjectPtr<APostProcessVolume> TargetPPV;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HexEngine")
	float HexSize = 100.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HexEngine")
	double RefreshRateSeconds = 0.01;

private:
	double LastRefreshTimeSeconds = 0.0;
};
