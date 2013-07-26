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
#include "BondsObject.h"

namespace Viz {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Viz, BondsObject, SceneObject)

/******************************************************************************
* Default constructor.
******************************************************************************/
BondsObject::BondsObject(BondsStorage* storage)
	: _storage(storage ? storage : new BondsStorage())
{
}

/******************************************************************************
* Replaces the internal storage object with the given one.
******************************************************************************/
void BondsObject::setStorage(BondsStorage* storage)
{
	OVITO_CHECK_POINTER(storage);
	_storage = storage;
	changed();
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void BondsObject::saveToStream(ObjectSaveStream& stream)
{
	SceneObject::saveToStream(stream);

	stream.beginChunk(0x01);
	_storage.constData()->saveToStream(stream, !saveWithScene());
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void BondsObject::loadFromStream(ObjectLoadStream& stream)
{
	SceneObject::loadFromStream(stream);

	stream.expectChunk(0x01);
	_storage->loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> BondsObject::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<BondsObject> clone = static_object_cast<BondsObject>(SceneObject::clone(deepCopy, cloneHelper));

	// Shallow copy storage.
	clone->_storage = this->_storage;

	return clone;
}

};
