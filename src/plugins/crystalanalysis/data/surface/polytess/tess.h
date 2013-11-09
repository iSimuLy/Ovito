/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
** Author: Eric Veach, July 1994.
**
** $Date: 2001/03/17 00:25:41 $ $Revision: 1.1 $
** $Header: /home/krh/git/sync/mesa-cvs-repo/Mesa/src/glu/sgi/libtess/tess.h,v 1.1 2001/03/17 00:25:41 brianp Exp $
*/

#ifndef __tess_h_
#define __tess_h_

#include "glu.h"
#include <setjmp.h>
#include "mesh.h"
#include "dict.h"
#include "priorityq.h"

/* The begin/end calls must be properly nested.  We keep track of
 * the current state to enforce the ordering.
 */
enum TessState { T_DORMANT, T_IN_POLYGON, T_IN_CONTOUR };

/* We cache vertex data for single-contour polygons so that we can
 * try a quick-and-dirty decomposition first.
 */
#define TESS_MAX_CACHE	100

typedef struct CachedVertex {
  double	coords[3];
  void		*data;
} CachedVertex;

struct GLUtesselator {

  /*** state needed for collecting the input data ***/

  enum TessState state;		/* what begin/end calls have we seen? */

  GLUhalfEdge	*lastEdge;	/* lastEdge->Org is the most recent vertex */
  GLUmesh	*mesh;		/* stores the input contours, and eventually
                                   the tessellation itself */

  void		(*callError)( int errnum );

  /*** state needed for projecting onto the sweep plane ***/

  double	normal[3];	/* user-specified normal (if provided) */
  double	sUnit[3];	/* unit vector in s-direction (debugging) */
  double	tUnit[3];	/* unit vector in t-direction (debugging) */

  /*** state needed for the line sweep ***/

  double	relTolerance;	/* tolerance for merging features */
  int	windingRule;	/* rule for determining polygon interior */
  tessbool	fatalError;	/* fatal error: needed combine callback */

  Dict		*dict;		/* edge dictionary for sweep line */
  PriorityQ	*pq;		/* priority queue of vertex events */
  GLUvertex	*event;		/* current sweep event being processed */

  void		(*callCombine)( double coords[3], void *data[4],
			        float weight[4], void **outData );

  /*** state needed for rendering callbacks (see render.c) ***/

  tessbool	flagBoundary;	/* mark boundary edges (use EdgeFlag) */
  tessbool	boundaryOnly;	/* Extract contours, not triangles */
  GLUface	*lonelyTriList;
    /* list of triangles which could not be rendered as strips or fans */

  void		(*callBegin)( int type );
  void		(*callEdgeFlag)( tessbool boundaryEdge );
  void		(*callVertex)( void *data );
  void		(*callEnd)( void );
  void		(*callMesh)( GLUmesh *mesh );


  /*** state needed to cache single-contour polygons for renderCache() */

  tessbool	emptyCache;		/* empty cache on next vertex() call */
  int		cacheCount;		/* number of cached vertices */
  CachedVertex	cache[TESS_MAX_CACHE];	/* the vertex data */

  /*** rendering callbacks that also pass polygon data  ***/ 
  void		(*callBeginData)( int type, void *polygonData );
  void		(*callEdgeFlagData)( tessbool boundaryEdge,
				     void *polygonData );
  void		(*callVertexData)( void *data, void *polygonData );
  void		(*callEndData)( void *polygonData );
  void		(*callErrorData)( int errnum, void *polygonData );
  void		(*callCombineData)( double coords[3], void *data[4],
				    float weight[4], void **outData,
				    void *polygonData );

  jmp_buf env;			/* place to jump to when memAllocs fail */

  void *polygonData;		/* client data for current polygon */
};

void __gl_noBeginData( int type, void *polygonData );
void __gl_noEdgeFlagData( tessbool boundaryEdge, void *polygonData );
void __gl_noVertexData( void *data, void *polygonData );
void __gl_noEndData( void *polygonData );
void __gl_noErrorData( int errnum, void *polygonData );
void __gl_noCombineData( double coords[3], void *data[4],
			 float weight[4], void **outData,
			 void *polygonData );

#define CALL_BEGIN_OR_BEGIN_DATA(a) \
   if (tess->callBeginData != &__gl_noBeginData) \
      (*tess->callBeginData)((a),tess->polygonData); \
   else (*tess->callBegin)((a));

#define CALL_VERTEX_OR_VERTEX_DATA(a) \
   if (tess->callVertexData != &__gl_noVertexData) \
      (*tess->callVertexData)((a),tess->polygonData); \
   else (*tess->callVertex)((a));

#define CALL_EDGE_FLAG_OR_EDGE_FLAG_DATA(a) \
   if (tess->callEdgeFlagData != &__gl_noEdgeFlagData) \
      (*tess->callEdgeFlagData)((a),tess->polygonData); \
   else (*tess->callEdgeFlag)((a));

#define CALL_END_OR_END_DATA() \
   if (tess->callEndData != &__gl_noEndData) \
      (*tess->callEndData)(tess->polygonData); \
   else (*tess->callEnd)();

#define CALL_COMBINE_OR_COMBINE_DATA(a,b,c,d) \
   if (tess->callCombineData != &__gl_noCombineData) \
      (*tess->callCombineData)((a),(b),(c),(d),tess->polygonData); \
   else (*tess->callCombine)((a),(b),(c),(d));

#define CALL_ERROR_OR_ERROR_DATA(a) \
   if (tess->callErrorData != &__gl_noErrorData) \
      (*tess->callErrorData)((a),tess->polygonData); \
   else (*tess->callError)((a));

#endif
