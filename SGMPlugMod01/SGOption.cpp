#include "precompile.h"
#include "SGOption.h"
#include "SGGeneralManip.h"
#include "SGTransformManip.h"
#include "SGNormalManip.h"
#include "SGPolygonManip.h"
#include "SGSoftSelectionManip.h"
#include <SGSelection.h>
#include <SGIntersectResult.h>
#include "SGPrintf.h"

extern vector<SGGeneralManip> GeneralManips;
extern SGTransformManip       transManip;
extern SGNormalManip          normalManip;
extern SGPolygonManip         polygonManip;
extern SGSoftSelectionManip   softSelectionManip;

extern vector<SGIntersectResult> generalResult;
extern vector<SGIntersectResult> edgeSplitIntersectResult;

extern vector<vector<SGSplitPoint>> spPointsArr;
extern vector<int>  snapIndexArr;

SGOption SGOption::option;

SGOption::SGOption() {
	vertexWeight = 0.5;
	symInfo.setNoMirror();
	update();
}

void SGOption::update() {
	
	if (symInfo.isNoMirror()) {
		sgPrintf("is no mirror");
		spPointsArr.resize(1);
		snapIndexArr.resize(1);
		generalResult.resize(1);
		edgeSplitIntersectResult.resize(1);
	}
	if (symInfo.isXMirror()) {
		sgPrintf("is x mirror");
		spPointsArr.resize(2);
		snapIndexArr.resize(2);
		generalResult.resize(2);
		edgeSplitIntersectResult.resize(2);
	}
	for (int i = 0; i < spPointsArr.size(); i++) {
		spPointsArr[i].clear();
	}
}


void SGOption::setSymmetry(int modeNum)
{
	if (modeNum == 1) {
		symInfo.setXMirror();
	}
	else {
		symInfo.setNoMirror();
	}
	update();
}