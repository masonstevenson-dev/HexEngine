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
#include "EditorSubsystem.h"
#include "HxlbEditorMessageSubscriber.h"
#include "InstancedStruct.h"

#include "HxlbEditorMessagingSubsystem.generated.h"

struct FGameplayTag;

UCLASS()
class HEXLIBEDITOR_API UHxlbEditorMessagingSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	static UHxlbEditorMessagingSubsystem* Get()
	{
		if (!GEditor)
		{
			return nullptr;
		}

		return GEditor->GetEditorSubsystem<UHxlbEditorMessagingSubsystem>();
	}
	
	//~ Begin UEditorSubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UEditorSubsystem interface

	template <typename FSubscriberType>
	bool Subscribe(FName Channel, FSubscriberType* Subscriber)
	{
		static_assert(TIsDerivedFrom<FSubscriberType, UObject>::IsDerived, "Subscriber must be a UObject");
		static_assert(TIsDerivedFrom<FSubscriberType, IHxlbEditorMessageSubscriberInterface>::IsDerived, "Subscriber must implement IHxlbEditorMessageSubscriberInterface");
		
		return SubscribeInternal(Channel, Subscriber);
	}

	template <typename FSubscriberType>
	bool Unsubscribe(FName Channel, FSubscriberType* Subscriber)
	{
		static_assert(TIsDerivedFrom<FSubscriberType, UObject>::IsDerived, "Subscriber must be a UObject");
		static_assert(TIsDerivedFrom<FSubscriberType, IHxlbEditorMessageSubscriberInterface>::IsDerived, "Subscriber must implement IHxlbEditorMessageSubscriberInterface");
		
		return UnsubscribeInternal(Channel, Subscriber);
	}

	void Publish(FName Channel)
	{
		auto EmptyMessage = FInstancedStruct();
		PublishInternal(Channel, EmptyMessage);
	}

	template <typename FMessageStructType>
	void Publish(FName Channel, const FMessageStructType& Message)
	{
		// Note using void* or std::any would probably be faster here, but FInstancedStruct seems to be the safer option.
		// Might be worth revisiting this at some point.
		auto InstancedMessage = FInstancedStruct::Make(Message);
		PublishInternal(Channel, InstancedMessage);
	}

protected:
	bool SubscribeInternal(FName Channel, UObject* Subscriber);
	bool UnsubscribeInternal(FName Channel, UObject* Subscriber);
	void PublishInternal(FName Channel, FInstancedStruct& MessagePayload);
	
	TMap<FName, TSet<TWeakObjectPtr<UObject>>> SubscriberMap;
};
