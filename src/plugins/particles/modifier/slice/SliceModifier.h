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

#ifndef __OVITO_SLICE_MODIFIER_H
#define __OVITO_SLICE_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <core/animation/controller/Controller.h>
#include <core/viewport/input/ViewportInputMode.h>
#include <core/viewport/input/ViewportInputManager.h>
#include <core/gui/actions/ViewportModeAction.h>
#include <plugins/particles/util/ParticlePickingHelper.h>
#include "../ParticleModifier.h"

namespace Particles {

using namespace Ovito;

/******************************************************************************
* The slice modifier deletes all particles on one side of a plane.
******************************************************************************/
class OVITO_PARTICLES_EXPORT SliceModifier : public ParticleModifier
{
public:

	/// Default constructor.
	Q_INVOKABLE SliceModifier();

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Lets the modifier render itself into the viewport.
	virtual void render(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay) override;

	/// Computes the bounding box of the visual representation of the modifier.
	virtual Box3 boundingBox(TimePoint time,  ObjectNode* contextNode, ModifierApplication* modApp) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp) override;

	// Property access functions:

	/// Returns the plane's distance from the origin.
	FloatType distance() const { return _distanceCtrl ? _distanceCtrl->currentValue() : (FloatType)0; }

	/// Sets the plane's distance from the origin.
	void setDistance(FloatType newDistance) { if(_distanceCtrl) _distanceCtrl->setCurrentValue(newDistance); }

	/// Returns the controller for the plane distance.
	FloatController* distanceController() const { return _distanceCtrl; }

	/// Sets the controller for the plane distance.
	void setDistanceController(const OORef<FloatController>& ctrl) { _distanceCtrl = ctrl; }

	/// Returns the plane's normal vector.
	Vector3 normal() const { return _normalCtrl ? _normalCtrl->currentValue() : Vector3(0,0,1); }

	/// Sets the plane's distance from the origin.
	void setNormal(const Vector3& newNormal) { if(_normalCtrl) _normalCtrl->setCurrentValue(newNormal); }

	/// Returns the controller for the plane normal.
	VectorController* normalController() const { return _normalCtrl; }

	/// Sets the controller for the plane normal.
	void setVectorController(const OORef<VectorController>& ctrl) { _normalCtrl = ctrl; }

	/// Returns the slice width.
	FloatType sliceWidth() const { return _widthCtrl ? _widthCtrl->currentValue() : (FloatType)0; }

	/// Sets the slice width.
	void setSliceWidth(FloatType newWidth) { if(_widthCtrl) _widthCtrl->setCurrentValue(newWidth); }

	/// Returns the controller for the slice width.
	FloatController* sliceWidthController() const { return _widthCtrl; }

	/// Sets the controller for the slice width.
	void setSliceWidthController(const OORef<FloatController>& ctrl) { _widthCtrl = ctrl; }

	/// Returns whether the plane's orientation should be flipped.
	bool inverse() const { return _inverse; }

	/// Sets whether the plane's orientation should be flipped.
	void setInverse(bool inverse) { _inverse = inverse; }

	/// Returns whether the atoms are only selected instead of deleted.
	bool createSelection() const { return _createSelection; }

	/// Sets whether the atoms are only selected instead of deleted.
	void setCreateSelection(bool select) { _createSelection = select; }

	/// Returns whether the modifier is only applied to the currently selected atoms.
	bool applyToSelection() const { return _applyToSelection; }

	/// Sets whether the modifier should only be applied to the currently selected atoms.
	void setApplyToSelection(bool flag) { _applyToSelection = flag; }

	/// Returns the slicing plane.
	Plane3 slicingPlane(TimePoint time, TimeInterval& validityInterval);

public:

	Q_PROPERTY(FloatType distance READ distance WRITE setDistance)
	Q_PROPERTY(Vector3 normal READ normal WRITE setNormal)
	Q_PROPERTY(FloatType sliceWidth READ sliceWidth WRITE setSliceWidth)
	Q_PROPERTY(bool inverse READ inverse WRITE setInverse)
	Q_PROPERTY(bool createSelection READ createSelection WRITE setCreateSelection)
	Q_PROPERTY(bool applyToSelection READ applyToSelection WRITE setApplyToSelection)

protected:

	/// Modifies the particle object.
	virtual ObjectStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// Performs the actual rejection of particles.
	size_t filterParticles(std::vector<bool>& mask, TimePoint time, TimeInterval& validityInterval);

	/// \brief Renders the modifier's visual representation and computes its bounding box.
	Box3 renderVisual(TimePoint time, ObjectNode* contextNode, SceneRenderer* renderer);

	/// Renders the plane in the viewport.
	Box3 renderPlane(SceneRenderer* renderer, const Plane3& plane, const Box3& box, const ColorA& color) const;

	/// Computes the intersection lines of a plane and a quad.
	void planeQuadIntersection(const Point3 corners[8], const std::array<int,4>& quadVerts, const Plane3& plane, QVector<Point3>& vertices) const;

	/// This controller stores the normal of the slicing plane.
	ReferenceField<VectorController> _normalCtrl;

	/// This controller stores the distance of the slicing plane from the origin.
	ReferenceField<FloatController> _distanceCtrl;

	/// Controls the slice width.
	ReferenceField<FloatController> _widthCtrl;

	/// Controls whether the atoms should only be selected instead of deleted.
	PropertyField<bool> _createSelection;

	/// Controls whether the selection/plane orientation should be inverted.
	PropertyField<bool> _inverse;

	/// Controls whether the modifier should only be applied to the currently selected atoms.
	PropertyField<bool> _applyToSelection;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Slice");
	Q_CLASSINFO("ModifierCategory", "Modification");

	DECLARE_REFERENCE_FIELD(_normalCtrl);
	DECLARE_REFERENCE_FIELD(_distanceCtrl);
	DECLARE_REFERENCE_FIELD(_widthCtrl);
	DECLARE_PROPERTY_FIELD(_createSelection);
	DECLARE_PROPERTY_FIELD(_inverse);
	DECLARE_PROPERTY_FIELD(_applyToSelection);
};

class SliceModifierEditor;

/******************************************************************************
* The viewport input mode that lets the user select three particles
* to define the slicing plane.
******************************************************************************/
class PickParticlePlaneInputMode : public ViewportInputMode, ParticlePickingHelper
{
public:

	/// Constructor.
	PickParticlePlaneInputMode(SliceModifierEditor* editor) : _editor(editor) {}

	/// Returns the activation behavior of this input handler.
	virtual InputHandlerType handlerType() override { return ViewportInputMode::NORMAL; }

	/// Handles the mouse events for a Viewport.
	virtual void mouseReleaseEvent(Viewport* vp, QMouseEvent* event) override;

	/// Lets the input mode render its overlay content in a viewport.
	virtual void renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer, bool isActive) override;

	/// Indicates whether this input mode renders into the viewports.
	virtual bool hasOverlay() override { return true; }

protected:

	/// This is called by the system after the input handler has become the active handler.
	virtual void activated() override;

	/// This is called by the system after the input handler is no longer the active handler.
	virtual void deactivated() override;

private:

	/// Aligns the modifier's slicing plane to the three selected particles.
	void alignPlane(SliceModifier* mod);

	/// The list of particles picked by the user so far.
	QVector<PickResult> _pickedParticles;

	/// The properties editor of the Slice modifier.
	SliceModifierEditor* _editor;
};


/******************************************************************************
* A properties editor for the SliceModifier class.
******************************************************************************/
class SliceModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE SliceModifierEditor() {}

	/// Destructor.
	virtual ~SliceModifierEditor() {
		// Deactivate the input mode when editor is closed.
		ViewportInputManager::instance().removeInputHandler(_pickParticlePlaneInputMode.get());
	}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Aligns the slicing plane to the viewing direction.
	void onAlignPlaneToView();

	/// Aligns the current viewing direction to the slicing plane.
	void onAlignViewToPlane();

	/// Aligns the normal of the slicing plane with the X, Y, or Z axis.
	void onXYZNormal(const QString& link);

	/// Moves the plane to the center of the simulation box.
	void onCenterOfBox();

private:

	OORef<PickParticlePlaneInputMode> _pickParticlePlaneInputMode;
	ViewportModeAction* _pickParticlePlaneInputModeAction;

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_SLICE_MODIFIER_H
