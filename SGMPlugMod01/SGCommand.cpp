#include "precompile.h"

#include "SGCommand.h"

#include <maya/M3dView.h>
#include <maya/MPlug.h>
#include <maya/MArgDatabase.h>
#include <maya/MPlugArray.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnComponentListData.h>
#include <SGPrintf.h>
#include "SGNodeControl.h"
#include "SGMesh.h"
#include "SGTransformManip.h"
#include "SGTimeCheck.h"
#include "Names.h"
#include "SGOption.h"


MDagPath  SGCommand::dagPathMesh;
MPointArray SGCommand::vmPoints_before;
MPointArray SGCommand::vmPoints_after;
MIntArray   SGCommand::vmMoveIndices;
MIntArray   SGCommand::vmMergeIndices;

vector<MIntArray>   SGCommand::psIndices;
vector<MFloatArray> SGCommand::psParams;
vector<MPointArray>  SGCommand::psPoints;

MIntArray   SGCommand::delEdgeIndices;
MIntArray   SGCommand::delPolyIndices;

vector<int> SGCommand::rootEdges;
vector<float> SGCommand::rootWeights;
vector<MIntArray> SGCommand::indicesEdges;

MIntArray SGCommand::bvlIndices;
double SGCommand::bvlOffset;


extern SGTransformManip transManip;

void SGCommand::clearResult() {
	SGCommand::vmPoints_before.clear();
	SGCommand::vmPoints_after.clear();
}

SGCommand::SGCommand() {
	m_isSt = false;
	m_isVm = false;
	m_isDel = false;
	m_isUpm = false;
	m_isUpc = false;
	m_isSym = 0;
	m_undoableValue = true;
}

SGCommand::~SGCommand() {
}


void* SGCommand::creator() {
	return new SGCommand();
}


MSyntax SGCommand::newSyntax() {
	MSyntax syntax;
	syntax.addFlag("-st", "-setTool", MSyntax::kNoArg);
	syntax.addFlag("-vm", "-vtxMove", MSyntax::kNoArg);
	syntax.addFlag("-del", "-deleteComponent", MSyntax::kNoArg);
	syntax.addFlag("-bvl", "-bevelEdge", MSyntax::kNoArg);
	syntax.addFlag("-exf", "-extrudeFace", MSyntax::kNoArg);
	syntax.addFlag("-upm", "-updateMesh", MSyntax::kNoArg);
	syntax.addFlag("-upc", "-updateCenter", MSyntax::kNoArg);
	syntax.addFlag("-spz", "-setPntsZero", MSyntax::kNoArg);
	syntax.addFlag("-sym", "-symmetry", MSyntax::kLong);
	return syntax;
}


MStatus SGCommand::doIt(const MArgList& arg) {
	MStatus status;
	m_dagPathMesh = dagPathMesh;
	MString argStr = arg.asString(0);

	if (argStr == "-st")  m_isSt = true;
	if (argStr == "-vm")  m_isVm = true;
	if (argStr == "-del") m_isDel = true;
	if (argStr == "-upm") m_isUpm = true;
	if (argStr == "-upc") m_isUpc = true;
	if (argStr == "-spz") m_isPntsZero = true;
	if (argStr == "-sym") {
		m_isSym = arg.asInt(1);
		m_undoableValue = false;
		SGOption::option.setSymmetry(m_isSym);
		SGMesh::pMesh->update(SGOption::option.symInfo, true );
		return MS::kSuccess;
	}

	if (m_isSt) {
	}
	else if (m_isVm) {
		m_vmPointsBefore = vmPoints_before;
		m_vmPointsAfter = vmPoints_after;
		m_vmMoveIndices = vmMoveIndices;
		m_vmMergeIndices = vmMergeIndices;
	}
	else if (m_isDel) {
		m_delEdgeIndices = delEdgeIndices;
		m_delPolyIndices = delPolyIndices;
	}

	vmPoints_before.clear();
	vmPoints_after.clear();
	vmMergeIndices.clear();
	psIndices.clear();
	psParams.clear();
	delEdgeIndices.clear();
	delPolyIndices.clear();

	return redoIt();
}

MStatus SGCommand::redoIt() {
	M3dView active3dView = M3dView().active3dView();
	MFnMesh fnMesh = m_dagPathMesh;

	SGMesh* pMesh = SGMesh::pMesh;

	if (m_isSt) {
		if( !checkMeshIsSelected() ){
			MGlobal::displayError("selection is not exists");
			return MS::kFailure; 
		}
		else {
			MGlobal::executeCommand("currentCtx;", m_stResults);
			MString command = MString("setToolTo ") + Names::toolName;
			MGlobal::executeCommand(command);
			return MS::kSuccess;
		}
	}
	else if (m_isVm) {
		MPointArray points;
		fnMesh.getPoints(points);
		for (int i = 0; i < m_vmMoveIndices.length(); i++) {
			points[m_vmMoveIndices[i]] = m_vmPointsAfter[i];
		}
		fnMesh.setPoints(points);
		pMesh->updateVertexAndNormals();
		pMesh->updateBaseMesh();
	}
	else if (m_isDel) {
		SGCommand::deleteComponent(m_dagPathMesh, m_delEdgeIndices, m_delPolyIndices);
		fnMesh.updateSurface();
		active3dView.refresh(false, true);
		pMesh->update(SGOption::option.symInfo);
	}
	else if (m_isUpm) {
		active3dView.refresh(false, true);
		pMesh->update(SGOption::option.symInfo);
	}
	else if (m_isUpc) {
		active3dView.refresh(false, true);
		pMesh->updateVertexAndNormals();
		pMesh->updateBaseMesh();
	}

	transManip.build();
	return MS::kSuccess;
}


MStatus SGCommand::undoIt() {

	SGMesh* pMesh = SGMesh::pMesh;
	M3dView active3dView = M3dView().active3dView();
	MFnMesh fnMesh = m_dagPathMesh;

	if (m_isSt) {
		if(m_stResults.length())
			MGlobal::executeCommand(MString("setToolTo ") + m_stResults[0] );
		return MS::kSuccess;
	}
	else if (m_isVm) {
		MPointArray points;
		fnMesh.getPoints(points);
		for (int i = 0; i < m_vmMoveIndices.length(); i++) {
			points[m_vmMoveIndices[i]] = m_vmPointsBefore[i];
		}
		fnMesh.setPoints(points);
		pMesh->updateVertexAndNormals();
		pMesh->updateBaseMesh();
	}
	else if (m_isDel) {
		if (m_delEdgeIndices.length() || m_delPolyIndices.length()) {
			if (m_delPolyIndices.length())SGNodeControl::deleteBeforeNode(m_dagPathMesh, "deleteComponent", "inputGeometry");
			else if (m_delEdgeIndices.length())SGNodeControl::deleteBeforeNode(m_dagPathMesh, "polyDelEdge", "inputPolymesh");
			fnMesh.updateSurface();
			active3dView.refresh(false, true);
			pMesh->update(SGOption::option.symInfo);
		}
	}
	else if (m_isUpm) {
		active3dView.refresh(false, true);
		pMesh->update(SGOption::option.symInfo);
	}
	else if (m_isUpc) {
		active3dView.refresh(false, true);
		pMesh->updateVertexAndNormals();
		pMesh->updateBaseMesh();
	}

	transManip.build();
	return MS::kSuccess;
}


bool SGCommand::isUndoable() const {
	return m_undoableValue;
}