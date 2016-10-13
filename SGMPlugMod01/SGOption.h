#pragma once

#include <SGSpace.h>
#include <SGSymmetry.h>
#include <SGSplitPoint.h>


class SGOption
{
public:
	SGOption();

	void setSymmetry( int modeNum );
	void update();

	float vertexWeight;
	SGSymmetry symInfo;
	SGSpace  spaceInfo;
	
	static SGOption option;
};