#include "SCIAsyncSaveImageTask.h"
#include "../VLog.h"
#include <Misc/FileHelper.h>

FSCIAsyncSaveImageTask::FSCIAsyncSaveImageTask( const TArray64<uint8>& InImage, const FString& InImageName )
{
    Image    = InImage;
    Filename = InImageName;
}

void FSCIAsyncSaveImageTask::DoWork()
{
    VLOG( Log, TEXT( "Starting save file." ) );
    FFileHelper::SaveArrayToFile( Image, *Filename );
    VLOG( Log, TEXT( "Stored Image: %s" ), *Filename );
}

TStatId FSCIAsyncSaveImageTask::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT( FSCIAsyncSaveImageTask, STATGROUP_ThreadPoolAsyncTasks );
}
