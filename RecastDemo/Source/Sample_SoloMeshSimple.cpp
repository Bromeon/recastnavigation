//
// Copyright (c) 2009 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"
#include "SDL_opengl.h"
#include "imgui.h"
#include "InputGeom.h"
#include "Sample.h"
#include "Sample_SoloMeshSimple.h"
#include "Recast.h"
#include "RecastTimer.h"
#include "RecastDebugDraw.h"
#include "RecastDump.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourDebugDraw.h"
#include "NavMeshTesterTool.h"
#include "ExtraLinkTool.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif


Sample_SoloMeshSimple::Sample_SoloMeshSimple() :
	m_keepInterResults(false),
	m_triflags(0),
	m_solid(0),
	m_chf(0),
	m_cset(0),
	m_pmesh(0),
	m_dmesh(0),
	m_drawMode(DRAWMODE_NAVMESH)
{
	setTool(new NavMeshTesterTool);
}
		
Sample_SoloMeshSimple::~Sample_SoloMeshSimple()
{
	cleanup();
}
	
void Sample_SoloMeshSimple::cleanup()
{
	delete [] m_triflags;
	m_triflags = 0;
	delete m_solid;
	m_solid = 0;
	delete m_chf;
	m_chf = 0;
	delete m_cset;
	m_cset = 0;
	delete m_pmesh;
	m_pmesh = 0;
	delete m_dmesh;
	m_dmesh = 0;
	delete m_navMesh;
	m_navMesh = 0;
}
			
void Sample_SoloMeshSimple::handleSettings()
{
	Sample::handleCommonSettings();
	
	if (imguiCheck("Keep Itermediate Results", m_keepInterResults))
		m_keepInterResults = !m_keepInterResults;

	imguiSeparator();
}

void Sample_SoloMeshSimple::handleTools()
{
	int type = !m_tool ? TOOL_NONE : m_tool->type();
	
	if (imguiCheck("Test Navmesh", type == TOOL_NAVMESH_TESTER))
	{
		setTool(new NavMeshTesterTool);
	}
	if (imguiCheck("Create Extra Links", type == TOOL_EXTRA_LINK))
	{
		setTool(new ExtraLinkTool);
	}
	
	imguiSeparator();

	if (m_tool)
		m_tool->handleMenu();
}

void Sample_SoloMeshSimple::handleDebugMode()
{
	// Check which modes are valid.
	bool valid[MAX_DRAWMODE];
	for (int i = 0; i < MAX_DRAWMODE; ++i)
		valid[i] = false;

	if (m_geom)
	{
		valid[DRAWMODE_NAVMESH] = m_navMesh != 0;
		valid[DRAWMODE_NAVMESH_TRANS] = m_navMesh != 0;
		valid[DRAWMODE_NAVMESH_BVTREE] = m_navMesh != 0;
		valid[DRAWMODE_NAVMESH_INVIS] = m_navMesh != 0;
		valid[DRAWMODE_MESH] = true;
		valid[DRAWMODE_VOXELS] = m_solid != 0;
		valid[DRAWMODE_VOXELS_WALKABLE] = m_solid != 0;
		valid[DRAWMODE_COMPACT] = m_chf != 0;
		valid[DRAWMODE_COMPACT_DISTANCE] = m_chf != 0;
		valid[DRAWMODE_COMPACT_REGIONS] = m_chf != 0;
		valid[DRAWMODE_REGION_CONNECTIONS] = m_cset != 0;
		valid[DRAWMODE_RAW_CONTOURS] = m_cset != 0;
		valid[DRAWMODE_BOTH_CONTOURS] = m_cset != 0;
		valid[DRAWMODE_CONTOURS] = m_cset != 0;
		valid[DRAWMODE_POLYMESH] = m_pmesh != 0;
		valid[DRAWMODE_POLYMESH_DETAIL] = m_dmesh != 0;
	}
	
	int unavail = 0;
	for (int i = 0; i < MAX_DRAWMODE; ++i)
		if (!valid[i]) unavail++;

	if (unavail == MAX_DRAWMODE)
		return;

	imguiLabel("Draw");
	if (imguiCheck("Input Mesh", m_drawMode == DRAWMODE_MESH, valid[DRAWMODE_MESH]))
		m_drawMode = DRAWMODE_MESH;
	if (imguiCheck("Navmesh", m_drawMode == DRAWMODE_NAVMESH, valid[DRAWMODE_NAVMESH]))
		m_drawMode = DRAWMODE_NAVMESH;
	if (imguiCheck("Navmesh Invis", m_drawMode == DRAWMODE_NAVMESH_INVIS, valid[DRAWMODE_NAVMESH_INVIS]))
		m_drawMode = DRAWMODE_NAVMESH_INVIS;
	if (imguiCheck("Navmesh Trans", m_drawMode == DRAWMODE_NAVMESH_TRANS, valid[DRAWMODE_NAVMESH_TRANS]))
		m_drawMode = DRAWMODE_NAVMESH_TRANS;
	if (imguiCheck("Navmesh BVTree", m_drawMode == DRAWMODE_NAVMESH_BVTREE, valid[DRAWMODE_NAVMESH_BVTREE]))
		m_drawMode = DRAWMODE_NAVMESH_BVTREE;
	if (imguiCheck("Voxels", m_drawMode == DRAWMODE_VOXELS, valid[DRAWMODE_VOXELS]))
		m_drawMode = DRAWMODE_VOXELS;
	if (imguiCheck("Walkable Voxels", m_drawMode == DRAWMODE_VOXELS_WALKABLE, valid[DRAWMODE_VOXELS_WALKABLE]))
		m_drawMode = DRAWMODE_VOXELS_WALKABLE;
	if (imguiCheck("Compact", m_drawMode == DRAWMODE_COMPACT, valid[DRAWMODE_COMPACT]))
		m_drawMode = DRAWMODE_COMPACT;
	if (imguiCheck("Compact Distance", m_drawMode == DRAWMODE_COMPACT_DISTANCE, valid[DRAWMODE_COMPACT_DISTANCE]))
		m_drawMode = DRAWMODE_COMPACT_DISTANCE;
	if (imguiCheck("Compact Regions", m_drawMode == DRAWMODE_COMPACT_REGIONS, valid[DRAWMODE_COMPACT_REGIONS]))
		m_drawMode = DRAWMODE_COMPACT_REGIONS;
	if (imguiCheck("Region Connections", m_drawMode == DRAWMODE_REGION_CONNECTIONS, valid[DRAWMODE_REGION_CONNECTIONS]))
		m_drawMode = DRAWMODE_REGION_CONNECTIONS;
	if (imguiCheck("Raw Contours", m_drawMode == DRAWMODE_RAW_CONTOURS, valid[DRAWMODE_RAW_CONTOURS]))
		m_drawMode = DRAWMODE_RAW_CONTOURS;
	if (imguiCheck("Both Contours", m_drawMode == DRAWMODE_BOTH_CONTOURS, valid[DRAWMODE_BOTH_CONTOURS]))
		m_drawMode = DRAWMODE_BOTH_CONTOURS;
	if (imguiCheck("Contours", m_drawMode == DRAWMODE_CONTOURS, valid[DRAWMODE_CONTOURS]))
		m_drawMode = DRAWMODE_CONTOURS;
	if (imguiCheck("Poly Mesh", m_drawMode == DRAWMODE_POLYMESH, valid[DRAWMODE_POLYMESH]))
		m_drawMode = DRAWMODE_POLYMESH;
	if (imguiCheck("Poly Mesh Detail", m_drawMode == DRAWMODE_POLYMESH_DETAIL, valid[DRAWMODE_POLYMESH_DETAIL]))
		m_drawMode = DRAWMODE_POLYMESH_DETAIL;
		
	if (unavail)
	{
		imguiValue("Tick 'Keep Itermediate Results'");
		imguiValue("to see more debug mode options.");
	}
}

void Sample_SoloMeshSimple::handleRender()
{
	if (!m_geom || !m_geom->getMesh())
		return;
	
	float col[4];

	DebugDrawGL dd;
	
	glEnable(GL_FOG);
	glDepthMask(GL_TRUE);

	if (m_drawMode == DRAWMODE_MESH)
	{
		// Draw mesh
		duDebugDrawTriMeshSlope(&dd, m_geom->getMesh()->getVerts(), m_geom->getMesh()->getVertCount(),
								m_geom->getMesh()->getTris(), m_geom->getMesh()->getNormals(), m_geom->getMesh()->getTriCount(),
								m_agentMaxSlope);
	}
	else if (m_drawMode != DRAWMODE_NAVMESH_TRANS)
	{
		// Draw mesh
		duDebugDrawTriMesh(&dd, m_geom->getMesh()->getVerts(), m_geom->getMesh()->getVertCount(),
						   m_geom->getMesh()->getTris(), m_geom->getMesh()->getNormals(), m_geom->getMesh()->getTriCount(), 0);
	}
	
	glDisable(GL_FOG);
	glDepthMask(GL_FALSE);

	// Draw bounds
	const float* bmin = m_geom->getMeshBoundsMin();
	const float* bmax = m_geom->getMeshBoundsMax();
	col[0] = 1; col[1] = 1; col[2] = 1; col[3] = 0.5f;
	duDebugDrawBoxWire(&dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], col);
	
	if (m_navMesh &&
		(m_drawMode == DRAWMODE_NAVMESH ||
		m_drawMode == DRAWMODE_NAVMESH_TRANS ||
		m_drawMode == DRAWMODE_NAVMESH_BVTREE ||
		m_drawMode == DRAWMODE_NAVMESH_INVIS))
	{
		if (m_drawMode != DRAWMODE_NAVMESH_INVIS)
			duDebugDrawNavMesh(&dd, m_navMesh); //, m_toolMode == TOOLMODE_PATHFIND);
		if (m_drawMode == DRAWMODE_NAVMESH_BVTREE)
			duDebugDrawNavMeshBVTree(&dd, m_navMesh);
	}
		
	glDepthMask(GL_TRUE);
	
	if (m_chf && m_drawMode == DRAWMODE_COMPACT)
		duDebugDrawCompactHeightfieldSolid(&dd, *m_chf);

	if (m_chf && m_drawMode == DRAWMODE_COMPACT_DISTANCE)
		duDebugDrawCompactHeightfieldDistance(&dd, *m_chf);
	if (m_chf && m_drawMode == DRAWMODE_COMPACT_REGIONS)
		duDebugDrawCompactHeightfieldRegions(&dd, *m_chf);
	if (m_solid && m_drawMode == DRAWMODE_VOXELS)
	{
		glEnable(GL_FOG);
		duDebugDrawHeightfieldSolid(&dd, *m_solid);
		glDisable(GL_FOG);
	}
	if (m_solid && m_drawMode == DRAWMODE_VOXELS_WALKABLE)
	{
		glEnable(GL_FOG);
		duDebugDrawHeightfieldWalkable(&dd, *m_solid);
		glDisable(GL_FOG);
	}
	if (m_cset && m_drawMode == DRAWMODE_RAW_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawRawContours(&dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_cset && m_drawMode == DRAWMODE_BOTH_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawRawContours(&dd, *m_cset, 0.5f);
		duDebugDrawContours(&dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_cset && m_drawMode == DRAWMODE_CONTOURS)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawContours(&dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_chf && m_cset && m_drawMode == DRAWMODE_REGION_CONNECTIONS)
	{
		duDebugDrawCompactHeightfieldRegions(&dd, *m_chf);
			
		glDepthMask(GL_FALSE);
		duDebugDrawRegionConnections(&dd, *m_cset);
		glDepthMask(GL_TRUE);
	}
	if (m_pmesh && m_drawMode == DRAWMODE_POLYMESH)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawPolyMesh(&dd, *m_pmesh);
		glDepthMask(GL_TRUE);
	}
	if (m_dmesh && m_drawMode == DRAWMODE_POLYMESH_DETAIL)
	{
		glDepthMask(GL_FALSE);
		duDebugDrawPolyMeshDetail(&dd, *m_dmesh);
		glDepthMask(GL_TRUE);
	}
	
/*	static const float startCol[4] = { 0.5f, 0.1f, 0.0f, 0.75f };
	static const float endCol[4] = { 0.2f, 0.4f, 0.0f, 0.75f };
	if (m_sposSet)
		drawAgent(m_spos, m_agentRadius, m_agentHeight, m_agentMaxClimb, startCol);
	if (m_eposSet)
		drawAgent(m_epos, m_agentRadius, m_agentHeight, m_agentMaxClimb, endCol);*/

	if (m_tool)
		m_tool->handleRender();

	glDepthMask(GL_TRUE);
}

void Sample_SoloMeshSimple::handleRenderOverlay(double* proj, double* model, int* view)
{
	if (m_tool)
		m_tool->handleRenderOverlay(proj, model, view);
}

void Sample_SoloMeshSimple::handleMeshChanged(class InputGeom* geom)
{
	Sample::handleMeshChanged(geom);
	delete m_navMesh;
	m_navMesh = 0;
	if (m_tool)
	{
		m_tool->reset();
		m_tool->init(this);
	}
}

bool Sample_SoloMeshSimple::handleBuild()
{
	if (!m_geom || !m_geom->getMesh())
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Input mesh is not specified.");
		return false;
	}
	
	cleanup();
	
	const float* bmin = m_geom->getMeshBoundsMin();
	const float* bmax = m_geom->getMeshBoundsMin();
	const float* verts = m_geom->getMesh()->getVerts();
	const int nverts = m_geom->getMesh()->getVertCount();
	const int* tris = m_geom->getMesh()->getTris();
	const int ntris = m_geom->getMesh()->getTriCount();
	
	//
	// Step 1. Initialize build config.
	//
	
	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = m_cellSize;
	m_cfg.ch = m_cellHeight;
	m_cfg.walkableSlopeAngle = m_agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)ceilf(m_agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
	m_cfg.maxSimplificationError = m_edgeMaxError;
	m_cfg.minRegionSize = (int)rcSqr(m_regionMinSize);
	m_cfg.mergeRegionSize = (int)rcSqr(m_regionMergeSize);
	m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
	m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
	m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;
	
	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	vcopy(m_cfg.bmin, bmin);
	vcopy(m_cfg.bmax, bmax);
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	// Reset build times gathering.
	memset(&m_buildTimes, 0, sizeof(m_buildTimes));
	rcSetBuildTimes(&m_buildTimes);

	// Start the build process.	
	rcTimeVal totStartTime = rcGetPerformanceTimer();
	
	if (rcGetLog())
	{
		rcGetLog()->log(RC_LOG_PROGRESS, "Building navigation:");
		rcGetLog()->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
		rcGetLog()->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts/1000.0f, ntris/1000.0f);
	}
	
	//
	// Step 2. Rasterize input polygon soup.
	//
	
	// Allocate voxel heighfield where we rasterize our input data to.
	m_solid = new rcHeightfield;
	if (!m_solid)
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if (!rcCreateHeightfield(*m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		return false;
	}
	
	// Allocate array that can hold triangle flags.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	m_triflags = new unsigned char[ntris];
	if (!m_triflags)
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'triangleFlags' (%d).", ntris);
		return false;
	}
	
	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the flags for each of the meshes and rasterize them.
	memset(m_triflags, 0, ntris*sizeof(unsigned char));
	rcMarkWalkableTriangles(m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, m_triflags);
	rcRasterizeTriangles(verts, nverts, tris, m_triflags, ntris, *m_solid);

	if (!m_keepInterResults)
	{
		delete [] m_triflags;
		m_triflags = 0;
	}
	
	//
	// Step 3. Filter walkables surfaces.
	//
	
	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLedgeSpans(m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	rcFilterWalkableLowHeightSpans(m_cfg.walkableHeight, *m_solid);

	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = new rcCompactHeightfield;
	if (!m_chf)
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if (!rcBuildCompactHeightfield(m_cfg.walkableHeight, m_cfg.walkableClimb, RC_WALKABLE, *m_solid, *m_chf))
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		return false;
	}
	
	if (!m_keepInterResults)
	{
		delete m_solid;
		m_solid = 0;
	}
	
	// Prepare for region partitioning, by calculating distance field along the walkable surface.
	if (!rcBuildDistanceField(*m_chf))
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
		return false;
	}

	// Partition the walkable surface into simple regions without holes.
	if (!rcBuildRegions(*m_chf, m_cfg.walkableRadius, m_cfg.borderSize, m_cfg.minRegionSize, m_cfg.mergeRegionSize))
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Could not build regions.");
	}
	
	//
	// Step 5. Trace and simplify region contours.
	//
	
	// Create contours.
	m_cset = new rcContourSet;
	if (!m_cset)
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if (!rcBuildContours(*m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		return false;
	}
	
	//
	// Step 6. Build polygons mesh from contours.
	//
	
	// Build polygon navmesh from the contours.
	m_pmesh = new rcPolyMesh;
	if (!m_pmesh)
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if (!rcBuildPolyMesh(*m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		return false;
	}
	
	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//
	
	m_dmesh = new rcPolyMeshDetail;
	if (!m_dmesh)
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if (!rcBuildPolyMeshDetail(*m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		if (rcGetLog())
			rcGetLog()->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
	}

	if (!m_keepInterResults)
	{
		delete m_chf;
		m_chf = 0;
		delete m_cset;
		m_cset = 0;
	}

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.
	
	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//
	
	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		unsigned char* navData = 0;
		int navDataSize = 0;
		if (!dtCreateNavMeshData(m_pmesh->verts, m_pmesh->nverts,
								 m_pmesh->polys, m_pmesh->npolys, m_pmesh->nvp,
								 m_dmesh->meshes, m_dmesh->verts, m_dmesh->nverts,
								 m_dmesh->tris, m_dmesh->ntris, 
								 m_pmesh->bmin, m_pmesh->bmax, m_cfg.cs, m_cfg.ch, 0, m_cfg.walkableClimb,
								 &navData, &navDataSize))
		{
			if (rcGetLog())
				rcGetLog()->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			return false;
		}
		
		m_navMesh = new dtNavMesh;
		if (!m_navMesh)
		{
			delete [] navData;
			if (rcGetLog())
				rcGetLog()->log(RC_LOG_ERROR, "Could not create Detour navmesh");
			return false;
		}
		
		if (!m_navMesh->init(navData, navDataSize, true, 2048))
		{
			delete [] navData;
			if (rcGetLog())
				rcGetLog()->log(RC_LOG_ERROR, "Could not init Detour navmesh");
			return false;
		}
	}
	
	rcTimeVal totEndTime = rcGetPerformanceTimer();

	// Show performance stats.
	if (rcGetLog())
	{
		const float pc = 100.0f / rcGetDeltaTimeUsec(totStartTime, totEndTime);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Rasterize: %.1fms (%.1f%%)", m_buildTimes.rasterizeTriangles/1000.0f, m_buildTimes.rasterizeTriangles*pc);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Build Compact: %.1fms (%.1f%%)", m_buildTimes.buildCompact/1000.0f, m_buildTimes.buildCompact*pc);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Filter Border: %.1fms (%.1f%%)", m_buildTimes.filterBorder/1000.0f, m_buildTimes.filterBorder*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "Filter Walkable: %.1fms (%.1f%%)", m_buildTimes.filterWalkable/1000.0f, m_buildTimes.filterWalkable*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "Filter Reachable: %.1fms (%.1f%%)", m_buildTimes.filterMarkReachable/1000.0f, m_buildTimes.filterMarkReachable*pc);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Build Distancefield: %.1fms (%.1f%%)", m_buildTimes.buildDistanceField/1000.0f, m_buildTimes.buildDistanceField*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "  - distance: %.1fms (%.1f%%)", m_buildTimes.buildDistanceFieldDist/1000.0f, m_buildTimes.buildDistanceFieldDist*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "  - blur: %.1fms (%.1f%%)", m_buildTimes.buildDistanceFieldBlur/1000.0f, m_buildTimes.buildDistanceFieldBlur*pc);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Build Regions: %.1fms (%.1f%%)", m_buildTimes.buildRegions/1000.0f, m_buildTimes.buildRegions*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "  - watershed: %.1fms (%.1f%%)", m_buildTimes.buildRegionsReg/1000.0f, m_buildTimes.buildRegionsReg*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "    - expand: %.1fms (%.1f%%)", m_buildTimes.buildRegionsExp/1000.0f, m_buildTimes.buildRegionsExp*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "    - find catchment basins: %.1fms (%.1f%%)", m_buildTimes.buildRegionsFlood/1000.0f, m_buildTimes.buildRegionsFlood*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "  - filter: %.1fms (%.1f%%)", m_buildTimes.buildRegionsFilter/1000.0f, m_buildTimes.buildRegionsFilter*pc);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Build Contours: %.1fms (%.1f%%)", m_buildTimes.buildContours/1000.0f, m_buildTimes.buildContours*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "  - trace: %.1fms (%.1f%%)", m_buildTimes.buildContoursTrace/1000.0f, m_buildTimes.buildContoursTrace*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "  - simplify: %.1fms (%.1f%%)", m_buildTimes.buildContoursSimplify/1000.0f, m_buildTimes.buildContoursSimplify*pc);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Build Polymesh: %.1fms (%.1f%%)", m_buildTimes.buildPolymesh/1000.0f, m_buildTimes.buildPolymesh*pc);
		rcGetLog()->log(RC_LOG_PROGRESS, "Build Polymesh Detail: %.1fms (%.1f%%)", m_buildTimes.buildDetailMesh/1000.0f, m_buildTimes.buildDetailMesh*pc);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "Polymesh: Verts:%d  Polys:%d", m_pmesh->nverts, m_pmesh->npolys);
		
		rcGetLog()->log(RC_LOG_PROGRESS, "TOTAL: %.1fms", rcGetDeltaTimeUsec(totStartTime, totEndTime)/1000.0f);
	}
	
	if (m_tool)
		m_tool->init(this);

	return true;
}