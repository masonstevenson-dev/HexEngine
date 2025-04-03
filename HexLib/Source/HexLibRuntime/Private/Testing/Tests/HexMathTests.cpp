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

#include "HexLibRuntimeLoggingDefs.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "FunctionLibraries/HxlbMath.h"
#include "Logging/LogVerbosity.h"
#include "Macros/HexLibLoggingMacros.h"
#include "Misc/AutomationTest.h"

using HexMath = UHxlbMath;

#if WITH_EDITOR

class TestSuite
{
public:
	TestSuite(FAutomationTestBase* NewTestFramework): TestFramework(NewTestFramework)
	{
		// This constructor is run before each test.
	}

	~TestSuite()
	{
		// This destructor is run after each test.
	}

	void Test_AxialToTexture()
	{
		FIntPoint Result1;
		TestFramework->TestTrue(TEXT("AxialToTexture((0,0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 0), 4096, 4096, Result1));
		TestFramework->TestEqual(TEXT("Result1"), Result1, FIntPoint(2048, 2048));

		FIntPoint Result2;
		TestFramework->TestTrue(TEXT("AxialToTexture((-2046, -2046), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2046, -2046), 4096, 4096, Result2));
		TestFramework->TestEqual(TEXT("Result2"), Result2, FIntPoint(2, 2));

		FIntPoint Result3;
		TestFramework->TestTrue(TEXT("AxialToTexture((2045, 2045), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2045, 2045), 4096, 4096, Result3));
		TestFramework->TestEqual(TEXT("Result2"), Result3, FIntPoint(4093, 4093));
	}

	void Test_AxialToTextureOffsetOverride()
	{
		FIntPoint Result1;
		TestFramework->TestTrue(TEXT("AxialToTexture((0,0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 0), 4096, 4096, Result1, 1));
		TestFramework->TestEqual(TEXT("Result1"), Result1, FIntPoint(2048, 2048));

		FIntPoint Result2;
		TestFramework->TestTrue(TEXT("AxialToTexture((-2047, -2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2047, -2047), 4096, 4096, Result2, 1));
		TestFramework->TestEqual(TEXT("Result2"), Result2, FIntPoint(1, 1));

		FIntPoint Result3;
		TestFramework->TestTrue(TEXT("AxialToTexture((2046, 2046), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2046, 2046), 4096, 4096, Result3, 1));
		TestFramework->TestEqual(TEXT("Result2"), Result3, FIntPoint(4094, 4094));

		FIntPoint Result4;
		TestFramework->TestTrue(TEXT("AxialToTexture((0,0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 0), 4096, 4096, Result4, 0));
		TestFramework->TestEqual(TEXT("Result4"), Result4, FIntPoint(2048, 2048));

		FIntPoint Result5;
		TestFramework->TestTrue(TEXT("AxialToTexture((-2048, -2048), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2048, -2048), 4096, 4096, Result5, 0));
		TestFramework->TestEqual(TEXT("Result5"), Result5, FIntPoint(0, 0));

		FIntPoint Result6;
		TestFramework->TestTrue(TEXT("AxialToTexture((2047, 2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2047, 2047), 4096, 4096, Result6, 0));
		TestFramework->TestEqual(TEXT("Result6"), Result6, FIntPoint(4095, 4095));
	}
	
	void Test_AxialToTexture_InvalidBufferSize()
	{
		FIntPoint OutTextureCoord;
		TestFramework->TestFalse(TEXT("AxialToTexture(Coord, 0, 0, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 0), 0, 0, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture(Coord, -1, -1, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 0),-1, -1, OutTextureCoord));
	}

	void Test_AxialToTexture_PixelCoordsOutOfBounds()
	{
		FIntPoint OutTextureCoord;
		TestFramework->TestFalse(TEXT("AxialToTexture((-2049, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2049, 0), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((-2048, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2048, 0), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((-2047, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2047, 0), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((2048, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2048, 0), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((2047, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2047, 0), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((2046, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2046, 0), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, -2049), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2049), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, -2048), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2048), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, -2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2047), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, 2048), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2048), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, 2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2047), 4096, 4096, OutTextureCoord));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, 2046), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2046), 4096, 4096, OutTextureCoord));
	}

	void Test_AxialToTexture_PixelCoordsOutOfBounds_BufferOverride()
	{
		FIntPoint OutTextureCoord;
		TestFramework->TestFalse(TEXT("AxialToTexture((-2049, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2049, 0), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestFalse(TEXT("AxialToTexture((-2048, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2048, 0), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestTrue(TEXT("AxialToTexture((-2047, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2047, 0), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestFalse(TEXT("AxialToTexture((2048, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2048, 0), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestFalse(TEXT("AxialToTexture((2047, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2047, 0), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestTrue(TEXT("AxialToTexture((2046, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2046, 0), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, -2049), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2049), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, -2048), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2048), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestTrue(TEXT("AxialToTexture((0, -2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2047), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, 2048), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2048), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, 2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2047), 4096, 4096, OutTextureCoord, 1));
		TestFramework->TestTrue(TEXT("AxialToTexture((0, 2046), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2046), 4096, 4096, OutTextureCoord, 1));

		TestFramework->TestFalse(TEXT("AxialToTexture((-2049, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2049, 0), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((-2048, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2048, 0), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((-2047, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(-2047, 0), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestFalse(TEXT("AxialToTexture((2048, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2048, 0), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((2047, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2047, 0), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((2046, 0), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(2046, 0), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, -2049), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2049), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((0, -2048), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2048), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((0, -2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, -2047), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestFalse(TEXT("AxialToTexture((0, 2048), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2048), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((0, 2047), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2047), 4096, 4096, OutTextureCoord, 0));
		TestFramework->TestTrue(TEXT("AxialToTexture((0, 2046), 4096, 4096, OUT)"), HexMath::AxialToTexture(FIntPoint(0, 2046), 4096, 4096, OutTextureCoord, 0));
	}

	// IMPORTANT! Be sure to register your fn inside your AutomationTest class below!

private:
	FAutomationTestBase* TestFramework;
};

#define REGISTER_TEST_SUITE_FN(TargetTestName) Tests.Add(TEXT(#TargetTestName), &TestSuite::TargetTestName)

class FHxlbMathTests: public FAutomationTestBase
{
public:
	typedef void (TestSuite::*TestFunction)();
	
	FHxlbMathTests(const FString& TestName): FAutomationTestBase(TestName, false)
	{
		REGISTER_TEST_SUITE_FN(Test_AxialToTexture);
		REGISTER_TEST_SUITE_FN(Test_AxialToTextureOffsetOverride);
		REGISTER_TEST_SUITE_FN(Test_AxialToTexture_InvalidBufferSize);
		REGISTER_TEST_SUITE_FN(Test_AxialToTexture_PixelCoordsOutOfBounds);
		REGISTER_TEST_SUITE_FN(Test_AxialToTexture_PixelCoordsOutOfBounds_BufferOverride);
	}
	
	virtual uint32 GetTestFlags() const override
	{
		return EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	}
	virtual bool IsStressTest() const { return false; }
	virtual uint32 GetRequiredDeviceNum() const override { return 1; }

protected:
	virtual FString GetBeautifiedTestName() const override
	{
		// This string is what the editor uses to organize your test in the Automated tests browser.
		return "HexEngine.Runtime.HexMathTests";
	}
	virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override
	{
		TArray<FString> TargetTestNames;
		Tests.GetKeys(TargetTestNames);
		for (const FString& TargetTestName : TargetTestNames)
		{
			OutBeautifiedNames.Add(TargetTestName);
			OutTestCommands.Add(TargetTestName);
		}
	}
	virtual bool RunTest(const FString& Parameters) override
	{
		TestFunction* CurrentTest = Tests.Find(Parameters);
		if (!CurrentTest || !*CurrentTest)
		{
			HXLB_LOG(LogHxlbRuntime, Error, TEXT("Cannot find test: %s"), *Parameters);
			return false;
		}

		TestSuite Suite(this);
		(Suite.**CurrentTest)(); // Run the current test from the test suite.

		return true;
	}

	TMap<FString, TestFunction> Tests;
};

namespace
{
	FHxlbMathTests FHxlbMathTestsInstance(TEXT("FHxlbMathTests"));
}

#endif //WITH_EDITOR