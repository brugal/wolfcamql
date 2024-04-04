/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "tr_local.h"
#include "tr_fbo.h"
#include "tr_dsa.h"

backEndData_t	*backEndData;
backEndState_t	backEnd;


static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};

void RB_SetGL2D (void);

/*
** GL_BindToTMU
*/
void GL_BindToTMU( image_t *image, int tmu )
{
	GLuint texture = (tmu == TB_COLORMAP) ? tr.defaultImage->texnum : 0;
	GLenum target = GL_TEXTURE_2D;

	if (image)
	{
		if (image->flags & IMGFLAG_CUBEMAP)
			target = GL_TEXTURE_CUBE_MAP;

		image->frameUsed = tr.frameCount;
		texture = image->texnum;
	}
	else
	{
		ri.Printf(PRINT_WARNING, "GL_BindToTMU: NULL image\n");
	}

	GL_BindMultiTexture(GL_TEXTURE0 + tmu, target, texture);
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	if ( cullType == CT_TWO_SIDED ) 
	{
		qglDisable( GL_CULL_FACE );
	} 
	else 
	{
		qboolean cullFront = (cullType == CT_FRONT_SIDED);

		if ( glState.faceCulling == CT_TWO_SIDED )
			qglEnable( GL_CULL_FACE );

		if ( glState.faceCullFront != cullFront )
			qglCullFace( cullFront ? GL_FRONT : GL_BACK );

		glState.faceCullFront = cullFront;
	}

	glState.faceCulling = cullType;
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( unsigned long stateBits )
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_BITS )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else if ( stateBits & GLS_DEPTHFUNC_GREATER)
		{
			qglDepthFunc( GL_GREATER );
		}
		else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		uint32_t oldState = glState.glStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
		uint32_t newState = stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
		uint32_t storedState = glState.storedGlState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );

		if (oldState == 0)
		{
			qglEnable( GL_BLEND );
		}
		else if (newState == 0)
		{
			qglDisable( GL_BLEND );
		}

		if (newState != 0 && storedState != newState)
		{
			GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

			glState.storedGlState &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
			glState.storedGlState |= newState;

			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid src blend state bits" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
				break;
			}

			qglBlendFunc( srcFactor, dstFactor );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			qglDepthMask( GL_TRUE );
		}
		else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			qglDisable( GL_DEPTH_TEST );
		}
		else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	glState.glStateBits = stateBits;
}


void GL_SetProjectionMatrix(mat4_t matrix)
{
	Mat4Copy(matrix, glState.projection);
	Mat4Multiply(glState.projection, glState.modelview, glState.modelviewProjection);	
}


void GL_SetModelviewMatrix(mat4_t matrix)
{
	Mat4Copy(matrix, glState.modelview);
	Mat4Multiply(glState.projection, glState.modelview, glState.modelviewProjection);	
}


/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	if (r_teleporterFlash->integer == 0) {
		c = 0;
	}
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );
	qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	backEnd.isHyperspace = qtrue;
}


static void SetViewportAndScissor( void ) {
	GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView (void) {
	int clearBits = 0;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish ();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}

	if (tr.worldMapLoaded) {
		if (r_cloudHeight->modified) {
			if (*r_cloudHeight->string) {
				R_InitSkyTexCoords(r_cloudHeight->value);
			} else {
				if (r_cloudHeightOrig) {
					R_InitSkyTexCoords(r_cloudHeightOrig->value);
				} else {
					R_InitSkyTexCoords(256);
				}
			}
			r_cloudHeight->modified = qfalse;
		}

		if (r_singleShaderName->modified) {
			R_CreateSingleShader();
			r_singleShaderName->modified = qfalse;
		}
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	if (glRefConfig.framebufferObject)
	{
		FBO_t *fbo = backEnd.viewParms.targetFbo;

		//if (fbo == NULL && (!r_postProcess->integer || !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL)))
		if (fbo == NULL)
			fbo = tr.renderFbo;

		if (tr.usingFinalFrameBufferObject  &&  fbo == NULL) {
			fbo = tr.finalFbo;
		}

		if (tr.renderCubeFbo && fbo == tr.renderCubeFbo)
		{
			cubemap_t *cubemap = &tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex];
			FBO_AttachImage(fbo, cubemap->image, GL_COLOR_ATTACHMENT0_EXT, backEnd.viewParms.targetFboLayer);
		}

		FBO_Bind(fbo);
	}

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );
	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_measureOverdraw->integer || r_shadows->integer == 2 )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	if ( r_fastsky->integer == 1  && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
		if (*r_fastSkyColor->string) {
			int v, tr, tg, tb;

			v = r_fastSkyColor->integer;
			tr = (v & 0xff0000) / 0x010000;
			tg = (v & 0x00ff00) / 0x000100;
			tb = (v & 0x0000ff) / 0x000001;

			qglClearColor((float)tr / 255.0, (float)tg / 255.0, (float)tb / 255.0, 1.0);
		} else {
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );        // FIXME: get color of sky
		}
	}

	// clear to black for cube maps
	if (tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
	}

	qglClear( clearBits );

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
#if 0
		float	plane[4];
		GLdouble	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.or.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.or.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.or.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.or.origin) - plane[3];
#endif
		GL_SetModelviewMatrix( s_flipMatrix );
	}
}

// reset to NULL when vid restart
static cameraPoint_t *PLcameraPoints = NULL;
static int *PLnumCameraPoints = NULL;
static vec3_t *PLsplinePoints = NULL;
static int *PLnumSplinePoints = NULL;
static vec4_t PLcolor;

void RE_SetPathLines (int *numCameraPoints, cameraPoint_t *cameraPoints, int *numSplinePoints, vec3_t *splinePoints, const vec4_t color)
{
	PLnumCameraPoints = numCameraPoints;
	PLcameraPoints = cameraPoints;
	PLnumSplinePoints = numSplinePoints;
	PLsplinePoints = splinePoints;
	Vector4Copy(color, PLcolor);
}

static void RE_DrawPathLines (void)
{
	int	i;
	int j;
	cameraPoint_t *cp, *cpnext;
	int pcvar;
	GLint loc;
	vec4_t color;

#if 0
	if (!PLnumSplinePoints  ||  *PLnumSplinePoints <= 0) {
		return;
	}
	if (!PLsplinePoints) {
		return;
	}
#endif

	if (!tr.cpshaderprogram) {
		return;
	}

	if (!PLnumCameraPoints  ||  *PLnumCameraPoints < 2) {
		return;
	}

	pcvar = ri.Cvar_VariableIntegerValue("cg_drawCameraPath");
	if (!pcvar) {
		return;
	}

	R_BindVao(tess.vao);

#if 0
	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
#endif

	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);

	//qglDepthRange(0, 1);	// occluded
	GL_UseProgram(tr.cpshaderprogram);

	//qglBindVertexArray(tr.cpvao);
	qglBindBuffer(GL_ARRAY_BUFFER, tr.cpvbo);


	qglEnableVertexAttribArray(0);
	/* attrib index 0, three floats per vertex */
	qglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	loc = qglGetUniformLocation(tr.cpshaderprogram, "u_ModelViewProjectionMatrix");
	qglUniformMatrix4fv(loc, 1 /*only setting 1 matrix*/, GL_FALSE /*transpose?*/, glState.modelviewProjection);


	//Vector4Set(color, 0.0f, 0.2f, 0.7f, 1.0f);

	loc = qglGetUniformLocation(tr.cpshaderprogram, "u_Color");
	//qglUniform4f(loc, color[0], color[1], color[2], color[3]);

	//qglDrawArrays(GL_LINES, 0, 3);
	//qglDrawArrays(GL_TRIANGLES, 0, 3);
	//qglDrawArrays(GL_LINE_STRIP, 0, 3);


	Vector4Set(color, PLcolor[0], PLcolor[1], PLcolor[2], 1.0f);
	qglBindBuffer(GL_ARRAY_BUFFER, tr.cpvbo);

	for (i = 0;  i < *PLnumCameraPoints - 1;  i++) {
		int n = 0;

		cp = &PLcameraPoints[i];
		if (!(cp->flags & CAM_ORIGIN)) {
			continue;
		}
		cpnext = cp->next;
		while (cpnext  &&  !(cpnext->flags & CAM_ORIGIN)) {
			cpnext = cpnext->next;
		}
		if (!cpnext) {
			continue;
		}
		if (cp->type == CAMERA_SPLINE  ||  cp->type == CAMERA_SPLINE_BEZIER  ||  cp->type == CAMERA_SPLINE_CATMULLROM  ||  (cp->type == CAMERA_CURVE  &&  cp->hasQuadratic)) {
			Vector4Set(color, PLcolor[0], PLcolor[1], PLcolor[2], 1.0f);
			if (cp->type == CAMERA_CURVE) {
				Vector4Set(color, 1.0f, 0.3f, 0.3f, 1.0f);
			} else if (cp->type == CAMERA_SPLINE_BEZIER) {
				Vector4Set(color, 0.3f, 1.0f, 0.3f, 1.0f);
			} else if (cp->type == CAMERA_SPLINE_CATMULLROM) {
				Vector4Set(color, 0.3f, 0.3f, 1.0f, 1.0f);
			} else if (cp->type == CAMERA_SPLINE) {
				Vector4Set(color, 1.0f, 0.7f, 0.1f, 1.0f);
			}

			for (j = cp->splineStart;  j < cpnext->splineStart;  j++) {
				tr.cpverts[n*3 + 0] = PLsplinePoints[j][0];
				tr.cpverts[n*3 + 1] = PLsplinePoints[j][1];
				tr.cpverts[n*3 + 2] = PLsplinePoints[j][2];
				n++;
			}
		} else if (cp->type == CAMERA_INTERP  ||  cp->type == CAMERA_CURVE) {  //FIXME curve
			Vector4Set(color, 0.5f, 0.5f, 1.0f, 1.0f);

			tr.cpverts[n*3 + 0] = cp->origin[0];
			tr.cpverts[n*3 + 1] = cp->origin[1];
			tr.cpverts[n*3 + 2] = cp->origin[2];
			n++;
			if (cpnext->type == CAMERA_SPLINE) {
				tr.cpverts[n*3 + 0] = PLsplinePoints[cpnext->splineStart][0];
				tr.cpverts[n*3 + 1] = PLsplinePoints[cpnext->splineStart][1];
				tr.cpverts[n*3 + 2] = PLsplinePoints[cpnext->splineStart][2];
			} else {
				tr.cpverts[n*3 + 0] = cpnext->origin[0];
				tr.cpverts[n*3 + 1] = cpnext->origin[1];
				tr.cpverts[n*3 + 2] = cpnext->origin[2];
			}
			n++;
		}

		qglUniform4f(loc, color[0], color[1], color[2], color[3]);
		qglBufferSubData(GL_ARRAY_BUFFER, 0, 3 * n * sizeof(float), tr.cpverts);
		qglDrawArrays(GL_LINE_STRIP, 0, n);
	}

	if (pcvar == 2) {
		int n;

		n = 0;
		for (i = 0;  i < *PLnumCameraPoints;  i++) {
			vec3_t origin;

			cp = &PLcameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}

			VectorCopy(PLsplinePoints[cp->splineStart], origin);
			origin[2] += 10;
			tr.cpverts[n*3 + 0] = origin[0];
			tr.cpverts[n*3 + 1] = origin[1];
			tr.cpverts[n*3 + 2] = origin[2];
			n++;

			origin[2] -= 20;
			tr.cpverts[n*3 + 0] = origin[0];
			tr.cpverts[n*3 + 1] = origin[1];
			tr.cpverts[n*3 + 2] = origin[2];
			n++;
		}

		Vector4Set(color, PLcolor[0], PLcolor[1], PLcolor[2], 1.0f);
		qglUniform4f(loc, color[0], color[1], color[2], color[3]);
		qglBufferSubData(GL_ARRAY_BUFFER, 0, 3 * n * sizeof(float), tr.cpverts);
		qglDrawArrays(GL_LINES, 0, n);

		n = 0;
		for (i = 0;  i < *PLnumCameraPoints;  i++) {
			vec3_t origin;

			cp = &PLcameraPoints[i];
			if (!(cp->flags & CAM_ORIGIN)) {
				continue;
			}

			VectorCopy(cp->origin, origin);
			origin[2] += 10;
			tr.cpverts[n*3 + 0] = origin[0];
			tr.cpverts[n*3 + 1] = origin[1];
			tr.cpverts[n*3 + 2] = origin[2];
			n++;

			origin[2] -= 20;
			tr.cpverts[n*3 + 0] = origin[0];
			tr.cpverts[n*3 + 1] = origin[1];
			tr.cpverts[n*3 + 2] = origin[2];
			n++;
		}

		Vector4Set(color, 0.5f, 0.5f, 1.0f, 1.0f);
		qglUniform4f(loc, color[0], color[1], color[2], color[3]);
		qglBufferSubData(GL_ARRAY_BUFFER, 0, 3 * n * sizeof(float), tr.cpverts);
		qglDrawArrays(GL_LINES, 0, n);
	}

	qglBindBuffer(GL_ARRAY_BUFFER, glState.currentVao->vertexesVBO);

	{
		uint32_t attribBit = 1 << 0;

		//if (glState.currentVao->attribs[0].enabled) {
		if (glState.vertexAttribsEnabled & attribBit) {
			vaoAttrib_t *vAtb = &glState.currentVao->attribs[0];

			qglVertexAttribPointer(0, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));
			qglEnableVertexAttribArray(0);
		} else {
			qglDisableVertexAttribArray(0);
		}
	}

	GL_BindNullProgram();
	GL_BindNullTextures();
}


static void RB_QLBloomDownSample (void)
{
	GLenum target;
	int width, height;
	GLint loc;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	target = GL_TEXTURE_RECTANGLE_ARB;

	width = glConfig.vidWidth;
	height = glConfig.vidHeight;

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.backBufferTexture);

	qglCopyTexSubImage2D(target, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight);

	GL_BindMultiTexture(GL_TEXTURE0_ARB + 1, target, tr.bloomTexture);

#if 0
	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
#endif

	GL_State( GLS_DEPTHTEST_DISABLE);

	// cull
	qglDisable( GL_CULL_FACE );

	qglDepthMask(GL_FALSE);


	GL_UseProgram(tr.downSample1Sp);
	loc = qglGetUniformLocation(tr.downSample1Sp, "backBufferTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get backBufferTex", __FUNCTION__);
	}
	qglUniform1i(loc, 0);

	loc = qglGetUniformLocation(tr.downSample1Sp, "u_ModelViewProjectionMatrix");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get u_ModelViewProjectionMatrix", __FUNCTION__);
	}

	qglUniformMatrix4fv(loc, 1 /* only setting 1 matrix */, GL_FALSE /*transpose?*/, glState.modelviewProjection);

	//qglDisable(GL_BLEND);


	VectorSet4(quadVerts[0], 0.0, (float)tr.bloomHeight, 0, 0);
	VectorSet4(quadVerts[1], (float)tr.bloomWidth, (float)tr.bloomHeight, 0, 0);
	VectorSet4(quadVerts[2], (float)tr.bloomWidth, 0.0, 0, 0);
	VectorSet4(quadVerts[3], 0.0, 0.0, 0, 0);

	VectorSet2(texCoords[0], 0, 0);
	VectorSet2(texCoords[1], (float)width, 0);
	VectorSet2(texCoords[2], (float)width, (float)height);
	VectorSet2(texCoords[3], 0, height);

	// RB_InstantQuad2 with tex1 coords
	// --

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	VectorCopy4(quadVerts[0], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[0], tess.texCoords[tess.numVertexes]);
	VectorCopy2(texCoords[0], tess.lightCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[1], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[1], tess.texCoords[tess.numVertexes]);
	VectorCopy2(texCoords[1], tess.lightCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[2], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[2], tess.texCoords[tess.numVertexes]);
	VectorCopy2(texCoords[2], tess.lightCoords[tess.numVertexes]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[3], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[3], tess.texCoords[tess.numVertexes]);
	VectorCopy2(texCoords[3], tess.lightCoords[tess.numVertexes]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	RB_UpdateTessVao(ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD);

	R_DrawElements(tess.numIndexes, tess.firstIndex);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;

	// --

	qglFinish();

	// restore cull
	{
		int current = glState.faceCulling;

		glState.faceCulling = !current;
		GL_Cull(current);
	}

	GL_BindNullProgram();
	GL_BindNullTextures();
}

static void RB_QLBloomBrightness (void)
{
	GLenum target;
	int width, height;
	GLint loc;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	RB_SetGL2D();

 	qglFinish();

 	target = GL_TEXTURE_RECTANGLE_ARB;

 	width = tr.bloomWidth;
 	height = tr.bloomHeight;

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.bloomTexture);

	qglCopyTexSubImage2D(target, 0, 0, 0, 0, glConfig.vidHeight - height, width, height);

	GL_UseProgram(tr.brightPassSp);

	loc = qglGetUniformLocation(tr.brightPassSp, "backBufferTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get backBufferTex", __FUNCTION__);
	}
	qglUniform1i(loc, 0);

	loc = qglGetUniformLocation(tr.brightPassSp, "p_brightthreshold");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_brightthreshold", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)r_BloomBrightThreshold->value);

	loc = qglGetUniformLocation(tr.brightPassSp, "u_ModelViewProjectionMatrix");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get u_ModelViewProjectionMatrix", __FUNCTION__);
	}
	qglUniformMatrix4fv(loc, 1 /* only setting 1 matrix */, GL_FALSE /*transpose?*/, glState.modelviewProjection);


#if 0  // alpha will not work
	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
#endif
	GL_State(GLS_DEPTHTEST_DISABLE);

	// cull
	qglDisable( GL_CULL_FACE );

	qglDepthMask(GL_FALSE);

	//qglClearColor(0.0, 0.0, 0.0, 1.0);
	//qglClear(GL_COLOR_BUFFER_BIT);

	//width /= 2;
	//height /= 2;

	VectorSet4(quadVerts[0], 0.0, (float)height, 0, 0);
	VectorSet4(quadVerts[1], (float)width, (float)height, 0, 0);
	VectorSet4(quadVerts[2], (float)width, 0.0, 0, 0);
	VectorSet4(quadVerts[3], 0.0, 0.0, 0, 0);

	VectorSet2(texCoords[0], 0, 0);
	VectorSet2(texCoords[1], (float)width, 0);
	VectorSet2(texCoords[2], (float)width, (float)height);
	VectorSet2(texCoords[3], 0, height);

	RB_InstantQuad2(quadVerts, texCoords);

	qglFinish();

	// restore cull
	{
		int current = glState.faceCulling;

		glState.faceCulling = !current;
		GL_Cull(current);
	}

	GL_BindNullProgram();
	GL_BindNullTextures();

	//GL_CheckErrors();
}

static void RB_QLBloomBlurHorizontal (void)
{
	GLenum target;
	int width, height;
	GLint loc;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	RB_SetGL2D();

	qglFinish();

	target = GL_TEXTURE_RECTANGLE_ARB;

	width = tr.bloomWidth;
	height = tr.bloomHeight;

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.bloomTexture);

	qglCopyTexSubImage2D(target, 0, 0, 0, 0, glConfig.vidHeight - height, width, height);

	GL_UseProgram(tr.blurHorizSp);
	loc = qglGetUniformLocation(tr.blurHorizSp, "backBufferTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get backBufferTex", __FUNCTION__);
	}
	qglUniform1i(loc, 0);

	loc = qglGetUniformLocation(tr.blurHorizSp, "u_ModelViewProjectionMatrix");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get u_ModelViewProjectionMatrix", __FUNCTION__);
	}
	qglUniformMatrix4fv(loc, 1 /* only setting 1 matrix */, GL_FALSE /*transpose?*/, glState.modelviewProjection);


#if 0  // alpha will not work
	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
#endif
	GL_State(GLS_DEPTHTEST_DISABLE);

	// cull
	qglDisable( GL_CULL_FACE );

	qglDepthMask(GL_FALSE);

	//qglClearColor(0.0, 0.0, 0.0, 1.0);
	//qglClear(GL_COLOR_BUFFER_BIT);

	//width /= 2;
	//height /= 2;

	VectorSet4(quadVerts[0], 0.0, (float)height, 0, 0);
	VectorSet4(quadVerts[1], (float)width, (float)height, 0, 0);
	VectorSet4(quadVerts[2], (float)width, 0.0, 0, 0);
	VectorSet4(quadVerts[3], 0.0, 0.0, 0, 0);

	VectorSet2(texCoords[0], 0, 0);
	VectorSet2(texCoords[1], (float)width, 0);
	VectorSet2(texCoords[2], (float)width, (float)height);
	VectorSet2(texCoords[3], 0, height);

	RB_InstantQuad2(quadVerts, texCoords);

	qglFinish();

	// restore cull
	{
		int current = glState.faceCulling;

		glState.faceCulling = !current;
		GL_Cull(current);
	}

	GL_BindNullProgram();
	GL_BindNullTextures();
}

static void RB_QLBloomBlurVertical (void)
{
	GLenum target;
	int width, height;
	GLint loc;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	RB_SetGL2D();

	qglFinish();

	target = GL_TEXTURE_RECTANGLE_ARB;

	width = tr.bloomWidth;
	height = tr.bloomHeight;

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.bloomTexture);

	qglCopyTexSubImage2D(target, 0, 0, 0, 0, glConfig.vidHeight - height, width, height);

	GL_UseProgram(tr.blurVerticalSp);
	loc = qglGetUniformLocation(tr.blurVerticalSp, "backBufferTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get backBufferTex", __FUNCTION__);
	}
	qglUniform1i(loc, 0);

	loc = qglGetUniformLocation(tr.blurVerticalSp, "u_ModelViewProjectionMatrix");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get u_ModelViewProjectionMatrix", __FUNCTION__);
	}
	qglUniformMatrix4fv(loc, 1 /* only setting 1 matrix */, GL_FALSE /*transpose?*/, glState.modelviewProjection);


#if 0  // alpha will not work
	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
#endif
	GL_State(GLS_DEPTHTEST_DISABLE);

	// cull
	qglDisable( GL_CULL_FACE );

	qglDepthMask(GL_FALSE);

	//qglClearColor(0.0, 0.0, 0.0, 1.0);
	//qglClear(GL_COLOR_BUFFER_BIT);

	//width /= 2;
	//height /= 2;

	VectorSet4(quadVerts[0], 0.0, (float)height, 0, 0);
	VectorSet4(quadVerts[1], (float)width, (float)height, 0, 0);
	VectorSet4(quadVerts[2], (float)width, 0.0, 0, 0);
	VectorSet4(quadVerts[3], 0.0, 0.0, 0, 0);

	VectorSet2(texCoords[0], 0, 0);
	VectorSet2(texCoords[1], (float)width, 0);
	VectorSet2(texCoords[2], (float)width, (float)height);
	VectorSet2(texCoords[3], 0, height);

	RB_InstantQuad2(quadVerts, texCoords);

	qglFinish();

	// restore cull
	{
		int current = glState.faceCulling;

		glState.faceCulling = !current;
		GL_Cull(current);
	}

	GL_BindNullProgram();
	GL_BindNullTextures();
}

static void RB_QLBloomCombine (void)
{
	GLenum target;
	int width, height;
	GLint loc;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	RB_SetGL2D();
	qglFinish();

	target = GL_TEXTURE_RECTANGLE_ARB;

	width = tr.bloomWidth;
	height = tr.bloomHeight;

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.bloomTexture);

	qglCopyTexSubImage2D(target, 0, 0, 0, 0, glConfig.vidHeight - height, width, height);

	GL_UseProgram(tr.combineSp);

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.backBufferTexture);

	loc = qglGetUniformLocation(tr.combineSp, "backBufferTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get backBufferTex", __FUNCTION__);
	}
	qglUniform1i(loc, 0);

	GL_BindMultiTexture(GL_TEXTURE0_ARB + 1, target, tr.bloomTexture);

	loc = qglGetUniformLocation(tr.combineSp, "bloomTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get bloomTex", __FUNCTION__);
	}
	qglUniform1i(loc, 1);

	loc = qglGetUniformLocation(tr.combineSp, "p_bloomsaturation");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_bloomsaturation", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)r_BloomSaturation->value);

	loc = qglGetUniformLocation(tr.combineSp, "p_scenesaturation");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_scenesaturation", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)r_BloomSceneSaturation->value);

	loc = qglGetUniformLocation(tr.combineSp, "p_bloomintensity");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_bloomintensity", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)r_BloomIntensity->value);

	loc = qglGetUniformLocation(tr.combineSp, "p_sceneintensity");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_sceneintensity", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)r_BloomSceneIntensity->value);

	loc = qglGetUniformLocation(tr.combineSp, "u_ModelViewProjectionMatrix");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get u_ModelViewProjectionMatrix", __FUNCTION__);
	}
	qglUniformMatrix4fv(loc, 1 /* only setting 1 matrix */, GL_FALSE /*transpose?*/, glState.modelviewProjection);


	// for combine do not use these
	#if 0
	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	#endif

	GL_State( GLS_DEPTHTEST_DISABLE);

	// cull
	qglDisable( GL_CULL_FACE );

	qglDepthMask(GL_FALSE);
	
	width = glConfig.vidWidth;
	height = glConfig.vidHeight;

	VectorSet4(quadVerts[0], 0.0, (float)height, 0, 0);
	VectorSet4(quadVerts[1], (float)width, (float)height, 0, 0);
	VectorSet4(quadVerts[2], (float)width, 0.0, 0, 0);
	VectorSet4(quadVerts[3], 0.0, 0.0, 0, 0);

	VectorSet2(texCoords[0], 0, 0);
	VectorSet2(texCoords[1], (float)width, 0);
	VectorSet2(texCoords[2], (float)width, (float)height);
	VectorSet2(texCoords[3], 0, height);

	// RB_InstantQuad2 with tex1 coords
	// --

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	VectorCopy4(quadVerts[0], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[0], tess.texCoords[tess.numVertexes]);
	//VectorCopy2(texCoords[0], tess.lightCoords[tess.numVertexes]);
	VectorSet2(tess.lightCoords[tess.numVertexes], 0.0f, 0.0f);
	tess.numVertexes++;

	VectorCopy4(quadVerts[1], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[1], tess.texCoords[tess.numVertexes]);
	//VectorCopy2(texCoords[1], tess.lightCoords[tess.numVertexes]);
	VectorSet2(tess.lightCoords[tess.numVertexes], (float)tr.bloomWidth, 0.0f);
	tess.numVertexes++;

	VectorCopy4(quadVerts[2], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[2], tess.texCoords[tess.numVertexes]);
	//VectorCopy2(texCoords[2], tess.lightCoords[tess.numVertexes]);
	VectorSet2(tess.lightCoords[tess.numVertexes], (float)tr.bloomWidth, (float)tr.bloomHeight);
	tess.numVertexes++;

	VectorCopy4(quadVerts[3], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[3], tess.texCoords[tess.numVertexes]);
	//VectorCopy2(texCoords[3], tess.lightCoords[tess.numVertexes]);
	VectorSet2(tess.lightCoords[tess.numVertexes], 0.0f, (float)tr.bloomHeight);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	RB_UpdateTessVao(ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD);

	R_DrawElements(tess.numIndexes, tess.firstIndex);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;

	// --

	qglFinish();

	// restore cull
	{
		int current = glState.faceCulling;

		glState.faceCulling = !current;
		GL_Cull(current);
	}

	GL_BindNullProgram();
	GL_BindNullTextures();
}

static void RB_QLBloom (void)
{
	int i;

	//ri.Printf(PRINT_ALL, "^5bloom...\n");

	if (glRefConfig.framebufferObject) {
		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(tr.finalFbo);
		} else {
			//FIXME wolfcam: GL_BindNullFramebuffers() doesn't work, it shows a black screen in the first main menu, if you enter another menu the rendering is correct
			FBO_Bind(NULL);
		}
	}

	for (i = 0;  i < r_BloomPasses->integer;  i++) {
		RB_QLBloomDownSample();
		RB_QLBloomBrightness();

		if (r_BloomDebug->integer < 2) {
			RB_QLBloomBlurHorizontal();
			RB_QLBloomBlurVertical();
		}
		if (r_BloomDebug->integer == 0  ||  r_BloomDebug->integer == 3) {
			RB_QLBloomCombine();
		}
	}
}

static void RB_QLPostProcessing (void)
{
	//GLenum target;
	//int width, height;

	//ri.Printf(PRINT_ALL, "count %d\n", tr.drawSurfsCount);
	if (tr.drawSurfsCount != 0) {
		//return;
	}

	//ri.Printf(PRINT_ALL, "en: %d  glsl: %d  enbl: %d\n", r_enablePostProcess->integer, glConfig.qlGlsl, r_enableBloom->integer);


	if (!r_enablePostProcess->integer  ||  !glConfig.qlGlsl) {
		return;
	}

	if (!r_enableBloom->integer) {
		return;
	}

	// 2017-03-23 qglScissor() not working with bloom and 3d icons which set
	//  a new scissor value  --  GL_RENDERER: AMD Radeon R9 M370X
	//FIXME not enough to set scissor to size of viewport like in RB_SetGL2D()?
	qglDisable(GL_SCISSOR_TEST);


	RB_SetGL2D();
	
	//qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	//SetViewportAndScissor();
	//GL_SetModelviewMatrix(backEnd.or.modelMatrix);
	//GL_SetModelviewMatrix(backEnd.viewParms.world.modelMatrix);
	
	//qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

#if 0
	// since bloom is using readpixels()
	if (tr.usingMultiSample) {
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.frameBufferMultiSample);
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.frameBuffer);
		if (tr.usingFboStencil) {
			// packed depth stencil, but apparently some cards have a problem
			// if GL_STENCIL_BUFFER_BIT used
			qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		} else {
			qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		//qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.frameBuffer);
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tr.frameBuffer);
	}
#endif

	RB_QLBloom();

#if 0
	// anti alias 2d
	if (tr.usingMultiSample) {
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, tr.frameBufferMultiSample);
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.frameBuffer);
		if (0) {  //(tr.usingFboStencil) {
			// packed depth stencil, but apparently some cards have a problem
			// if GL_STENCIL_BUFFER_BIT used
			qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		} else {
			qglBlitFramebufferEXT(0, 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		//qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, tr.frameBuffer);
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tr.frameBufferMultiSample);
	}
#endif

	qglEnable(GL_SCISSOR_TEST);
}

static void RB_ColorCorrect (void)
{
	GLint loc;
	GLenum target;
	int width, height;
	int shift;
	float mul;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	if (!r_enablePostProcess->integer  ||  !r_enableColorCorrect->integer  ||  !glConfig.qlGlsl) {
		return;
	}

	if (glRefConfig.framebufferObject) {
		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(tr.finalFbo);
		} else {
			//GL_BindNullFramebuffers();
			FBO_Bind(NULL);
		}
	}

	width = glConfig.vidWidth;
	height = glConfig.vidHeight;

	//GL_BindNullProgram();
	//GL_BindNullTextures();

	RB_SetGL2D();

	qglFinish();

	target = GL_TEXTURE_RECTANGLE_ARB;

	width = glConfig.vidWidth;
	height = glConfig.vidHeight;

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.backBufferTexture);

	qglCopyTexSubImage2D(target, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight);

	GL_UseProgram(tr.colorCorrectSp);

	loc = qglGetUniformLocation(tr.colorCorrectSp, "p_gammaRecip");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_gammaRecip", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)(1.0 / r_gamma->value));

	mul = r_overBrightBitsValue->value;
	if (mul < 0.0) {
		mul = 0.0;
	}
	shift = tr.overbrightBits;

	loc = qglGetUniformLocation(tr.colorCorrectSp, "p_overbright");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_overbright", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)((float)(1 << shift) * mul));

	loc = qglGetUniformLocation(tr.colorCorrectSp, "p_contrast");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get p_contrast", __FUNCTION__);
	}
	qglUniform1f(loc, (GLfloat)r_contrast->value);

	loc = qglGetUniformLocation(tr.colorCorrectSp, "backBufferTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get backBufferTex", __FUNCTION__);
	}

	//qglUniform1i(loc, GL_TEXTURE0 + TB_COLORMAP);
	//ri.Printf(PRINT_ALL, "^5GL_TEXTURE0 : %d\n", GL_TEXTURE0);
	qglUniform1i(loc, 0);

	loc = qglGetUniformLocation(tr.colorCorrectSp, "u_ModelViewProjectionMatrix");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get u_ModelViewProjectionMatrix", __FUNCTION__);
	}

	qglUniformMatrix4fv(loc, 1 /* only setting 1 matrix */, GL_FALSE /*transpose?*/, glState.modelviewProjection);


#if 0
	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
#endif

	GL_State(GLS_DEPTHTEST_DISABLE);

	// cull
	qglDisable( GL_CULL_FACE );

	qglDepthMask(GL_FALSE);

	//qglClearColor(0.0, 0.0, 0.0, 1.0);
	//qglClear(GL_COLOR_BUFFER_BIT);

	//width /= 2;
	//height /= 2;

	VectorSet4(quadVerts[0], 0.0, (float)height, 0, 0);
	VectorSet4(quadVerts[1], (float)width, (float)height, 0, 0);
	VectorSet4(quadVerts[2], (float)width, 0.0, 0, 0);
	VectorSet4(quadVerts[3], 0.0, 0.0, 0, 0);

	VectorSet2(texCoords[0], 0, 0);
	VectorSet2(texCoords[1], (float)width, 0);
	VectorSet2(texCoords[2], (float)width, (float)height);
	VectorSet2(texCoords[3], 0, height);

	RB_InstantQuad2(quadVerts, texCoords);

	qglFinish();

	// restore cull
	{
		int current = glState.faceCulling;

		glState.faceCulling = !current;
		GL_Cull(current);
	}

	GL_BindNullProgram();
	GL_BindNullTextures();
}

/*
==================
RB_RenderDrawSurfList
==================
*/
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t                *shader = NULL, *oldShader;
	int				fogNum, oldFogNum;
	int				entityNum, oldEntityNum;
	int				dlighted, oldDlighted;
	int				pshadowed, oldPshadowed;
	int             cubemapIndex, oldCubemapIndex;
	qboolean		depthRange, oldDepthRange, isCrosshair, wasCrosshair;
	int				i;
	drawSurf_t		*drawSurf;
	//int				oldSort;
	uint64_t 		oldSort;
	double			originalTime;
	double			realOriginalTime;
	FBO_t*			fbo = NULL;

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;
	realOriginalTime = backEnd.refdef.realFloatTime;

	fbo = glState.currentFBO;

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	wasCrosshair = qfalse;
	oldDlighted = qfalse;
	oldPshadowed = qfalse;
	oldCubemapIndex = -1;
	oldSort = -1;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++) {
		qboolean useMergable;
		qboolean dontMerge;

		if ( drawSurf->sort == oldSort && drawSurf->cubemapIndex == oldCubemapIndex) {
			if (backEnd.depthFill && shader && (shader->sort != SS_OPAQUE && shader->sort != SS_PORTAL))
				continue;

			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed );
		cubemapIndex = drawSurf->cubemapIndex;

				if (r_ignoreEntityMergable->integer == 0) {
			useMergable = qtrue;
		} else if (r_ignoreEntityMergable->integer == 2  &&  mme_saveDepth->integer == 0) {
			useMergable = qtrue;
		} else {
			useMergable = qfalse;
		}

		if (useMergable) {
			if (shader  &&  !shader->entityMergable) {
				dontMerge = qtrue;
			} else {
				dontMerge = qfalse;
			}
		} else {
			dontMerge = qtrue;
		}

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from separate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != NULL && ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted || pshadowed != oldPshadowed || cubemapIndex != oldCubemapIndex
			|| ( entityNum != oldEntityNum  &&  dontMerge ) ) ) {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum, cubemapIndex );
			backEnd.pc.c_surfBatches++;
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
			oldPshadowed = pshadowed;
			oldCubemapIndex = cubemapIndex;
		}

		if (backEnd.depthFill && shader && (shader->sort != SS_OPAQUE && shader->sort != SS_PORTAL))
			continue;

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = isCrosshair = qfalse;

			if ( entityNum != REFENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];

				// FIXME: e.shaderTime must be passed as int to avoid fp-precision loss issues
				backEnd.refdef.floatTime = originalTime - (double)backEnd.currentEntity->e.shaderTime;
				backEnd.refdef.realFloatTime = realOriginalTime - (double)backEnd.currentEntity->e.shaderTime;

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				if (tess.shader->useRealTime) {
					tess.shaderTime = backEnd.refdef.realFloatTime - tess.shader->timeOffset;
				} else {
					tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				}

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
					
					if(backEnd.currentEntity->e.renderfx & RF_CROSSHAIR)
						isCrosshair = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.refdef.realFloatTime = realOriginalTime;
				backEnd.or = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				if (tess.shader->useRealTime) {
					tess.shaderTime = backEnd.refdef.realFloatTime - tess.shader->timeOffset;
				} else {
					tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				}
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}

			GL_SetModelviewMatrix( backEnd.or.modelMatrix );

			//
			// change depthrange. Also change projection matrix so first person weapon does not look like coming
			// out of the screen.
			//
			if (oldDepthRange != depthRange || wasCrosshair != isCrosshair)
			{
				if (depthRange)
				{
					if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
					{
						if(isCrosshair)
						{
							if(oldDepthRange)
							{
								// was not a crosshair but now is, change back proj matrix
								GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
							}
						}
						else
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix( temp.projectionMatrix );
						}
					}

					if(!oldDepthRange)
						qglDepthRange (0, 0.3);
				}
				else
				{
					if(!wasCrosshair && backEnd.viewParms.stereoFrame != STEREO_CENTER)
					{
						GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
					}

					qglDepthRange (0, 1);
				}

				oldDepthRange = depthRange;
				wasCrosshair = isCrosshair;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	backEnd.refdef.floatTime = originalTime;
	backEnd.refdef.realFloatTime = realOriginalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	if (glRefConfig.framebufferObject) {
		if (tr.usingFinalFrameBufferObject  &&  fbo == NULL) {
			fbo = tr.finalFbo;
		}

		FBO_Bind(fbo);
	}

	// go back to the world modelview matrix

	GL_SetModelviewMatrix( backEnd.viewParms.world.modelMatrix );

	qglDepthRange (0, 1);
}


/*
============================================================================

RENDER BACK END FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void	RB_SetGL2D (void) {
	mat4_t matrix;
	int width, height;

	if (backEnd.projection2D && backEnd.last2DFBO == glState.currentFBO)
		return;

	backEnd.projection2D = qtrue;
	backEnd.last2DFBO = glState.currentFBO;

	if (glState.currentFBO)
	{
		width = glState.currentFBO->width;
		height = glState.currentFBO->height;
	}
	else
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}

	// set 2D virtual screen size
	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Mat4Ortho(0, width, height, 0, 0, 1, matrix);
	GL_SetProjectionMatrix(matrix);
	Mat4Identity(matrix);
	GL_SetModelviewMatrix(matrix);

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );

	// set time for 2D shaders
	backEnd.refdef.time = ri.ScaledMilliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001;

	backEnd.refdef.realTime = ri.RealMilliseconds();
	backEnd.refdef.realFloatTime = backEnd.refdef.realTime * 0.001f;
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	if ( !tr.registered ) {
		return;
	}
	R_IssuePendingRenderCommands();

	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definitely want to sync every frame for the cinematics
	qglFinish();

	start = 0;
	if ( r_speeds->integer ) {
		start = ri.RealMilliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	RE_UploadCinematic (w, h, cols, rows, data, client, dirty);
	GL_BindToTMU(tr.scratchImage[client], TB_COLORMAP);

	if ( r_speeds->integer ) {
		end = ri.RealMilliseconds();
		ri.Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	if (glRefConfig.framebufferObject)
	{
		/*
		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(r_postProcess->integer ? tr.finalFbo : tr.renderFbo);
		} else {
			FBO_Bind(r_postProcess->integer ? NULL : tr.renderFbo);
		}
		*/
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	VectorSet4(quadVerts[0], x,     y,     0.0f, 1.0f);
	VectorSet4(quadVerts[1], x + w, y,     0.0f, 1.0f);
	VectorSet4(quadVerts[2], x + w, y + h, 0.0f, 1.0f);
	VectorSet4(quadVerts[3], x,     y + h, 0.0f, 1.0f);

	VectorSet2(texCoords[0], 0.5f / cols,          0.5f / rows);
	VectorSet2(texCoords[1], (cols - 0.5f) / cols, 0.5f / rows);
	VectorSet2(texCoords[2], (cols - 0.5f) / cols, (rows - 0.5f) / rows);
	VectorSet2(texCoords[3], 0.5f / cols,          (rows - 0.5f) / rows);

	GLSL_BindProgram(&tr.textureColorShader);

	GLSL_SetUniformMat4(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}

// glConfig.vidWidth x glConfig.vidHeight rgb data
static void RE_StretchRawRectScreen (const byte *data)
{
	int start, end;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	GLenum target;
	GLint loc;
	int width;
	int height;

	if ( !tr.registered ) {
		return;
	}

	if (!glConfig.textureRectangleAvailable) {
		static qboolean warningIssued = qfalse;

		if (!warningIssued) {
			ri.Printf(PRINT_ALL, "^3%s: rectangle texture not supported\n", __FUNCTION__);
			warningIssued = qtrue;
		}

		return;
	}

	width = glConfig.vidWidth;
	height = glConfig.vidHeight;

	//R_IssuePendingRenderCommands();

	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definitely want to sync every frame for the cinematics
	qglFinish();

	start = 0;
	if ( r_speeds->integer ) {
		start = ri.RealMilliseconds();
	}

	target = GL_TEXTURE_RECTANGLE_ARB;

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.rectScreenTexture);

	qglTexSubImage2D(target, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

	if ( r_speeds->integer ) {
		end = ri.RealMilliseconds();
		ri.Printf( PRINT_ALL, "rect: qglTexSubImage2D %i, %i: %i msec\n", width, height, end - start );
	}

	// FIXME: HUGE hack
	if (glRefConfig.framebufferObject)
	{
		/*
		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(r_postProcess->integer ? tr.finalFbo : tr.renderFbo);
		} else {
			FBO_Bind(r_postProcess->integer ? NULL : tr.renderFbo);
		}
		*/
		// 2024-04-04 wc  not sure...
		FBO_Bind(tr.renderFbo);
	}

	RB_SetGL2D();

	GL_BindMultiTexture(GL_TEXTURE0_ARB, target, tr.rectScreenTexture);

	GL_UseProgram(tr.rectScreenShader.program);

	loc = qglGetUniformLocation(tr.rectScreenShader.program, "backBufferTex");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get backBufferTex", __FUNCTION__);
	}

	//qglUniform1i(loc, GL_TEXTURE0 + TB_COLORMAP);
	//ri.Printf(PRINT_ALL, "^5GL_TEXTURE0 : %d\n", GL_TEXTURE0);
	qglUniform1i(loc, 0);

	//GLSL_SetUniformMat4(&tr.rectScreenShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

	loc = qglGetUniformLocation(tr.rectScreenShader.program, "u_ModelViewProjectionMatrix");
	if (loc < 0) {
		ri.Error(ERR_FATAL, "%s() couldn't get u_ModelViewProjectionMatrix", __FUNCTION__);
	}

	qglUniformMatrix4fv(loc, 1 /* only setting 1 matrix */, GL_FALSE /*transpose?*/, glState.modelviewProjection);

	GL_State(GLS_DEPTHTEST_DISABLE);

	// cull
	qglDisable( GL_CULL_FACE );

	qglDepthMask(GL_FALSE);

	qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);

	//FIXME testing
	//w /= 2;
	//h /= 2;

	VectorSet4(quadVerts[0], 0.0, (float)height, 0, 0);
	VectorSet4(quadVerts[1], (float)width, (float)height, 0, 0);
	VectorSet4(quadVerts[2], (float)width, 0.0, 0, 0);
	VectorSet4(quadVerts[3], 0.0, 0.0, 0, 0);

	VectorSet2(texCoords[0], 0, 0);
	VectorSet2(texCoords[1], (float)width, 0);
	VectorSet2(texCoords[2], (float)width, (float)height);
	VectorSet2(texCoords[3], 0, (float)height);


	RB_InstantQuad2(quadVerts, texCoords);

	qglFinish();

	// restore cull
	{
		int current = glState.faceCulling;

		glState.faceCulling = !current;
		GL_Cull(current);
	}

	GL_BindNullProgram();
	GL_BindNullTextures();
}

void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	GLuint texture;

	if (!tr.scratchImage[client])
	{
		ri.Printf(PRINT_WARNING, "RE_UploadCinematic: scratch images not initialized\n");
		return;
	}

	texture = tr.scratchImage[client]->texnum;

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTextureImage2DEXT(texture, GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTextureSubImage2DEXT(texture, GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
}


/*
=============
RB_SetColor

=============
*/
const void	*RB_SetColor( const void *data ) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)(cmd + 1);
}

/*
=============
RB_StretchPic
=============
*/
const void *RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int		numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	if (glRefConfig.framebufferObject) {
		/*
		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(r_postProcess->integer ? tr.finalFbo : tr.renderFbo);
		} else {
			FBO_Bind(r_postProcess->integer ? NULL : tr.renderFbo);
		}
		*/
		FBO_Bind(tr.renderFbo);
	}
	
	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		uint16_t color[4];

		VectorScale4(backEnd.color2D, 257, color);

		VectorCopy4(color, tess.color[ numVerts ]);
		VectorCopy4(color, tess.color[ numVerts + 1]);
		VectorCopy4(color, tess.color[ numVerts + 2]);
		VectorCopy4(color, tess.color[ numVerts + 3 ]);
	}

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0] = cmd->s1;
	tess.texCoords[ numVerts ][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][1] = cmd->t2;

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawSurfs

=============
*/
const void	*RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;
	qboolean isShadowView;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	isShadowView = !!(backEnd.viewParms.flags & VPF_DEPTHSHADOW);

	//FIXME not here?
	// from q3mme
	//Jitter the camera origin
	if ( !backEnd.viewParms.isPortal && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) ) {
		int i;
		float x, y;
		qboolean adjustOrigin = qfalse;

		if ((tr.recordingVideo  ||  mme_dofVisualize->integer)  &&  mme_dofFrames->integer > 0) {
			if (r_anaglyphMode->integer == 19  &&  *ri.SplitVideo  &&  !tr.leftRecorded) {
				adjustOrigin = R_MME_JitterOrigin(&x, &y, qfalse);
			} else {
				adjustOrigin = R_MME_JitterOrigin(&x, &y, qtrue);
			}
		}

		if (adjustOrigin) {
			orientationr_t* or = &backEnd.viewParms.or;
			orientationr_t* world = &backEnd.viewParms.world;

//			VectorScale( or->axis[0], 0.5, or->axis[0] );
//			VectorScale( or->axis[1], 0.3, or->axis[1] );
//			VectorScale( or->axis[2], 0.8, or->axis[2] );
			VectorMA( or->origin, x, or->axis[1], or->origin );
			VectorMA( or->origin, y, or->axis[2], or->origin );
//			or->origin[2] += 4000;
//			or->origin[2] += 0.1 * x;
			R_RotateForWorld( or, world );
			for ( i = 0; i < 16; i++ ) {
				////int r = (rand() & 0xffff ) - 0x4000;
				//world->modelMatrix[i] *= (0.9 + r * 0.0001);
				//or->modelMatrix[i] *= (0.9 + r * 0.0001);
			}
		} else {
			for ( i = 0; i < 16; i++ ) {
//				int r = (rand() & 0xffff ) - 0x4000;
//				backEnd.viewParms.world.modelMatrix[i] *= (0.9 + r * 0.0001);
			}
		}
	}

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	if (glRefConfig.framebufferObject && (backEnd.viewParms.flags & VPF_DEPTHCLAMP) && glRefConfig.depthClamp)
	{
		qglEnable(GL_DEPTH_CLAMP);
	}

	if (glRefConfig.framebufferObject && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && (r_depthPrepass->integer || isShadowView))
	{
		FBO_t *oldFbo = glState.currentFBO;
		vec4_t viewInfo;

		VectorSet4(viewInfo, backEnd.viewParms.zFar / r_znear->value, backEnd.viewParms.zFar, 0.0, 0.0);

		backEnd.depthFill = qtrue;
		qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
		qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		backEnd.depthFill = qfalse;

		if (!isShadowView)
		{
			if (tr.msaaResolveFbo)
			{
				// If we're using multisampling, resolve the depth first
				FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
			}
			else if (tr.renderFbo == NULL && tr.renderDepthImage)
			{
				// If we're rendering directly to the screen, copy the depth to a texture
				// This is incredibly slow on Intel Graphics, so just skip it on there
				if (!glRefConfig.intelGraphics)
					qglCopyTextureSubImage2DEXT(tr.renderDepthImage->texnum, GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight);
			}

			if (tr.hdrDepthFbo)
			{
				// need the depth in a texture we can do GL_LINEAR sampling on, so copy it to an HDR image
				vec4_t srcTexCoords;

				VectorSet4(srcTexCoords, 0.0f, 0.0f, 1.0f, 1.0f);

				FBO_BlitFromTexture(tr.renderDepthImage, srcTexCoords, NULL, tr.hdrDepthFbo, NULL, NULL, NULL, 0);
			}

			if (r_sunlightMode->integer && backEnd.viewParms.flags & VPF_USESUNLIGHT)
			{
				vec4_t quadVerts[4];
				vec2_t texCoords[4];
				vec4_t box;

				FBO_Bind(tr.screenShadowFbo);

				box[0] = backEnd.viewParms.viewportX      * tr.screenShadowFbo->width / (float)glConfig.vidWidth;
				box[1] = backEnd.viewParms.viewportY      * tr.screenShadowFbo->height / (float)glConfig.vidHeight;
				box[2] = backEnd.viewParms.viewportWidth  * tr.screenShadowFbo->width / (float)glConfig.vidWidth;
				box[3] = backEnd.viewParms.viewportHeight * tr.screenShadowFbo->height / (float)glConfig.vidHeight;

				qglViewport(box[0], box[1], box[2], box[3]);
				qglScissor(box[0], box[1], box[2], box[3]);

				box[0] = backEnd.viewParms.viewportX / (float)glConfig.vidWidth;
				box[1] = backEnd.viewParms.viewportY / (float)glConfig.vidHeight;
				box[2] = box[0] + backEnd.viewParms.viewportWidth / (float)glConfig.vidWidth;
				box[3] = box[1] + backEnd.viewParms.viewportHeight / (float)glConfig.vidHeight;

				texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
				texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
				texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
				texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

				box[0] = -1.0f;
				box[1] = -1.0f;
				box[2] = 1.0f;
				box[3] = 1.0f;

				VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
				VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
				VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
				VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

				GL_State(GLS_DEPTHTEST_DISABLE);

				GLSL_BindProgram(&tr.shadowmaskShader);

				GL_BindToTMU(tr.renderDepthImage, TB_COLORMAP);

				if (r_shadowCascadeZFar->integer != 0)
				{
					GL_BindToTMU(tr.sunShadowDepthImage[0], TB_SHADOWMAP);
					GL_BindToTMU(tr.sunShadowDepthImage[1], TB_SHADOWMAP2);
					GL_BindToTMU(tr.sunShadowDepthImage[2], TB_SHADOWMAP3);
					GL_BindToTMU(tr.sunShadowDepthImage[3], TB_SHADOWMAP4);

					GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP, backEnd.refdef.sunShadowMvp[0]);
					GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP2, backEnd.refdef.sunShadowMvp[1]);
					GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP3, backEnd.refdef.sunShadowMvp[2]);
					GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP4, backEnd.refdef.sunShadowMvp[3]);
				}
				else
				{
					GL_BindToTMU(tr.sunShadowDepthImage[3], TB_SHADOWMAP);
					GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP, backEnd.refdef.sunShadowMvp[3]);
				}

				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWORIGIN, backEnd.refdef.vieworg);
				{
					vec3_t viewVector;

					float zmax = backEnd.viewParms.zFar;
					float ymax = zmax * tan(backEnd.viewParms.fovY * M_PI / 360.0f);
					float xmax = zmax * tan(backEnd.viewParms.fovX * M_PI / 360.0f);

					VectorScale(backEnd.refdef.viewaxis[0], zmax, viewVector);
					GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWFORWARD, viewVector);
					VectorScale(backEnd.refdef.viewaxis[1], xmax, viewVector);
					GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWLEFT, viewVector);
					VectorScale(backEnd.refdef.viewaxis[2], ymax, viewVector);
					GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWUP, viewVector);

					GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_VIEWINFO, viewInfo);
				}

				RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

				if (r_shadowBlur->integer)
				{
					viewInfo[2] = 1.0f / (float)(tr.screenScratchFbo->width);
					viewInfo[3] = 1.0f / (float)(tr.screenScratchFbo->height);

					FBO_Bind(tr.screenScratchFbo);

					GLSL_BindProgram(&tr.depthBlurShader[0]);

					GL_BindToTMU(tr.screenShadowImage, TB_COLORMAP);
					GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

					GLSL_SetUniformVec4(&tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo);

					RB_InstantQuad2(quadVerts, texCoords);

					FBO_Bind(tr.screenShadowFbo);

					GLSL_BindProgram(&tr.depthBlurShader[1]);

					GL_BindToTMU(tr.screenScratchImage, TB_COLORMAP);
					GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

					GLSL_SetUniformVec4(&tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo);

					RB_InstantQuad2(quadVerts, texCoords);
				}
			}

			if (r_ssao->integer)
			{
				vec4_t quadVerts[4];
				vec2_t texCoords[4];

				viewInfo[2] = 1.0f / ((float)(tr.quarterImage[0]->width)  * tan(backEnd.viewParms.fovX * M_PI / 360.0f) * 2.0f);
				viewInfo[3] = 1.0f / ((float)(tr.quarterImage[0]->height) * tan(backEnd.viewParms.fovY * M_PI / 360.0f) * 2.0f);
				viewInfo[3] *= (float)backEnd.viewParms.viewportHeight / (float)backEnd.viewParms.viewportWidth;

				FBO_Bind(tr.quarterFbo[0]);

				qglViewport(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
				qglScissor(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);

				VectorSet4(quadVerts[0], -1,  1, 0, 1);
				VectorSet4(quadVerts[1],  1,  1, 0, 1);
				VectorSet4(quadVerts[2],  1, -1, 0, 1);
				VectorSet4(quadVerts[3], -1, -1, 0, 1);

				texCoords[0][0] = 0; texCoords[0][1] = 1;
				texCoords[1][0] = 1; texCoords[1][1] = 1;
				texCoords[2][0] = 1; texCoords[2][1] = 0;
				texCoords[3][0] = 0; texCoords[3][1] = 0;

				GL_State( GLS_DEPTHTEST_DISABLE );

				GLSL_BindProgram(&tr.ssaoShader);

				GL_BindToTMU(tr.hdrDepthImage, TB_COLORMAP);

				GLSL_SetUniformVec4(&tr.ssaoShader, UNIFORM_VIEWINFO, viewInfo);

				RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


				viewInfo[2] = 1.0f / (float)(tr.quarterImage[0]->width);
				viewInfo[3] = 1.0f / (float)(tr.quarterImage[0]->height);

				FBO_Bind(tr.quarterFbo[1]);

				qglViewport(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);
				qglScissor(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);

				GLSL_BindProgram(&tr.depthBlurShader[0]);

				GL_BindToTMU(tr.quarterImage[0],  TB_COLORMAP);
				GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

				GLSL_SetUniformVec4(&tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo);

				RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


				FBO_Bind(tr.screenSsaoFbo);

				qglViewport(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);
				qglScissor(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);

				GLSL_BindProgram(&tr.depthBlurShader[1]);

				GL_BindToTMU(tr.quarterImage[1],  TB_COLORMAP);
				GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

				GLSL_SetUniformVec4(&tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo);


				RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);
			}
		}

		// reset viewport and scissor
		FBO_Bind(oldFbo);
		SetViewportAndScissor();
	}

	if (glRefConfig.framebufferObject && (backEnd.viewParms.flags & VPF_DEPTHCLAMP) && glRefConfig.depthClamp)
	{
		qglDisable(GL_DEPTH_CLAMP);
	}

	if (!isShadowView)
	{
		//RE_DrawPathLines();
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

		RE_DrawPathLines();

		if (r_drawSun->integer)
		{
			RB_DrawSun(0.1, tr.sunShader);
		}

		if (glRefConfig.framebufferObject && r_drawSunRays->integer)
		{
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.sunRaysFbo);
			
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			qglClear( GL_COLOR_BUFFER_BIT );

			if (glRefConfig.occlusionQuery)
			{
				tr.sunFlareQueryActive[tr.sunFlareQueryIndex] = qtrue;
				qglBeginQuery(GL_SAMPLES_PASSED, tr.sunFlareQuery[tr.sunFlareQueryIndex]);
			}

			RB_DrawSun(0.3, tr.sunFlareShader);

			if (glRefConfig.occlusionQuery)
			{
				qglEndQuery(GL_SAMPLES_PASSED);
			}

			FBO_Bind(oldFbo);
		}

		// darken down any stencil shadows
		RB_ShadowFinish();		

		// add light flares on lights that aren't obscured
		RB_RenderFlares();
	}

	if (glRefConfig.framebufferObject && tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		cubemap_t *cubemap = &tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex];

		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(tr.finalFbo);
		} else {
			FBO_Bind(NULL);
		}
		if (cubemap && cubemap->image)
			qglGenerateTextureMipmapEXT(cubemap->image->texnum, GL_TEXTURE_CUBE_MAP);
	}

	// FIXME? backEnd.viewParms doesn't get properly initialized for 2D drawing.
	// r_cubeMapping 1 generates cubemaps with R_RenderCubemapSide()
	// and sets isMirror = qtrue. Clear it here to prevent it from leaking
	// to 2D drawing and causing the loading screen to be culled.
	backEnd.viewParms.isMirror = qfalse;
	backEnd.viewParms.flags = 0;

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer

=============
*/
const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (glRefConfig.framebufferObject) {
		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(tr.finalFbo);
		} else {
			FBO_Bind(NULL);
		}
	}

	if (!tr.usingFinalFrameBufferObject) {
		qglDrawBuffer( cmd->buffer );
	}

	// clear screen for debugging
	if ( r_clear->integer ) {
		if (*r_clearColor->string) {
			int v, tr, tg, tb;

			v = r_clearColor->integer;
			tr = (v & 0xff0000) / 0x010000;
			tg = (v & 0x00ff00) / 0x000100;
			tb = (v & 0x0000ff) / 0x000001;

			qglClearColor((float)tr / 255.0, (float)tg / 255.0, (float)tb / 255.0, 1.0);
		} else {
			qglClearColor( 1, 0, 0.5, 1 );
		}

		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)(cmd + 1);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	start = ri.RealMilliseconds();

	for ( i=0 ; i<tr.numImages ; i++ ) {
		image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		{
			vec4_t quadVerts[4];

			GL_BindToTMU(image, TB_COLORMAP);

			VectorSet4(quadVerts[0], x, y, 0, 1);
			VectorSet4(quadVerts[1], x + w, y, 0, 1);
			VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
			VectorSet4(quadVerts[3], x, y + h, 0, 1);

			RB_InstantQuad(quadVerts);
		}
	}

	qglFinish();

	end = ri.RealMilliseconds();
	ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

}

/*
=============
RB_ColorMask

=============
*/
const void *RB_ColorMask(const void *data)
{
	const colorMaskCommand_t *cmd = data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (glRefConfig.framebufferObject)
	{
		// reverse color mask, so 0 0 0 0 is the default
		backEnd.colorMask[0] = !cmd->rgba[0];
		backEnd.colorMask[1] = !cmd->rgba[1];
		backEnd.colorMask[2] = !cmd->rgba[2];
		backEnd.colorMask[3] = !cmd->rgba[3];
	}

	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
	
	return (const void *)(cmd + 1);
}

/*
=============
RB_ClearDepth

=============
*/
const void *RB_ClearDepth(const void *data)
{
	const clearDepthCommand_t *cmd = data;
	
	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// texture swapping test
	if (r_showImages->integer)
		RB_ShowImages();

	if (glRefConfig.framebufferObject)
	{
		if (!tr.renderFbo) {
			if (tr.usingFinalFrameBufferObject) {
				FBO_Bind(tr.finalFbo);
			} else {
				FBO_Bind(NULL);
			}
		}
		else
		{
			FBO_Bind(tr.renderFbo);
		}
	}

	qglClear(GL_DEPTH_BUFFER_BIT);

	// if we're doing MSAA, clear the depth texture for the resolve buffer
	if (tr.msaaResolveFbo)
	{
		FBO_Bind(tr.msaaResolveFbo);
		qglClear(GL_DEPTH_BUFFER_BIT);
	}

	
	return (const void *)(cmd + 1);
}


/*
=============
RB_SwapBuffers

=============
*/
const void	*RB_SwapBuffers( const void *data, qboolean endFrame ) {
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	//RB_ColorCorrect();

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}

	if (glRefConfig.framebufferObject)
	{
		if (tr.msaaResolveFbo && r_hdr->integer)
		{
			// Resolving an RGB16F MSAA FBO to the screen messes with the brightness, so resolve to an RGB16F FBO first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			if (tr.usingFinalFrameBufferObject) {
				FBO_FastBlit(tr.msaaResolveFbo, NULL, tr.finalFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			} else {
				FBO_FastBlit(tr.msaaResolveFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
		}
		else if (tr.renderFbo)
		{
			if (tr.usingFinalFrameBufferObject) {
				FBO_FastBlit(tr.renderFbo, NULL, tr.finalFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			} else {
					FBO_FastBlit(tr.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
		}

		//FBO_FastBlit(tr.renderFbo, NULL, (void *)-1, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		//ri.Printf(PRINT_ALL, "frame...\n");
	}

	qglDisable(GL_SCISSOR_TEST);
	//RB_QLPostProcessing();
	qglEnable(GL_SCISSOR_TEST);

	
	// 2017-03-23 qglScissor() not working with bloom and 3d icons which set
	//  a new scissor value  --  GL_RENDERER: AMD Radeon R9 M370X
	//FIXME not enough to set scissor to size of viewport like in RB_SetGL2D()?
	qglDisable(GL_SCISSOR_TEST);

	RB_ColorCorrect();

	qglEnable(GL_SCISSOR_TEST);

	if (glRefConfig.framebufferObject) {
		if (tr.usingFinalFrameBufferObject) {
			//GL_BindNullFramebuffers();
			FBO_Bind(NULL);

			//ri.Printf(PRINT_ALL, "finalfbo %p\n", tr.finalFbo);
			FBO_Blit(tr.finalFbo, NULL, NULL, NULL, NULL, NULL, NULL, 0);

			// this
			//FBO_FastBlit(tr.finalFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
	}

	if ( !glState.finishCalled ) {
		qglFinish();
	}

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	if (endFrame) {
		GLimp_EndFrame();
	}

	// videos and screenshots
	if (glRefConfig.framebufferObject) {
		if (tr.usingFinalFrameBufferObject) {
			FBO_Bind(tr.finalFbo);
		} else {
			//GL_BindNullFramebuffers();
			FBO_Bind(NULL);
		}
	}

	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
}

/*
=============
RB_CapShadowMap

=============
*/
const void *RB_CapShadowMap(const void *data)
{
	const capShadowmapCommand_t *cmd = data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (cmd->map != -1)
	{
		if (cmd->cubeSide != -1)
		{
			if (tr.shadowCubemaps[cmd->map])
			{
				qglCopyTextureSubImage2DEXT(tr.shadowCubemaps[cmd->map]->texnum, GL_TEXTURE_CUBE_MAP_POSITIVE_X + cmd->cubeSide, 0, 0, 0, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE);
			}
		}
		else
		{
			if (tr.pshadowMaps[cmd->map])
			{
				qglCopyTextureSubImage2DEXT(tr.pshadowMaps[cmd->map]->texnum, GL_TEXTURE_2D, 0, 0, 0, backEnd.refdef.x, glConfig.vidHeight - (backEnd.refdef.y + PSHADOW_MAP_SIZE), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE);
			}
		}
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_PostProcess

=============
*/
const void *RB_PostProcess(const void *data)
{
	const postProcessCommand_t *cmd = data;
	FBO_t *srcFbo, *dstFbo;
	ivec4_t srcBox, dstBox;
	qboolean autoExposure;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (!glRefConfig.framebufferObject || !r_postProcess->integer)
	{
		// do nothing
		return (const void *)(cmd + 1);
	}

	if (cmd)
	{
		backEnd.refdef = cmd->refdef;
		backEnd.viewParms = cmd->viewParms;
	}

	srcFbo = tr.renderFbo;
	dstFbo = tr.renderFbo;

	if (tr.msaaResolveFbo)
	{
		// Resolve the MSAA before anything else
		// Can't resolve just part of the MSAA FBO, so multiple views will suffer a performance hit here
		FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		srcFbo = tr.msaaResolveFbo;
	}

	dstBox[0] = backEnd.viewParms.viewportX;
	dstBox[1] = backEnd.viewParms.viewportY;
	dstBox[2] = backEnd.viewParms.viewportWidth;
	dstBox[3] = backEnd.viewParms.viewportHeight;

	if (r_ssao->integer)
	{
		srcBox[0] = backEnd.viewParms.viewportX      * tr.screenSsaoImage->width  / (float)glConfig.vidWidth;
		srcBox[1] = backEnd.viewParms.viewportY      * tr.screenSsaoImage->height / (float)glConfig.vidHeight;
		srcBox[2] = backEnd.viewParms.viewportWidth  * tr.screenSsaoImage->width  / (float)glConfig.vidWidth;
		srcBox[3] = backEnd.viewParms.viewportHeight * tr.screenSsaoImage->height / (float)glConfig.vidHeight;

		FBO_Blit(tr.screenSsaoFbo, srcBox, NULL, srcFbo, dstBox, NULL, NULL, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
	}

	srcBox[0] = backEnd.viewParms.viewportX;
	srcBox[1] = backEnd.viewParms.viewportY;
	srcBox[2] = backEnd.viewParms.viewportWidth;
	srcBox[3] = backEnd.viewParms.viewportHeight;

	if (srcFbo)
	{
		if (r_hdr->integer && (r_toneMap->integer || r_forceToneMap->integer))
		{
			autoExposure = r_autoExposure->integer || r_forceAutoExposure->integer;
			/*
			if (tr.usingFinalFrameBufferObject) {
				RB_ToneMap(srcFbo, srcBox, tr.finalFbo, dstBox, autoExposure);
			} else {
				RB_ToneMap(srcFbo, srcBox, NULL, dstBox, autoExposure);
			}
			*/
			// Use an intermediate FBO because it can't blit to the same FBO directly
			// and can't read from an MSAA dstFbo later.
			RB_ToneMap(srcFbo, srcBox, tr.screenScratchFbo, srcBox, autoExposure);
			FBO_FastBlit(tr.screenScratchFbo, srcBox, srcFbo, srcBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		/*
		else if (r_cameraExposure->value == 0.0f)
		{
			if (tr.usingFinalFrameBufferObject) {
				FBO_FastBlit(srcFbo, srcBox, tr.finalFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			} else {
				FBO_FastBlit(srcFbo, srcBox, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
		}
		*/
		else if (r_cameraExposure->value != 0.0f)
		{
			vec4_t color;

			color[0] =
			color[1] =
			color[2] = pow(2, r_cameraExposure->value); //exp2(r_cameraExposure->value);
			color[3] = 1.0f;

			/*
			if (tr.usingFinalFrameBufferObject) {
				FBO_Blit(srcFbo, srcBox, NULL, tr.finalFbo, dstBox, NULL, color, 0);
			} else {
				FBO_Blit(srcFbo, srcBox, NULL, NULL, dstBox, NULL, color, 0);
			}
			*/

			FBO_BlitFromTexture(tr.whiteImage, NULL, NULL, srcFbo, srcBox, NULL, color, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
		}
	}

	if (r_drawSunRays->integer) {
		if (tr.usingFinalFrameBufferObject) {
			RB_SunRays(tr.finalFbo, srcBox, tr.finalFbo, dstBox);
		} else {
			RB_SunRays(srcFbo, srcBox, srcFbo, dstBox);
		}
	}

	if (1) {
		if (tr.usingFinalFrameBufferObject) {
			RB_BokehBlur(tr.finalFbo, srcBox, tr.finalFbo, dstBox, backEnd.refdef.blurFactor);
		} else {
			RB_BokehBlur(srcFbo, srcBox, srcFbo, dstBox, backEnd.refdef.blurFactor);
		}
	} else {
		//RB_GaussianBlur(backEnd.refdef.blurFactor);
		if (tr.usingFinalFrameBufferObject) {
			RB_GaussianBlur(NULL, tr.finalFbo, backEnd.refdef.blurFactor);
		} else {
			RB_GaussianBlur(srcFbo, srcFbo, backEnd.refdef.blurFactor);
		}
	}

	// 2024-04-04 wtf?  they are always equal
	if (srcFbo != dstFbo)
		FBO_FastBlit(srcFbo, srcBox, dstFbo, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if 0
	if (0)
	{
		vec4_t quadVerts[4];
		vec2_t texCoords[4];
		ivec4_t iQtrBox;
		vec4_t box;
		vec4_t viewInfo;
		static float scale = 5.0f;

		scale -= 0.005f;
		if (scale < 0.01f)
			scale = 5.0f;

		if (tr.usingFinalFrameBufferObject) {
			FBO_FastBlit(tr.finalFbo, NULL, tr.quarterFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		} else {
			FBO_FastBlit(dstFbo, NULL, tr.quarterFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

		iQtrBox[0] = backEnd.viewParms.viewportX      * tr.quarterImage[0]->width / (float)glConfig.vidWidth;
		iQtrBox[1] = backEnd.viewParms.viewportY      * tr.quarterImage[0]->height / (float)glConfig.vidHeight;
		iQtrBox[2] = backEnd.viewParms.viewportWidth  * tr.quarterImage[0]->width / (float)glConfig.vidWidth;
		iQtrBox[3] = backEnd.viewParms.viewportHeight * tr.quarterImage[0]->height / (float)glConfig.vidHeight;

		qglViewport(iQtrBox[0], iQtrBox[1], iQtrBox[2], iQtrBox[3]);
		qglScissor(iQtrBox[0], iQtrBox[1], iQtrBox[2], iQtrBox[3]);

		VectorSet4(box, 0.0f, 0.0f, 1.0f, 1.0f);

		texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
		texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
		texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
		texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

		VectorSet4(box, -1.0f, -1.0f, 1.0f, 1.0f);

		VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
		VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
		VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
		VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

		GL_State(GLS_DEPTHTEST_DISABLE);


		VectorSet4(viewInfo, backEnd.viewParms.zFar / r_znear->value, backEnd.viewParms.zFar, 0.0, 0.0);

		viewInfo[2] = scale / (float)(tr.quarterImage[0]->width);
		viewInfo[3] = scale / (float)(tr.quarterImage[0]->height);

		FBO_Bind(tr.quarterFbo[1]);
		GLSL_BindProgram(&tr.depthBlurShader[2]);
		GL_BindToTMU(tr.quarterImage[0], TB_COLORMAP);
		GLSL_SetUniformVec4(&tr.depthBlurShader[2], UNIFORM_VIEWINFO, viewInfo);
		RB_InstantQuad2(quadVerts, texCoords);

		FBO_Bind(tr.quarterFbo[0]);
		GLSL_BindProgram(&tr.depthBlurShader[3]);
		GL_BindToTMU(tr.quarterImage[1], TB_COLORMAP);
		GLSL_SetUniformVec4(&tr.depthBlurShader[3], UNIFORM_VIEWINFO, viewInfo);
		RB_InstantQuad2(quadVerts, texCoords);

		SetViewportAndScissor();

		if (tr.usingFinalFrameBufferObject) {
			FBO_FastBlit(tr.quarterFbo[1], NULL, tr.finalFbo, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			FBO_Bind(tr.finalFbo);
		} else {
			FBO_FastBlit(tr.quarterFbo[1], NULL, dstFbo, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			FBO_Bind(NULL);
		}
	}
#endif

	if (0 && r_sunlightMode->integer)
	{
		ivec4_t dstBox;
		FBO_t *dstFboTmp = dstFbo;

		if (tr.usingFinalFrameBufferObject) {
			dstFboTmp = tr.finalFbo;
		}

		VectorSet4(dstBox, 0, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[0], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[1], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[2], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 384, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[3], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
	}

	if (0 && r_shadows->integer == 4)
	{
		ivec4_t dstBox;
		FBO_t *dstFboTmp = dstFbo;

		if (tr.usingFinalFrameBufferObject) {
			dstFboTmp = tr.finalFbo;
		}

		VectorSet4(dstBox, 512 + 0, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[0], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 128, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[1], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 256, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[2], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512 + 384, glConfig.vidHeight - 128, 128, 128);
		FBO_BlitFromTexture(tr.pshadowMaps[3], NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		ivec4_t dstBox;
		FBO_t *dstFboTmp = dstFbo;

		if (tr.usingFinalFrameBufferObject) {
			dstFboTmp = tr.finalFbo;
		}

		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.screenShadowImage, NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		ivec4_t dstBox;
		FBO_t *dstFboTmp = dstFbo;

		if (tr.usingFinalFrameBufferObject) {
			dstFboTmp = tr.finalFbo;
		}

		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.sunRaysImage, NULL, NULL, dstFboTmp, dstBox, NULL, NULL, 0);
	}

#if 0
	if (r_cubeMapping->integer && tr.numCubemaps)
	{
		ivec4_t dstBox;
		FBO_t *dstFboTmp = dstFbo;
		int cubemapIndex = R_CubemapForPoint( backEnd.viewParms.or.origin );

		if (tr.usingFinalFrameBufferObject) {
			dstFboTmp = tr.finalFbo;
		}

		if (cubemapIndex)
		{
			VectorSet4(dstBox, 0, glConfig.vidHeight - 256, 256, 256);
			//FBO_BlitFromTexture(tr.renderCubeImage, NULL, NULL, dstFboTmp, dstBox, &tr.testcubeShader, NULL, 0);
			FBO_BlitFromTexture(tr.cubemaps[cubemapIndex - 1].image, NULL, NULL, dstFboTmp, dstBox, &tr.testcubeShader, NULL, 0);
		}
	}
#endif

	//RB_ColorCorrect();

	return (const void *)(cmd + 1);
}

// FIXME: put this function declaration elsewhere
void R_SaveDDS(const char *filename, byte *pic, int width, int height, int depth);

/*
=============
RB_ExportCubemaps

=============
*/
const void *RB_ExportCubemaps(const void *data)
{
	const exportCubemapsCommand_t *cmd = data;

	// finish any 2D drawing if needed
	if (tess.numIndexes)
		RB_EndSurface();

	if (!glRefConfig.framebufferObject || !tr.world || tr.numCubemaps == 0)
	{
		// do nothing
		ri.Printf(PRINT_ALL, "Nothing to export!\n");
		return (const void *)(cmd + 1);
	}

	if (cmd)
	{
		FBO_t *oldFbo = glState.currentFBO;
		int sideSize = r_cubemapSize->integer * r_cubemapSize->integer * 4;
		byte *cubemapPixels = ri.Malloc(sideSize * 6);
		int i, j;

		FBO_Bind(tr.renderCubeFbo);

		for (i = 0; i < tr.numCubemaps; i++)
		{
			char filename[MAX_QPATH];
			cubemap_t *cubemap = &tr.cubemaps[i];
			byte *p = cubemapPixels;

			for (j = 0; j < 6; j++)
			{
				FBO_AttachImage(tr.renderCubeFbo, cubemap->image, GL_COLOR_ATTACHMENT0_EXT, j);
				qglReadPixels(0, 0, r_cubemapSize->integer, r_cubemapSize->integer, GL_RGBA, GL_UNSIGNED_BYTE, p);
				p += sideSize;
			}

			if (cubemap->name[0])
			{
				COM_StripExtension(cubemap->name, filename, MAX_QPATH);
				Q_strcat(filename, MAX_QPATH, ".dds");
			}
			else
			{
				Com_sprintf(filename, MAX_QPATH, "cubemaps/%s/%03d.dds", tr.world->baseName, i);
			}

			R_SaveDDS(filename, cubemapPixels, r_cubemapSize->integer, r_cubemapSize->integer, 6);
			ri.Printf(PRINT_ALL, "Saved cubemap %d as %s\n", i, filename);
		}

		FBO_Bind(oldFbo);

		ri.Free(cubemapPixels);
	}

	return (const void *)(cmd + 1);
}

static const void *RB_DebugGraphics (const void *data)
{
	const debugGraphicsCommand_t *cmd;

	cmd = (const debugGraphicsCommand_t *)data;

	GL_BindToTMU(tr.whiteImage, TB_COLORMAP);
	GL_Cull( CT_FRONT_SIDED );
	ri.CM_DrawDebugSurface( R_DebugPolygon );

	return (const void *)(cmd + 1);
}

static const void *RB_SkipRenderCommand (const void *data)
{
	data = PADP(data, sizeof(void *));

	switch (*(const int *)data) {
	case RC_SET_COLOR:
		data += sizeof(setColorCommand_t);
		break;
	case RC_STRETCH_PIC:
		data += sizeof(stretchPicCommand_t);
		break;
	case RC_DRAW_SURFS:
		data += sizeof(drawSurfsCommand_t);
		break;
	case RC_DRAW_BUFFER:
		data += sizeof(drawBufferCommand_t);
		break;
	case RC_SWAP_BUFFERS:
		data += sizeof(swapBuffersCommand_t);
		break;
	case RC_SCREENSHOT:
		data += sizeof(screenshotCommand_t);
		break;
	case RC_VIDEOFRAME:
		data += sizeof(videoFrameCommand_t);
		break;
	case RC_COLORMASK:
		data += sizeof(colorMaskCommand_t);
		break;
	case RC_CLEARDEPTH:
		data += sizeof(clearDepthCommand_t);
		break;
	case RC_CAPSHADOWMAP:
		data += sizeof(capShadowmapCommand_t);
		break;
	case RC_BEGIN_HUD:
		data += sizeof(beginHudCommand_t);
		break;
	case RC_DEBUG_GRAPHICS:
		data += sizeof(debugGraphicsCommand_t);
		break;
	case RC_POSTPROCESS:
		data += sizeof(postProcessCommand_t);
		break;
	case RC_EXPORT_CUBEMAPS:
		data += sizeof(exportCubemapsCommand_t);
		break;
	case RC_END_OF_LIST:
		ri.Printf(PRINT_ALL, "^1RB_SkipRenderCommand():  called with RC_END_OF_LIST\n");
		break;
	default:
		ri.Printf(PRINT_ALL, "^1RB_SkipRenderCommand():  unknown render command: %d\n", *(const int *)data);
		break;
	}

	return data;
}

/*
====================
RB_ExecuteRenderCommands
====================
*/

#if 0
#define dprintf(format, varargs...) ri.Printf(PRINT_ALL, format, ## varargs)
#else
  #define dprintf(format, varargs...)
#endif

void RB_ExecuteRenderCommands( const void *data ) {
	int		t1, t2;
	const void *dataOrig;
	qboolean depthWasCleared;
	const colorMaskCommand_t *cmd;
	qboolean colorMaskSet;
	GLboolean rgba[4];
	qboolean videoCommand;
	const void *data2;
	qboolean renderingHud;
	qboolean renderingHud2;

	// silence compiler warning
	rgba[0] = rgba[1] = rgba[2] = rgba[3] = GL_FALSE;

	tr.leftRecorded = qfalse;  // to break outer dof loop

	//FIXME splitting rendering between world and hud will not work correctly when R_IssuePendingRenderCommands() is used  -- 2018-08-10 hack added to only run R_IssuePendingRenderCommands() at the end of frame

	if (tr.recordingVideo  ||  mme_dofVisualize->integer) {
		R_MME_CheckCvars(qfalse, &shotDataMain);
		if (r_anaglyphMode->integer == 19  &&  *ri.SplitVideo) {
			R_MME_CheckCvars(qfalse, &shotDataLeft);
		}
	}

	t1 = ri.RealMilliseconds();
	dataOrig = data;

#if 0  // orig
	while ( 1 ) {
		data = PADP(data, sizeof(void *));

		switch ( *(const int *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data, qtrue );
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd( data );
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd(data, &shotDataMain);
			break;
		case RC_COLORMASK:
			data = RB_ColorMask(data);
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth(data);
			break;
		case RC_CAPSHADOWMAP:
			data = RB_CapShadowMap(data);
			break;
		case RC_POSTPROCESS:
			data = RB_PostProcess(data);
			RB_QLPostProcessing();
			break;
		case RC_EXPORT_CUBEMAPS:
			data = RB_ExportCubemaps(data);
			break;
		case RC_END_OF_LIST:
		default:
			// finish any 2D drawing if needed
			if(tess.numIndexes)
				RB_EndSurface();

			// stop rendering
			t2 = ri.RealMilliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}

	return;
#endif

	videoCommand = qfalse;
	data = dataOrig;
	while ( 1 ) {
		data = PADP(data, sizeof(void *));
		switch (*(const int *)data) {
		case RC_VIDEOFRAME:
			//ri.Printf(PRINT_ALL, "^2video cmd\n");
			videoCommand = qtrue;
			data = RB_SkipRenderCommand(data);
			break;
		case RC_END_OF_LIST:
			goto videoCommandCheckDone;
		default:
			data = RB_SkipRenderCommand(data);
			break;
		}
	}

 videoCommandCheckDone:

	depthWasCleared = qfalse;
	colorMaskSet = qfalse;
	tr.drawSurfsCount = 0;

	renderingHud = qfalse;

	data = dataOrig;
	dprintf("render1 commands start ----------------------------\n");


	//ri.Printf(PRINT_ALL, "^6 video command: %d\n", videoCommand);
	///////////////////

	data = dataOrig;

	while ( 1 ) {
		data = PADP(data, sizeof(void *));
		switch (*(const int *)data) {
		case RC_DRAW_SURFS:
			dprintf("r1 drawsurfs\n");
			if (!renderingHud) {
				data = RB_DrawSurfs(data);  //FIXME others are hud? you sure?
				tr.drawSurfsCount++;
			} else {
				data = RB_SkipRenderCommand(data);
			}
			break;

		case RC_CLEARDEPTH:
			dprintf("r1 cleardepth\n");

			if (r_anaglyphMode->integer == 19  &&  tr.recordingVideo  &&  *ri.SplitVideo  &&  !tr.leftRecorded) {
				videoFrameCommand_t cmd;

				if (!videoCommand) {
					//FIXME 'videoCommand' not used
					// shouldn't happen
				}

				if ( tess.numIndexes ) {
					RB_EndSurface();
				}

				if (!shotDataLeft.workAlloc) {
					ri.Error(ERR_DROP, "shotDataLeft memory not allocated");
				}

				if (tr.drawSurfsCount) {
					if (R_MME_MultiPassNext(qfalse)) {
						R_InitNextFrameNoCommands();
						goto videoCommandCheckDone;
					}

					// blit mme dof
					if (mme_dofFrames->integer > 1  &&  tr.recordingVideo  &&  R_MME_GetPassData(qfalse)  &&  !shotDataLeft.allocFailed) {
						byte *buffer;

						buffer = R_MME_GetPassData(qfalse);
						RE_StretchRawRectScreen(buffer);
					}
				}

				// draw hud
				renderingHud2 = qfalse;

				data2 = dataOrig;
				while ( 1 ) {
					data2 = PADP(data2, sizeof(void *));
					switch (*(const int *)data2) {
					case RC_DRAW_SURFS:
						dprintf("r1 -- depth drawsurfs\n");
						if (renderingHud2) {
							data2 = RB_DrawSurfs(data2);  // 3d hud models
							tr.drawSurfsCount++;
						} else {
							data2 = RB_SkipRenderCommand(data2);
						}
						break;

					case RC_SET_COLOR:
						//dprintf("r1 hud setcolor\n");
						data2 = RB_SetColor(data2);
						break;
					case RC_STRETCH_PIC:
						//dprintf("r1 hud stretchpic\n");
						data2 = RB_StretchPic(data2);
						break;

					case RC_BEGIN_HUD:
						data2 = RB_SkipRenderCommand(data2);
						renderingHud2 = qtrue;
						break;

					case RC_CLEARDEPTH:
						goto renderpass1huddone;

					case RC_END_OF_LIST:
						//dprintf("render1 hud commands stop ------------------------\n");
						goto renderpass1huddone;
					default:
						//ri.Printf(PRINT_ALL, "^1RB_ExecuteRenderCommands():  unknown render command: %d\n", *(const int *)data2);
						data2 = RB_SkipRenderCommand(data2);
						break;
					}
				}
			renderpass1huddone:

				RB_QLPostProcessing();

				RB_SwapBuffers(NULL, qfalse);

				//ri.Printf(PRINT_ALL, "^3about to record...\n");
				//FIXME hack
				cmd.width = ri.afdLeft->width;
				cmd.height = ri.afdLeft->height;
				cmd.captureBuffer = ri.afdLeft->cBuffer;
				cmd.encodeBuffer = ri.afdLeft->eBuffer;
				cmd.motionJpeg = (ri.afdLeft->codec == CODEC_MJPEG);
				cmd.avi = ri.afdLeft->avi;
				cmd.tga = ri.afdLeft->tga;
				cmd.jpg = ri.afdLeft->jpg;
				cmd.png = ri.afdLeft->png;
				cmd.picCount = ri.afdMain->picCount - 1;
				Q_strncpyz(cmd.givenFileName, ri.afdMain->givenFileName, MAX_QPATH);
				RB_TakeVideoFrameCmd(&cmd, &shotDataLeft);
				tr.leftRecorded = qtrue;
			}

			data = RB_ClearDepth(data);
			depthWasCleared = qtrue;
			renderingHud = qfalse;
			break;

		case RC_DRAW_BUFFER:
			dprintf("r1 drawbuffer\n");
			data = RB_DrawBuffer( data );
			if (r_anaglyphMode->integer) {
				qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

				backEnd.colorMask[0] = GL_FALSE;
				backEnd.colorMask[1] = GL_FALSE;
				backEnd.colorMask[2] = GL_FALSE;
				backEnd.colorMask[3] = GL_FALSE;

				qglClear(GL_COLOR_BUFFER_BIT);
			}
			//data = RB_SkipRenderCommand(data);
			break;

		case RC_SWAP_BUFFERS:
			//dprintf("r1 swapbuffers\n");
			//data = RB_SwapBuffers(data, qtrue);
			data = RB_SkipRenderCommand(data);
			break;

		case RC_COLORMASK:
			cmd = data;
			dprintf("r1 colormask %d %d %d %d\n", cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
			rgba[0] = cmd->rgba[0];
			rgba[1] = cmd->rgba[1];
			rgba[2] = cmd->rgba[2];
			rgba[3] = cmd->rgba[3];
			//ri.Printf(PRINT_ALL, "mask %d %d %d %d\n", rgba[0], rgba[1], rgba[2], rgba[3]);
			data = RB_ColorMask(data);
			colorMaskSet = qtrue;
			break;

		case RC_CAPSHADOWMAP:
			data = RB_CapShadowMap(data);
			break;
		case RC_POSTPROCESS:
			data = RB_PostProcess(data);
			//RB_QLPostProcessing();
			//count++;
			//ri.Printf(PRINT_ALL, "^5ql postprocessing %d\n", count);
			break;
		case RC_EXPORT_CUBEMAPS:
			data = RB_ExportCubemaps(data);
			break;

		case RC_BEGIN_HUD:
			data = RB_SkipRenderCommand(data);
			renderingHud = qtrue;
			break;

		case RC_DEBUG_GRAPHICS:
			data = RB_DebugGraphics(data);
			break;
		case RC_END_OF_LIST:
			goto firstpassdone;
		default:
			data = RB_SkipRenderCommand(data);
			break;
		}
	}

 firstpassdone:

	if (colorMaskSet) {
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		backEnd.colorMask[0] = GL_FALSE;
		backEnd.colorMask[1] = GL_FALSE;
		backEnd.colorMask[2] = GL_FALSE;
		backEnd.colorMask[3] = GL_FALSE;
	}

	// video command is in it's own render list, so drawing doesn't always
	// take place

	/* Take and merge DOF frames */
	if ((tr.recordingVideo  ||  mme_dofVisualize->integer)  &&  tr.drawSurfsCount) {
		if (R_MME_MultiPassNext(qtrue)) {
			R_InitNextFrameNoCommands();
			goto videoCommandCheckDone;
		}

		// blit q3mme dof
		if (mme_dofFrames->integer > 1  &&  (tr.recordingVideo  ||  mme_dofVisualize->integer)  &&  R_MME_GetPassData(qtrue)  &&  !shotDataMain.allocFailed) {
			byte *buffer;
			//int i, j;

			buffer = R_MME_GetPassData(qtrue);

#if 0  // testing
			for (i = 0;  i < glConfig.vidHeight;  i++) {
				for (j = 0;  j < glConfig.vidWidth;  j++) {
					byte *p;
					p = buffer + i * glConfig.vidWidth * 3 + j * 3;
					if (j == glConfig.vidWidth / 2) {
						p[0] = 255;
						p[1] = 0;
						p[2] = 0;
					} else {
						//p[0] = 0;
						//p[1] = 0;
						//p[2] = 0;
					}
				}
			}
#endif

			RE_StretchRawRectScreen(buffer);
		}
	}

	if (tr.drawSurfsCount) {
		// screen map texture
		if (tr.needScreenMap) {
			vec4_t quadVerts[4];
			vec2_t texCoords[4];
			int x, y, w, h;

			if (glRefConfig.framebufferObject) {
				if (tr.usingFinalFrameBufferObject) {
					FBO_Bind(tr.finalFbo);
				} else {
					//FIXME wolfcam: GL_BindNullFramebuffers() doesn't work, it shows a black screen in the first main menu, if you enter another menu the rendering is correct
					FBO_Bind(NULL);
				}
			}

			RB_SetGL2D();

			// get original area that is going to be ovewritten
			qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
			qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);

			GL_State(GLS_DEPTHTEST_DISABLE);
			qglDepthMask(GL_FALSE);

			GL_BindMultiTexture(GL_TEXTURE0_ARB, GL_TEXTURE_2D, tr.screenMapImageScratchBuffer->texnum);
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.screenMapImageScratchBuffer->uploadWidth, tr.screenMapImageScratchBuffer->uploadHeight);

			// get big screen image, note: it might clip parts since the texture might not be as big as the screen
			//FIXME bindtotmmu
			GL_BindMultiTexture(GL_TEXTURE0_ARB, GL_TEXTURE_2D, tr.screenMapFullImage->texnum);
			// this will be a flipped image, but it is flipped again by the other call to glCopyTexSubImage2D()
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.screenMapFullImage->uploadWidth, tr.screenMapFullImage->uploadHeight);

			// draw scaled down version
			x = 0;
			y = 0;
			w = tr.screenMapFullImage->uploadWidth;
			h = tr.screenMapFullImage->uploadHeight;

			qglViewport(0, 0, tr.screenMapImage->uploadWidth, tr.screenMapImage->uploadHeight);

			VectorSet4(quadVerts[0], x, y, 0, 1);
			VectorSet4(quadVerts[1], x + w, y, 0, 1);
			VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
			VectorSet4(quadVerts[3], x, y + h, 0, 1);

			RB_InstantQuad(quadVerts);

			// get scaled version
			GL_BindMultiTexture(GL_TEXTURE0_ARB, GL_TEXTURE_2D, tr.screenMapImage->texnum);
			// this is flipped back
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, tr.screenMapImage->uploadWidth, tr.screenMapImage->uploadHeight);

			// restore original image, need to flip
			qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
			GL_BindMultiTexture(GL_TEXTURE0_ARB, GL_TEXTURE_2D, tr.screenMapImageScratchBuffer->texnum);
			//GL_BindMultiTexture(GL_TEXTURE0_ARB, GL_TEXTURE_2D, tr.whiteImage->texnum);

			x = 0;
			y = glConfig.vidHeight - tr.screenMapImageScratchBuffer->uploadHeight;
			w = tr.screenMapImageScratchBuffer->uploadWidth;
			h = tr.screenMapImageScratchBuffer->uploadHeight;

			VectorSet4(quadVerts[0], x, y, 0, 1);
			VectorSet4(quadVerts[1], x + w, y, 0, 1);
			VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
			VectorSet4(quadVerts[3], x, y + h, 0, 1);

			VectorSet2(texCoords[0], 0.0f, 1.0f);
			VectorSet2(texCoords[1], 1.0f, 1.0f);
			VectorSet2(texCoords[2], 1.0f, 0.0f);
			VectorSet2(texCoords[3], 0.0f, 0.0f);

			// can't use tr.textureColorShader since, even with colorWhite,
			// there's slight color differences
			GLSL_BindProgram(&tr.textureNoColorShader);

			GLSL_SetUniformMat4(&tr.textureNoColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
			//GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);
			//GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorRed);

			RB_InstantQuad2(quadVerts, texCoords);

			// done
			qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
			GL_BindMultiTexture(GL_TEXTURE0_ARB, GL_TEXTURE_2D, 0);
			backEnd.projection2D = qfalse;
		}
		if (r_anaglyphMode->integer) {
			if (depthWasCleared) {
				RB_QLPostProcessing();
			} else {
				//ri.Printf(PRINT_ALL, "skipping\n");
				//ri.Printf(PRINT_ALL, "3d 1stpass done\n");
			}
		} else {
			RB_QLPostProcessing();
		}
	}

	if (colorMaskSet  &&  r_anaglyph2d->integer) {
		qglColorMask(rgba[0], rgba[1], rgba[2], rgba[3]);

		backEnd.colorMask[0] = !rgba[0];
		backEnd.colorMask[1] = !rgba[1];
		backEnd.colorMask[2] = !rgba[2];
		backEnd.colorMask[3] = !rgba[3];
	}

	// draw hud
	renderingHud = qfalse;

	RB_SetGL2D();

	data = dataOrig;
	dprintf("render2 commands start ------------------------------\n");
	while ( 1 ) {
		data = PADP(data, sizeof(void *));
		switch ( *(const int *)data ) {
		case RC_BEGIN_HUD:
			data = RB_SkipRenderCommand(data);
			renderingHud = qtrue;
			break;
		case RC_SET_COLOR:
			//dprintf("r2 setcolor\n");
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			//dprintf("r2 stretchpic\n");
			data = RB_StretchPic( data );
			//data = RB_SkipRenderCommand(data);
			break;
		case RC_DRAW_SURFS:
			//dprintf("r2 drawsurfs\n");
			if (renderingHud) {
				data = RB_DrawSurfs(data);  // 3d hud models
				tr.drawSurfsCount++;
				// hack to set rendering mode back correctly
				RB_SetGL2D();
			} else {
				data = RB_SkipRenderCommand(data);
			}
			break;
		case RC_DRAW_BUFFER:
			//dprintf("r2 drawbuffer\n");
			//data = RB_DrawBuffer( data );
			data = RB_SkipRenderCommand(data);
			break;
		case RC_SWAP_BUFFERS:
			dprintf("r2 swapbuffers\n");
			data = RB_SwapBuffers(data, qtrue);
			//data = RB_SkipRenderCommand(data);
			break;
		case RC_SCREENSHOT:
			//dprintf("r2 takescreenshot\n");
			//data = RB_TakeScreenshotCmd( data );
			data = RB_SkipRenderCommand(data);
			break;
		case RC_VIDEOFRAME:
			//dprintf("r2 takevideoframe\n");
			//data = RB_TakeVideoFrameCmd( data );
			data = RB_SkipRenderCommand(data);
			break;
		case RC_COLORMASK:
			//dprintf("r2 colormask\n");
			//data = RB_ColorMask(data);
			data = RB_SkipRenderCommand(data);
			break;
		case RC_CLEARDEPTH:
			//dprintf("r2 cleardepth\n");
			//ri.Printf(PRINT_ALL, "r2 cleardepth\n");
			//data = RB_ClearDepth(data);
			data = RB_SkipRenderCommand(data);
			renderingHud = qfalse;
			break;
		case RC_CAPSHADOWMAP:
			//data = RB_CapShadowMap(data);
			data = RB_SkipRenderCommand(data);
			break;
		case RC_POSTPROCESS:
			//data = RB_PostProcess(data);
			//RB_QLPostProcessing();
			data = RB_SkipRenderCommand(data);
			//count++;
			//ri.Printf(PRINT_ALL, "^5ql postprocessing %d\n", count);
			break;
		case RC_EXPORT_CUBEMAPS:
			//data = RB_ExportCubemaps(data);
			data = RB_SkipRenderCommand(data);
			break;
		case RC_DEBUG_GRAPHICS:
			data = RB_SkipRenderCommand(data);
			break;

		case RC_END_OF_LIST:
			//dprintf("render2 commands stop ------------------------\n");
			// stop rendering
			//t2 = ri.RealMilliseconds();
			//backEnd.pc.msec = t2 - t1;
			goto secondpassdone;
		default:
			ri.Printf(PRINT_ALL, "^1RB_ExecuteRenderCommands():  unknown render command: %d\n", *(const int *)data);
			data = RB_SkipRenderCommand(data);
			break;
		}
	}

 secondpassdone:

	data = dataOrig;
	dprintf("render3 commands start -------------------------\n");
	while ( 1 ) {
		data = PADP(data, sizeof(void *));
		switch (*(const int *)data) {
		case RC_SCREENSHOT:
			dprintf("r3 takescreenshot\n");
			data = RB_TakeScreenshotCmd( data );
			break;
		case RC_VIDEOFRAME:
			dprintf("r3 takevideoframe\n");
			//ri.Printf(PRINT_ALL, "r3 takevideoframe\n");
			data = RB_TakeVideoFrameCmd(data, &shotDataMain);
			break;
		case RC_END_OF_LIST:
			t2 = ri.RealMilliseconds();
			backEnd.pc.msec = t2 - t1;
			goto thirdpassdone;
		default:
			data = RB_SkipRenderCommand(data);
			break;
		}
	}

 thirdpassdone:

#if 0
	if (tr.usingMultiSample) {
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tr.frameBufferMultiSample);
	} else {
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, tr.frameBuffer);
	}
#endif

	//ri.Printf(PRINT_ALL, "done.\n");
	return;

}
