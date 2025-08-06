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
#include "Engine/TextureRenderTarget2D.h"

#include "HxlbTypes.generated.h"

namespace HxlbPackedData
{
	struct FHexInfo
	{
		// Data
		union 
		{
			// Note 1: Support for uint16_t in .ush files seems to be platform dependant (see PLATFORM_SUPPORTS_REAL_TYPES).
			//         For now, it's simpler to just pack everything into 8 bit chunks representing RG color channels
			//         instead of trying to pack a full 16 bit value and then reconstruct the data in hlsl.
			//
			// Note 2: Anonymous structs are non-standard C++, but this pattern appears to be supported by the compiler
			//         and used by epic in several different places. See FColor for example.
			struct
			{
				// Red Channel
				uint8 EdgeFlags	: 6;
				uint8 : 2;
	
				// Green Channel
				uint8 HighlightType : 3;
				uint8 : 5;
			};
			struct
			{
				uint8 R;
				uint8 G;
			};
			uint16 Raw;
		};

		// Constructors
		FHexInfo()
		{
			R = G = 0;
		}
		FHexInfo(const uint8 InR, const uint8 InG)
		{
			R = InR;
			G = InG;
		}
		FHexInfo(const FColor Color)
		{
			R = Color.R;
			G = Color.G;
		}
	};

	static constexpr ETextureRenderTargetFormat RequiredRTF = ETextureRenderTargetFormat::RTF_RG8;

	// Field masks
	static constexpr uint8 FM_EdgeFlags = 0b00111111u;
	static constexpr uint8 FM_HighlightType = 0b00000111u;
}

// WARNING: If you change this, you need to manually refresh any material expressions that reference it!
UENUM()
enum class EHxlbHighlightType : uint8
{
	None = 0,

	// Hover Types
	HoverSelection = 1,
	HoverRemoval = 2,

	// Selection types
	Selecting = 3,
	Selected = 4,
	Removing = 5,

	// UNUSED_6 = 6,
	// UNUSED_7 = 7,
};

// Most of the time, a raw FIntPoint is fine for describing a hex coordinate. Occasionally, we need to also check if the
// coordinate is set.
USTRUCT()
struct FHxlbHexCoord
{
	GENERATED_BODY()

public:
	FHxlbHexCoord() = default;
	FHxlbHexCoord(const FIntPoint& NewHexCoord) { Set(NewHexCoord); }

	void Set(const FIntPoint& NewHexCoord)
	{
		HexCoord = NewHexCoord;
		bIsSet = true;
	}
	const FIntPoint& Get() const
	{
		return HexCoord;
	}
	bool IsSet() const
	{
		return bIsSet;
	}
	void Clear()
	{
		HexCoord = FIntPoint::ZeroValue;
		bIsSet = false;
	}

protected:
	FIntPoint HexCoord = FIntPoint::ZeroValue;
	bool bIsSet = false;
};

USTRUCT()
struct FHxlbHexCoordDelta
{
	GENERATED_BODY()

public:
	void Clear()
	{
		PrevHex.Clear();
		CurrentHex.Clear();
	}
	void ClearCurrent()
	{
		if (!CurrentHex.IsSet())
		{
			return;
		}

		PrevHex.Set(CurrentHex.Get());
		CurrentHex.Clear();
	}
	
	const FIntPoint& GetPrev() const { return PrevHex.Get(); }
	bool HasPrev() const { return PrevHex.IsSet(); }
	
	void SetCurrent(const FIntPoint& NewCurrent)
	{
		if (CurrentHex.IsSet())
		{
			PrevHex.Set(CurrentHex.Get());
		}
		else
		{
			PrevHex.Clear();
		}
		
		CurrentHex.Set(NewCurrent);
	}
	const FIntPoint& GetCurrent() const { return CurrentHex.Get(); }
	bool HasCurrent() const { return CurrentHex.IsSet(); }

protected:
	FHxlbHexCoord PrevHex = FHxlbHexCoord();
	FHxlbHexCoord CurrentHex = FHxlbHexCoord();
};

USTRUCT()
struct FHxlbSelectionState
{
	GENERATED_BODY()

public:
	// Selection
	TSet<FIntPoint> SelectedHexes;
	TSet<FIntPoint> SelectingHexes;
	TSet<FIntPoint> RemovingHexes;
	FIntPoint FirstSelectedHex = FIntPoint::ZeroValue;

	bool bWriteSelectedToRT = true;
	bool bWriteSelectingToRT = true;
	bool bWriteRemovingToRT = true;
};