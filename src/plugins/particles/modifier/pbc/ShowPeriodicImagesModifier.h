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

#ifndef __OVITO_SHOW_PERIODIC_IMAGES_MODIFIER_H
#define __OVITO_SHOW_PERIODIC_IMAGES_MODIFIER_H

#include <plugins/particles/Particles.h>
#include "../ParticleModifier.h"

namespace Particles {

using namespace Ovito;

/**
 * \brief This modifier duplicates all atoms multiple times and shifts them by
 *        one of the simulation cell vectors to visualize the periodic images.
 *
 * \author Alexander Stukowski
 */
class OVITO_PARTICLES_EXPORT ShowPeriodicImagesModifier : public ParticleModifier
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ShowPeriodicImagesModifier();

protected:

	/// Modifies the particle object. The time interval passed
	/// to the function is reduced to the interval where the modified object is valid/constant.
	virtual ObjectStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Controls whether the periodic images are shown in the X direction.
	PropertyField<bool> _showImageX;
	/// Controls whether the periodic images are shown in the Y direction.
	PropertyField<bool> _showImageY;
	/// Controls whether the periodic images are shown in the Z direction.
	PropertyField<bool> _showImageZ;

	/// Controls the number of periodic images shown in the X direction.
	PropertyField<int> _numImagesX;
	/// Controls the number of periodic images shown in the Y direction.
	PropertyField<int> _numImagesY;
	/// Controls the number of periodic images shown in the Z direction.
	PropertyField<int> _numImagesZ;

	/// Controls whether the size of the simulation box is adjusted to the extended system.
	PropertyField<bool> _adjustBoxSize;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Show periodic images");
	Q_CLASSINFO("ModifierCategory", "Modification");

	DECLARE_PROPERTY_FIELD(_showImageX);
	DECLARE_PROPERTY_FIELD(_showImageY);
	DECLARE_PROPERTY_FIELD(_showImageZ);
	DECLARE_PROPERTY_FIELD(_numImagesX);
	DECLARE_PROPERTY_FIELD(_numImagesY);
	DECLARE_PROPERTY_FIELD(_numImagesZ);
	DECLARE_PROPERTY_FIELD(_adjustBoxSize);
};

/**
 * \brief A properties editor for the ShowPeriodicImagesModifier class.
 *
 * This editor class creates and manages the user interface through which the
 * user can alter the modifier's parameters.
 */
class ShowPeriodicImagesModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE ShowPeriodicImagesModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_SHOW_PERIODIC_IMAGES_MODIFIER_H
