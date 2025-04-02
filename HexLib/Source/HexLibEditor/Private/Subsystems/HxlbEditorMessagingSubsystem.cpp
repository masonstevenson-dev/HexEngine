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

#include "Subsystems/HxlbEditorMessagingSubsystem.h"

#include "HexLibEditorLoggingDefs.h"

void UHxlbEditorMessagingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UHxlbEditorMessagingSubsystem::Deinitialize()
{
	SubscriberMap.Empty();

	Super::Deinitialize();
}

bool UHxlbEditorMessagingSubsystem::SubscribeInternal(FName Channel, UObject* Subscriber)
{
	TSet<TWeakObjectPtr<UObject>>& SubscriberSet = SubscriberMap.FindOrAdd(Channel);

	if (SubscriberSet.Contains(Subscriber))
	{
		return false;
	}

	SubscriberSet.Add(Subscriber);
	return true;
}

bool UHxlbEditorMessagingSubsystem::UnsubscribeInternal(FName Channel, UObject* Subscriber)
{
	TSet<TWeakObjectPtr<UObject>>* SubscriberSet = SubscriberMap.Find(Channel);
	if (!SubscriberSet || !SubscriberSet->Contains(Subscriber))
	{
		return false;
	}

	SubscriberSet->Remove(Subscriber);
	return true;
}

void UHxlbEditorMessagingSubsystem::PublishInternal(FName Channel, FInstancedStruct& MessagePayload)
{
	TSet<TWeakObjectPtr<UObject>>* SubscriberSet = SubscriberMap.Find(Channel);
	if (!SubscriberSet)
	{
		return;
	}

	for(auto CurrentSubscriber = SubscriberSet->CreateIterator(); CurrentSubscriber; ++CurrentSubscriber)
	{
		UObject* SubscriberObj = CurrentSubscriber->Get();
		auto* SubscriberInterface = Cast<IHxlbEditorMessageSubscriberInterface>(SubscriberObj);
		if (SubscriberInterface)
		{
			if (!SubscriberInterface->RouteHexEditorMessage(Channel, MessagePayload))
			{
				UE_LOG(LogHxlbEditor, Error, TEXT("Subscriber %s was unable to route message on channel %s. Removing subscriber."), *GetNameSafe(SubscriberObj), *Channel.ToString());
				CurrentSubscriber.RemoveCurrent();
			}
		}
		else
		{
			CurrentSubscriber.RemoveCurrent();
		}
	}
}
