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
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "HxlbHex.h"
#include "HxlbTypes.h"
#include "Data/HxlbHexTagInfo.h"
#include "FunctionLibraries/HxlbMath.h"

#include "HxlbHexMap.generated.h"

class ALandscape;
class UHxlbHexIteratorWrapper;

UENUM(BlueprintType)
enum class EHexMapShape : uint8
{
	Unbounded,
	Hexagonal,
	Rectangular,

	// Setting map shape to Manual enables the layout tool. For now, this is disabled.
	Manual UMETA(Hidden),
};

UENUM()
enum class EHexGridMode : uint8
{
	Proxy,
	Tiled,
	Landscape
};

USTRUCT(BlueprintType)
struct HEXLIBRUNTIME_API FHexagonalHexMapSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hexagonal Map Settings")
	int32 Radius = 3;
};

USTRUCT(BlueprintType)
struct HEXLIBRUNTIME_API FRectangularHexMapSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rectangular Map Settings")
	int32 Height = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rectangular Map Settings")
	int32 Width = 7;
};

USTRUCT(BlueprintType)
struct HEXLIBRUNTIME_API FHexMapOverlaySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay Settings")
	TSoftObjectPtr<ALandscape> TargetLandscape = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay Settings")
	TObjectPtr<UTextureRenderTarget2D> LandscapeHeightRT = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay Settings")
	TObjectPtr<UMaterialInterface> OverlayMaterial = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Overlay Settings")
	TObjectPtr<UTextureRenderTarget2D> PerHexDataRT = nullptr;
};

USTRUCT(BlueprintType)
struct HEXLIBRUNTIME_API FHxlbMapSettings
{
	GENERATED_BODY()

public:
	FHxlbMapSettings();

	UPROPERTY(EditAnywhere, Category="Grid")
	EHexGridMode GridMode = EHexGridMode::Proxy;
	
	UPROPERTY(EditAnywhere, Category="Map Shape")
	double HexSize = 500.0;
	
	UPROPERTY(EditAnywhere, Category="Map Shape")
	EHexMapShape Shape = EHexMapShape::Unbounded;

	// For now, only pointy orientation is supported.
	UPROPERTY(EditAnywhere, Category="Map Shape")
	EHexOrientation HexOrientation = EHexOrientation::Pointy;
	
	UPROPERTY(EditAnywhere, meta=(EditCondition = "Shape == EHexMapShape::Hexagonal", EditConditionHides), Category="Map Shape")
	FHexagonalHexMapSettings HaxagonalMapSettings;

	UPROPERTY(EditAnywhere, meta=(EditCondition = "Shape == EHexMapShape::Rectangular", EditConditionHides), Category="Map Shape")
	FRectangularHexMapSettings RectangularHexMapSettings;

	UPROPERTY(EditAnywhere, meta=(EditCondition = "GridMode == EHexGridMode::Landscape", EditConditionHides), Category="Landscape")
	FHexMapOverlaySettings OverlaySettings;

	UPROPERTY(EditAnywhere, Category = "Hex Data")
	TSubclassOf<UHxlbHex> DefaultHexClass = UHxlbHex::StaticClass();

	// Editor Only Properties -----------------------------------------------------------------------------------------
	// UPROPERTY(EditAnywhere, meta = (Categories = "HexGame.Map"), Category="Hex Data")
	UPROPERTY()
	FGameplayTag DebugTag;

	// UPROPERTY(EditAnywhere, meta = (Categories = "HexGame.Map"), Category="Hex Data")
	UPROPERTY()
	TMap<FGameplayTag, FHxlbHexTagInfo_Editor> TagSettings;
};

USTRUCT()
struct FHxlbHexMapUpdateOptions
{
	GENERATED_BODY()

public:
	bool bRefreshTagSettings = false;
	bool bForceLandscapeRTRefresh = false;
	bool bRefreshGridlines = false;
};

UCLASS(meta=(DisplayName="Hex Map Component"))
class HEXLIBRUNTIME_API UHxlbHexMapComponent : public USceneComponent
{
	GENERATED_BODY()

// constants
public:
	static constexpr int32 kBatchedUpdateThreshold = 7;

public:
	UHxlbHexMapComponent(const FObjectInitializer& Initializer);
	
	virtual void InitGridData(FVector NewGridOrigin);
	virtual bool IsValidAxialCoord(FIntPoint AxialCoord);
	virtual UHxlbHexIteratorWrapper* GetGridIterator(FIntPoint CameraCoord);
	UHxlbHex* GetHexData(FIntPoint AxialCoord) { return HexData.FindRef(AxialCoord); }
	virtual UHxlbHex* GetOrCreateHex(FIntPoint AxialCoord);
	virtual void ClearHexActors();
	virtual UHxlbHex* CreateBulkEditProxy();
	virtual void ClearBulkEditProxy();
	virtual void CommitBulkEdits();
	
#if WITH_EDITORONLY_DATA
	void Update(FHxlbMapSettings& NewMapSettings, FHxlbHexMapUpdateOptions UpdateOptions = FHxlbHexMapUpdateOptions());
	void RefreshLandscapeData();
	void HoverHex(FHxlbHexCoordDelta& HoverState, bool bIsSelecting = true);
	void UpdateSelection(FHxlbSelectionState& NewSelectionState);
#endif
	
	double GetHexSize() { return MapSettings.HexSize; }
	FIntPoint GetGridOrigin() { return GridOrigin; }
	
	UPROPERTY()
	FHxlbMapSettings MapSettings;
	
protected:
	void RefreshLandscapeRT(ALandscape* TargetLandscape);
	UTextureRenderTarget2D* GetHexInfoRT();
	virtual UTextureRenderTarget2D* GetHexInfoEditorRT();
	virtual UTextureRenderTarget2D* GetHexInfoGameRT();
	void RefreshGridlines();

	void WriteHexInfo_Bulk16(UTextureRenderTarget2D* PerHexDataRT, TArray<FIntPoint>& HexCoords, TArray<uint16> RawInfoArray, uint16 BitMask);
	void WriteHexInfo_16(UTextureRenderTarget2D* PerHexDataRT, FIntPoint HexCoord, uint16 RawInfo, uint16 BitMask);

	void SetHexHighlightType(FIntPoint HexCoord, EHxlbHighlightType HighlightType);
	
	FIntPoint GridOrigin = FIntPoint(0, 0);

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<UHxlbHex>> HexData;

	UPROPERTY()
	TObjectPtr<UHxlbHex> BulkEditProxy;
	
	// Represents the current state of selection in both the editor and at runtime
	UPROPERTY(Transient)
	FHxlbSelectionState SelectionState;

	double LandscapeHalfLengthCm = -1.0;
};
