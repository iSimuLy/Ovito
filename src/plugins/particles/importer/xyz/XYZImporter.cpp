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

#include <plugins/particles/Particles.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/Future.h>
#include <core/utilities/concurrent/Task.h>
#include <core/utilities/concurrent/ProgressManager.h>
#include <core/dataset/importexport/LinkedFileObject.h>
#include <core/gui/properties/BooleanParameterUI.h>

#include <plugins/particles/importer/InputColumnMappingDialog.h>
#include "XYZImporter.h"

namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, XYZImporter, ParticleImporter)
IMPLEMENT_OVITO_OBJECT(Particles, XYZImporterEditor, PropertiesEditor)
SET_OVITO_OBJECT_EDITOR(XYZImporter, XYZImporterEditor)

/******************************************************************************
 * Sets the user-defined mapping between data columns in the input file and
 * the internal particle properties.
 *****************************************************************************/
void XYZImporter::setColumnMapping(const InputColumnMapping& mapping)
{
	_columnMapping = mapping;

	// Remember the mapping for the next time.
	QSettings settings;
	settings.beginGroup("viz/importer/xyz/");
	settings.setValue("columnmapping", mapping.toByteArray());
	settings.endGroup();

	notifyDependents(ReferenceEvent::TargetChanged);
}

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool XYZImporter::checkFileFormat(QIODevice& input, const QUrl& sourceLocation)
{
	// Open input file.
	CompressedTextParserStream stream(input, sourceLocation.path());

	// Read first line.
	stream.readLine(20);
	if(stream.line()[0] == '\0')
		return true;

	// Skip initial whitespace.
	const char* p = stream.line();
	while(isspace(*p)) {
		if(*p == '\0') return false;
		++p;
	}
	if(!isdigit(*p)) return false;
	// Skip digits.
	while(isdigit(*p)) {
		if(*p == '\0') break;
		++p;
	}
	// Check trailing whitespace.
	while(*p != '\0') {
		if(!isspace(*p)) return false;
		++p;
	}

	return true;
}

/******************************************************************************
*  This method is called by the LinkedFileObject each time a new source
* file has been selected by the user.
******************************************************************************/
bool XYZImporter::inspectNewFile(LinkedFileObject* obj)
{
	if(obj->frames().empty())
		return false;

	// Start task that inspects the file header to determine the number of data columns.
	std::unique_ptr<XYZImportTask> inspectionTask(new XYZImportTask(obj->frames().front()));
	Future<void> future = runInBackground<void>(std::bind(&XYZImportTask::load, inspectionTask.get(), std::placeholders::_1));
	if(!ProgressManager::instance().waitForTask(future))
		return false;

	// This is to throw an exception if an error has occurred.
	future.result();

	InputColumnMapping mapping(_columnMapping);
	mapping.setColumnCount(inspectionTask->columnMapping().columnCount());
	if(_columnMapping.columnCount() == 0) {
		int oldCount = 0;

		// Load last mapping from settings store.
		QSettings settings;
		settings.beginGroup("viz/importer/xyz/");
		if(settings.contains("columnmapping")) {
			try {
				mapping.fromByteArray(settings.value("columnmapping").toByteArray());
				oldCount = mapping.columnCount();
			}
			catch(Exception& ex) {
				ex.prependGeneralMessage(tr("Failed to load last used column-to-property mapping from application settings store."));
				ex.logError();
			}
		}

		mapping.setColumnCount(inspectionTask->columnMapping().columnCount());
		for(int i = oldCount; i < mapping.columnCount(); i++)
			mapping.mapCustomColumn(i, tr("Custom property %1").arg(i+1), qMetaTypeId<FloatType>());
	}

	InputColumnMappingDialog dialog(mapping);
	if(dialog.exec() == QDialog::Accepted) {
		setColumnMapping(dialog.mapping());
		return true;
	}
	return false;
}

/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void XYZImporter::scanFileForTimesteps(FutureInterfaceBase& futureInterface, QVector<LinkedFileImporter::FrameSourceInformation>& frames, const QUrl& sourceUrl, CompressedTextParserStream& stream)
{
	futureInterface.setProgressText(tr("Scanning XYZ file %1").arg(stream.filename()));
	futureInterface.setProgressRange(stream.underlyingSize() / 1000);

	// Regular expression for whitespace characters.
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	int numParticles = 0;
	QFileInfo fileInfo(stream.filename());
	QString filename = fileInfo.fileName();
	QDateTime lastModified = fileInfo.lastModified();
	int frameNumber = 0;

	while(!stream.eof()) {
		qint64 byteOffset = stream.byteOffset();

		// Parse number of atoms.
		stream.readLine();
		int startLineNumber = stream.lineNumber();

		if(stream.line()[0] == '\0') break;
		if(sscanf(stream.line(), "%u", &numParticles) != 1 || numParticles < 0 || numParticles > 1e9)
			throw Exception(tr("Invalid number of particles in line %1 of XYZ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));

		// Create a new record for the time step.
		FrameSourceInformation frame;
		frame.sourceFile = sourceUrl;
		frame.byteOffset = byteOffset;
		frame.lineNumber = startLineNumber;
		frame.lastModificationTime = lastModified;
		frame.label = QString("%1 (Frame %2)").arg(filename).arg(frameNumber++);
		frames.push_back(frame);

		// Skip comment line.
		stream.readLine();

		// Skip atom lines.
		for(int i = 0; i < numParticles; i++) {
			stream.readLine();
			if((i % 4096) == 0) {
				futureInterface.setProgressValue(stream.underlyingByteOffset() / 1000);
				if(futureInterface.isCanceled())
					return;
			}
		}
	}
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
void XYZImporter::XYZImportTask::parseFile(FutureInterfaceBase& futureInterface, CompressedTextParserStream& stream)
{
	futureInterface.setProgressText(tr("Reading XYZ file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Parse number of atoms.
	int numParticles;
	if(sscanf(stream.readLine(), "%u", &numParticles) != 1 || numParticles < 0 || numParticles > 1e9)
		throw Exception(tr("Invalid number of particles in line %1 of XYZ file: %2").arg(stream.lineNumber()).arg(stream.lineString()));
	futureInterface.setProgressRange(numParticles);

	// Regular expression for whitespace characters.
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	// Extract some useful information from the comment line.
	stream.readLine();
	bool hasSimulationCell = false;
	int movieMode = -1;

	simulationCell().setPbcFlags(false, false, false);
	Vector3 cellOrigin = Vector3::Zero();
	Vector3 cellVector1 = Vector3::Zero();
	Vector3 cellVector2 = Vector3::Zero();
	Vector3 cellVector3 = Vector3::Zero();
	QString remainder;
	int index;

	// Try to parse the simulation cell geometry from the comment line.
	QString commentLine = stream.lineString();
	if((index = commentLine.indexOf("Lxyz=")) >= 0) remainder = commentLine.mid(index + 5).trimmed();
	else if((index = commentLine.indexOf("boxsize")) >= 0) remainder = commentLine.mid(index + 7).trimmed();
	if(!remainder.isEmpty()) {
		QStringList list = remainder.split(ws_re);
		if(list.size() >= 3) {
			bool ok1, ok2, ok3;
			FloatType sx = (FloatType)list[0].toDouble(&ok1);
			FloatType sy = (FloatType)list[1].toDouble(&ok2);
			FloatType sz = (FloatType)list[2].toDouble(&ok3);
			if(ok1 && ok2 && ok3) {
				simulationCell().setMatrix(AffineTransformation(
						Vector3(sx,0,0), Vector3(0,sy,0), Vector3(0,0,sz), Vector3(-sx/2, -sy/2, -sz/2)));
				hasSimulationCell = true;
			}
		}
	}
	if((index = commentLine.indexOf("cell_orig ")) >= 0) {
		QStringList list = commentLine.mid(index + 10).split(ws_re);
		for(int k = 0; k < list.size() && k < 3; k++)
			cellOrigin[k] = (FloatType)list[k].toDouble();
	}
	if((index = commentLine.indexOf("cell_vec1 ")) >= 0) {
		QStringList list = commentLine.mid(index + 10).split(ws_re);
		for(int k = 0; k < list.size() && k < 3; k++)
			cellVector1[k] = (FloatType)list[k].toDouble();
	}
	if((index = commentLine.indexOf("cell_vec2 ")) >= 0) {
		QStringList list = commentLine.mid(index + 10).split(ws_re);
		for(int k = 0; k < list.size() && k < 3; k++)
			cellVector2[k] = (FloatType)list[k].toDouble();
	}
	if((index = commentLine.indexOf("cell_vec3 ")) >= 0) {
		QStringList list = commentLine.mid(index + 10).split(ws_re);
		for(int k = 0; k < list.size() && k < 3; k++)
			cellVector3[k] = (FloatType)list[k].toDouble();
	}

	if(cellVector1 != Vector3::Zero() && cellVector2 != Vector3::Zero() && cellVector3 != Vector3::Zero()) {
		simulationCell().setMatrix(AffineTransformation(cellVector1, cellVector2, cellVector3, cellOrigin));
		hasSimulationCell = true;
	}
	if((index = commentLine.indexOf("pbc ")) >= 0) {
		QStringList list = commentLine.mid(index + 4).split(ws_re);
		simulationCell().setPbcFlags((bool)list[0].toInt(), (bool)list[1].toInt(), (bool)list[2].toInt());
	}

	if(_parseFileHeaderOnly) {
		// Read first atoms line and count number of data columns.
		stream.readLine();
		_columnMapping.setColumnCount(stream.lineString().split(ws_re, QString::SkipEmptyParts).size());
		return;
	}

	// Parse data columns.
	InputColumnReader columnParser(_columnMapping, *this, numParticles);
	try {
		for(size_t i = 0; i < numParticles; i++) {
			if((i % 4096) == 0) {
				if(futureInterface.isCanceled())
					return;	// Abort!
				futureInterface.setProgressValue((int)i);
			}
			stream.readLine();
			columnParser.readParticle(i, const_cast<char*>(stream.line()));
		}
	}
	catch(Exception& ex) {
		throw ex.prependGeneralMessage(tr("Parsing error in line %1 of XYZ file.").arg(stream.lineNumber()));
	}

	ParticleProperty* posProperty = particleProperty(ParticleProperty::PositionProperty);
	if(posProperty && numParticles > 0) {
		Box3 boundingBox;
		boundingBox.addPoints(posProperty->constDataPoint3(), posProperty->size());

		if(!hasSimulationCell) {
			// If the input file does not contain simulation cell info,
			// Use bounding box of particles as simulation cell.
			simulationCell().setMatrix(AffineTransformation(
					Vector3(boundingBox.sizeX(), 0, 0),
					Vector3(0, boundingBox.sizeY(), 0),
					Vector3(0, 0, boundingBox.sizeZ()),
					boundingBox.minc - Point3::Origin()));
		}
		else {
			// Find out if coordinates are given in reduced format and need to be rescaled to absolute format.
			// Check if all atom coordinates are within the [0,1] interval.
			// If yes, we assume reduced coordinate format.

			if(Box3(Point3(-0.01f), Point3(1.01f)).containsBox(boundingBox)) {
				// Convert all atom coordinates from reduced to absolute (Cartesian) format.
				const AffineTransformation simCell = simulationCell().matrix();
				Point3* p = posProperty->dataPoint3();
				Point3* p_end = p + posProperty->size();
				for(; p != p_end; ++p)
					*p = simCell * (*p);
			}
			else if(Box3(Point3(-0.51f), Point3(0.51f)).containsBox(boundingBox)) {
				// Convert all atom coordinates from reduced to absolute (Cartesian) format.
				const AffineTransformation simCell = simulationCell().matrix();
				Point3* p = posProperty->dataPoint3();
				Point3* p_end = p + posProperty->size();
				for(; p != p_end; ++p)
					*p = simCell * ((*p) + Vector3(0.5,0.5,0.5));
			}
		}
	}

	setInfoText(tr("%1 particles").arg(numParticles));
}

/******************************************************************************
 * Saves the class' contents to the given stream.
 *****************************************************************************/
void XYZImporter::saveToStream(ObjectSaveStream& stream)
{
	ParticleImporter::saveToStream(stream);

	stream.beginChunk(0x01);
	_columnMapping.saveToStream(stream);
	stream.endChunk();
}

/******************************************************************************
 * Loads the class' contents from the given stream.
 *****************************************************************************/
void XYZImporter::loadFromStream(ObjectLoadStream& stream)
{
	ParticleImporter::loadFromStream(stream);

	stream.expectChunk(0x01);
	_columnMapping.loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
 * Creates a copy of this object.
 *****************************************************************************/
OORef<RefTarget> XYZImporter::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<XYZImporter> clone = static_object_cast<XYZImporter>(ParticleImporter::clone(deepCopy, cloneHelper));
	clone->_columnMapping = this->_columnMapping;
	return clone;
}

/******************************************************************************
 * Displays a dialog box that allows the user to edit the custom file column to particle
 * property mapping.
 *****************************************************************************/
void XYZImporter::showEditColumnMappingDialog(QWidget* parent)
{
	InputColumnMappingDialog dialog(_columnMapping, parent);
	if(dialog.exec() == QDialog::Accepted) {
		setColumnMapping(dialog.mapping());
		requestReload();
	}
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void XYZImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("XYZ file"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* animFramesBox = new QGroupBox(tr("Timesteps"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(animFramesBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(animFramesBox);

	// Multi-timestep file
	BooleanParameterUI* multitimestepUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::_isMultiTimestepFile));
	sublayout->addWidget(multitimestepUI->checkBox());

	QGroupBox* columnMappingBox = new QGroupBox(tr("File columns"), rollout);
	sublayout = new QVBoxLayout(columnMappingBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(columnMappingBox);

	QPushButton* editMappingButton = new QPushButton(tr("Edit column mapping..."));
	sublayout->addWidget(editMappingButton);
	connect(editMappingButton, SIGNAL(clicked(bool)), this, SLOT(onEditColumnMapping()));
}

/******************************************************************************
* Is called when the user pressed the "Edit column mapping" button.
******************************************************************************/
void XYZImporterEditor::onEditColumnMapping()
{
	XYZImporter* importer = static_object_cast<XYZImporter>(editObject());
	if(importer) {
		importer->showEditColumnMappingDialog();
	}
}

};