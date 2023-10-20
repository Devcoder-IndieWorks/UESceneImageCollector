#include "SCIPathControllerVisProxies.h"

#if WITH_EDITOR
IMPLEMENT_HIT_PROXY(HSCIPathControllerVisProxy, HComponentVisProxy);
IMPLEMENT_HIT_PROXY(HSCIRollAngleKeyProxy,      HSCIPathControllerVisProxy);
IMPLEMENT_HIT_PROXY(HSCIEventPointKeyProxy,     HSCIPathControllerVisProxy);
IMPLEMENT_HIT_PROXY(HSCISpeedPointKeyProxy,     HSCIPathControllerVisProxy);
#endif
