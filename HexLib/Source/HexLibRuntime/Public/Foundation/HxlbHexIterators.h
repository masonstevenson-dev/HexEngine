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
#include "UObject/Object.h"

#include "HxlbHexIterators.generated.h"

USTRUCT()
struct HEXLIBRUNTIME_API FHxlbHexIterator
{
	GENERATED_BODY()

public:
	FHxlbHexIterator() = default;
	virtual ~FHxlbHexIterator() = default;

	virtual bool Next() { return false; }
	virtual FIntPoint Get() { return FIntPoint(); }

protected:
	// These are commonly used by most subclasses
	FIntPoint Origin = FIntPoint::ZeroValue;
	FIntPoint Current = FIntPoint::ZeroValue;
	bool bIsInitialized = false;
	bool bFirstIteration = false;
};

// Gets all hexes within a given radius.
USTRUCT()
struct HEXLIBRUNTIME_API FHxlbRadialIterator: public FHxlbHexIterator
{
	GENERATED_BODY()

public:
	FHxlbRadialIterator() = default;
	FHxlbRadialIterator(FIntPoint NewOrigin, int32 NewRadius);

	virtual bool Next() override;
	virtual FIntPoint Get() override;

protected:
	int32 Radius = 0;
};

// Gets all hexes on the border of a given radius. Can set a search range to automatically update increment the ring.
USTRUCT()
struct HEXLIBRUNTIME_API FHxlbRingIterator: public FHxlbHexIterator
{
	GENERATED_BODY()
	
	FHxlbRingIterator() = default;
	FHxlbRingIterator(FIntPoint NewOrigin, int32 NewRadius);
	
	virtual bool Next() override;
	virtual FIntPoint Get() override;
	
protected:
	FIntVector CubeCurrent = FIntVector::ZeroValue;
	int32 Radius = 0;
	int32 CurrentDirection = 0;
	int32 SideCount = 0;
};

USTRUCT()
struct HEXLIBRUNTIME_API FHxlbRectangularIterator : public FHxlbHexIterator
{
	GENERATED_BODY()

public:
	FHxlbRectangularIterator() = default;
	FHxlbRectangularIterator(FIntPoint NewOrigin, int32 NewHalfWidth, int32 NewHalfHeight);
	FHxlbRectangularIterator(FIntPoint NewStartHex, FIntPoint NewEndHex);

	virtual bool Next() override;
	virtual FIntPoint Get() override;

protected:
	int32 HalfWidth = 0;
	int32 HalfHeight = 0;
	FIntPoint StartHex = FIntPoint::ZeroValue;
	FIntPoint EndHex = FIntPoint::ZeroValue;
	int32 Left = 0;
	int32 Right = 0;
	bool bReflectWidth = false;
	bool bReflectHeight = false;
};

// Wrapper class for hex iterators. Allows for polymorphic access to iterators (returning a raw struct would result in
// value slicing).
UCLASS()
class HEXLIBRUNTIME_API UHxlbHexIteratorWrapper: public UObject
{
	GENERATED_BODY()

public:
	virtual bool Next() { return false; }
	virtual FIntPoint Get() { return FIntPoint(); }
};

UCLASS()
class HEXLIBRUNTIME_API UHxlbRadialIW: public UHxlbHexIteratorWrapper
{
	GENERATED_BODY()

public:
	virtual bool Next() override { return Iterator.Next(); }
	virtual FIntPoint Get() override { return Iterator.Get(); }

	FHxlbRadialIterator Iterator;
};

UCLASS()
class HEXLIBRUNTIME_API UHxlbRectangularIW : public UHxlbHexIteratorWrapper
{
	GENERATED_BODY()

public:
	virtual bool Next() override { return Iterator.Next(); }
	virtual FIntPoint Get() override { return Iterator.Get(); }

	FHxlbRectangularIterator Iterator;
};