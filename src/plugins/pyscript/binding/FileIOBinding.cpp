///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <plugins/pyscript/PyScript.h>
#include <core/dataset/importexport/ImportExportManager.h>
#include <core/dataset/importexport/FileImporter.h>
#include <core/dataset/importexport/FileExporter.h>
#include <core/dataset/importexport/LinkedFileImporter.h>
#include <core/dataset/importexport/LinkedFileObject.h>
#include <core/utilities/io/FileManager.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace boost::python;
using namespace Ovito;

void setupFileIOBinding()
{
	class_<QUrl>("QUrl", init<>())
		.def(init<const QString&>())
		.add_property("errorString", &QUrl::errorString)
		.add_property("isEmpty", &QUrl::isEmpty)
		.add_property("isLocalFile", &QUrl::isLocalFile)
		.add_property("isValid", &QUrl::isValid)
		.add_property("localFile", &QUrl::toLocalFile)
	;

	class_<FileImporter, bases<RefTarget>, OORef<FileImporter>, boost::noncopyable>("FileImporter", no_init)
		.add_property("fileFilter", &FileImporter::fileFilter)
		.add_property("fileFilterDescription", &FileImporter::fileFilterDescription)
		.def("importFile", &FileImporter::importFile)
		.def("checkFileFormat", &FileImporter::checkFileFormat)
	;

	enum_<FileImporter::ImportMode>("ImportMode")
		.value("AskUser", FileImporter::AskUser)
		.value("AddToScene", FileImporter::AddToScene)
		.value("ReplaceSelected", FileImporter::ReplaceSelected)
		.value("ResetScene", FileImporter::ResetScene)
	;

	class_<ImportExportManager, boost::noncopyable>("ImportExportManager", no_init)
		.add_static_property("instance", make_function(&ImportExportManager::instance, return_value_policy<reference_existing_object>()))
		.def("autodetectFileFormat", (OORef<FileImporter> (ImportExportManager::*)(DataSet*, const QUrl&))&ImportExportManager::autodetectFileFormat)
	;

	class_<FileManager, boost::noncopyable>("FileManager", no_init)
		.add_static_property("instance", make_function(&FileManager::instance, return_value_policy<reference_existing_object>()))
		.def("removeFromCache", &FileManager::removeFromCache)
		.def("urlFromUserInput", &FileManager::urlFromUserInput)
	;

	class_<LinkedFileImporter, bases<FileImporter>, OORef<LinkedFileImporter>, boost::noncopyable>("LinkedFileImporter", no_init)
		.def("requestReload", &LinkedFileImporter::requestReload)
		.def("requestFramesUpdate", &LinkedFileImporter::requestFramesUpdate)
	;

	class_<FileExporter, bases<RefTarget>, OORef<FileExporter>, boost::noncopyable>("FileExporter", no_init)
		.add_property("fileFilter", &FileExporter::fileFilter)
		.add_property("fileFilterDescription", &FileExporter::fileFilterDescription)
		.def("exportToFile", &FileExporter::exportToFile)
	;

	class_<LinkedFileObject, bases<SceneObject>, OORef<LinkedFileObject>, boost::noncopyable>("LinkedFileObject", init<DataSet*>())
		.add_property("importer", make_function(&LinkedFileObject::importer, return_value_policy<ovito_object_reference>()))
	// Add setter method.
		.add_property("sourceUrl", make_function(&LinkedFileObject::sourceUrl, return_value_policy<copy_const_reference>()))
		.add_property("status", &LinkedFileObject::status)
		.add_property("numberOfFrames", &LinkedFileObject::numberOfFrames)
		.add_property("loadedFrame", &LinkedFileObject::loadedFrame)
		.add_property("adjustAnimationIntervalEnabled", &LinkedFileObject::adjustAnimationIntervalEnabled, &LinkedFileObject::setAdjustAnimationIntervalEnabled)
		.add_property("sceneObjects", make_function(&LinkedFileObject::sceneObjects, return_internal_reference<>()))
		.def("refreshFromSource", &LinkedFileObject::refreshFromSource)
		.def("updateFrames", &LinkedFileObject::updateFrames)
		.def("animationTimeToInputFrame", &LinkedFileObject::animationTimeToInputFrame)
		.def("inputFrameToAnimationTime", &LinkedFileObject::inputFrameToAnimationTime)
		.def("adjustAnimationInterval", &LinkedFileObject::adjustAnimationInterval)
		.def("addSceneObject", &LinkedFileObject::addSceneObject)
	;
}

};