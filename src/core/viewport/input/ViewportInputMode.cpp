///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/input/ViewportInputManager.h>
#include <core/viewport/input/ViewportInputMode.h>
#include <core/viewport/input/NavigationModes.h>

namespace Ovito {

/******************************************************************************
* This is called by the system after the input handler has become the active handler.
******************************************************************************/
void ViewportInputMode::activated()
{
	Q_EMIT statusChanged(true);
}

/******************************************************************************
* This is called by the system after the input handler is no longer the active handler.
******************************************************************************/
void ViewportInputMode::deactivated()
{
	Q_EMIT statusChanged(false);
}

/******************************************************************************
* Checks whether this mode is currently active.
******************************************************************************/
bool ViewportInputMode::isActive() const
{
	if(!_manager) return false;
	return _manager->activeMode() == this;
}

/******************************************************************************
* Activates the given temporary navigation mode.
******************************************************************************/
void ViewportInputMode::activateTemporaryNavigationMode(ViewportInputMode* mode)
{
	inputManager()->pushInputMode(mode);
}

/******************************************************************************
* Sets the mouse cursor shown in the viewport windows
* while this input handler is active.
******************************************************************************/
void ViewportInputMode::setCursor(const QCursor& cursor)
{
	_cursor = cursor;
	Q_EMIT curserChanged();
}

/******************************************************************************
* Handles the mouse down event for the given viewport.
******************************************************************************/
void ViewportInputMode::mousePressEvent(Viewport* vp, QMouseEvent* event)
{
	_lastMousePressEvent.reset();
	if(event->button() == Qt::RightButton) {
		if(modeType() != ExclusiveMode) {
			inputManager()->removeInputMode(this);
		}
		else {
			activateTemporaryNavigationMode(inputManager()->panMode());
			if(inputManager()->activeMode() == inputManager()->panMode())
				inputManager()->activeMode()->mousePressEvent(vp, event);
		}
		event->accept();
	}
	else if(event->button() == Qt::LeftButton) {
		_lastMousePressEvent.reset(new QMouseEvent(event->type(), event->localPos(), event->windowPos(), event->screenPos(), event->button(), event->buttons(), event->modifiers()));
		event->accept();
	}
	else if(event->button() == Qt::MidButton) {
		activateTemporaryNavigationMode(inputManager()->panMode());
		if(inputManager()->activeMode() == inputManager()->panMode())
			inputManager()->activeMode()->mousePressEvent(vp, event);
		event->accept();
	}
}

/******************************************************************************
* Handles the mouse up event for the given viewport.
******************************************************************************/
void ViewportInputMode::mouseReleaseEvent(Viewport* vp, QMouseEvent* event)
{
	_lastMousePressEvent.reset();
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void ViewportInputMode::mouseMoveEvent(Viewport* vp, QMouseEvent* event)
{
	if(_lastMousePressEvent && (event->pos() - _lastMousePressEvent->pos()).manhattanLength() > 2) {
		if(this != inputManager()->orbitMode()) {
			activateTemporaryNavigationMode(inputManager()->orbitMode());
			if(inputManager()->activeMode() == inputManager()->orbitMode())
				inputManager()->activeMode()->mousePressEvent(vp, _lastMousePressEvent.get());
		}
		_lastMousePressEvent.reset();
	}
}

/******************************************************************************
* Handles the mouse wheel event for the given viewport.
******************************************************************************/
void ViewportInputMode::wheelEvent(Viewport* vp, QWheelEvent* event)
{
	_lastMousePressEvent.reset();
	inputManager()->zoomMode()->zoom(vp, (FloatType)event->delta());
	event->accept();
}

/******************************************************************************
* Handles the mouse double-click events for the given viewport.
******************************************************************************/
void ViewportInputMode::mouseDoubleClickEvent(Viewport* vp, QMouseEvent* event)
{
	_lastMousePressEvent.reset();
	if(event->button() == Qt::LeftButton) {
		inputManager()->pickOrbitCenterMode()->pickOrbitCenter(vp, event->pos());
		event->accept();
	}
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void ViewportInputMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
//	if(_showOrbitCenter && isActive())
//		OrbitMode::instance()->renderOverlay3D(vp, renderer, isActive);
}

/******************************************************************************
* Computes the bounding box of the visual viewport overlay rendered by the input mode.
******************************************************************************/
Box3 ViewportInputMode::overlayBoundingBox(Viewport* vp, ViewportSceneRenderer* renderer)
{
	Box3 bb;
//	if(_showOrbitCenter && isActive())
//		bb.addBox(OrbitMode::instance()->overlayBoundingBox(vp, renderer, isActive));
	return bb;
}

};
