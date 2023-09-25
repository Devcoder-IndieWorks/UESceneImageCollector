// Copyright Vivestudios. All Rights Reserved.
#pragma once
#include <CoreMinimal.h>

DECLARE_LOG_CATEGORY_EXTERN( LogVRTEditor, Log, All )
DECLARE_LOG_CATEGORY_EXTERN( LogVRTEditorInternal, NoLogging, All )

//-----------------------------------------------------------------------------

#define VLOG_CALLINFO                              (FString( TEXT( "[" ) ) + FString( __FUNCTION__ ) + TEXT( "(" ) + FString::FromInt( __LINE__ ) + TEXT( ")" ) + FString( TEXT( "]" ) ) )
#define VLOG_CALLONLY( Verbosity )                 UE_LOG( LogVRTEditor, Verbosity, TEXT( "%s" ), *VLOG_CALLINFO )
#define VLOG( Verbosity, Format, ... )             UE_LOG( LogVRTEditor, Verbosity, TEXT( "%s LOG: ##### %s ######" ), *VLOG_CALLINFO, *FString::Printf( Format, ##__VA_ARGS__ ) )
#define VCLOG( Condition, Verbosity, Format, ... ) UE_CLOG( Condition, LogVRTEditor, Verbosity, TEXT( "%s LOG: ###### %s #####" ), *VLOG_CALLINFO, *FString::Printf( Format, ##__VA_ARGS__ ) )

//-----------------------------------------------------------------------------

#define VLOG_INTERNAL( Verbosity, Format, ... ) UE_LOG( LogVRTEditorInternal, Verbosity, TEXT( "%s LOG: ##### %s #####" ), *VLOG_CALLINFO, *FString::Printf( Format, ##__VA_ARGS__ ) )
