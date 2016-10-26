#pragma once

#include <SGSpace.h>
#include <SGSymmetry.h>
#include <SGSplitPoint.h>


class SGToolCondition
{
public:
	SGToolCondition();

	void setSymmetry( int modeNum );
	
	void update();

	float vertexWeight;
	float manipScale;
	SGSymmetry symInfo;
	SGSpace  spaceInfo;
	
	static SGToolCondition option;
	static MString getOptionString();
	static bool toolIsOn;
};