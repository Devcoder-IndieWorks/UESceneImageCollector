// Copyright Devcoder.
#pragma once
#include <CoreMinimal.h>
#include <Async/AsyncWork.h>

class FSCIAsyncSaveImageTask : public FNonAbandonableTask
{
public:
    FSCIAsyncSaveImageTask( const TArray64<uint8>& InImage, const FString& InImageName );

    void DoWork();
    TStatId GetStatId() const;

protected:
    TArray64<uint8> Image;
    FString Filename;
};
