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

#ifndef __OVITO_LINKED_FILE_OBJECT_H
#define __OVITO_LINKED_FILE_OBJECT_H

#include <core/Core.h>
#include <core/scene/objects/SceneObject.h>
#include "LinkedFileImporter.h"

namespace Ovito {

/**
 * \brief A place holder object that feeds data read from an external file into the scene.
 *
 * This class is used in conjunction with a LinkedFileImporter class.
 */
class OVITO_CORE_EXPORT LinkedFileObject : public SceneObject
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE LinkedFileObject(DataSet* dataset);

	/// \brief Returns the parser that loads the input file.
	LinkedFileImporter* importer() const { return _importer; }

	/// \brief Sets the source location for importing data.
	/// \param sourceUrl The new source location.
	/// \param importerType The type of importer object that will parse the input file (can be NULL to request auto-detection).
	/// \return false if the operation has been canceled by the user.
	bool setSource(const QUrl& sourceUrl, const FileImporterDescription* importerType = nullptr);

	/// \brief Sets the source location for importing data.
	/// \param sourceUrl The new source location.
	/// \param importer The importer object that will parse the input file.
	/// \return false if the operation has been canceled by the user.
	bool setSource(QUrl sourceUrl, const OORef<LinkedFileImporter>& importer, bool useExactURL = false);

	/// \brief Returns the source location of the data.
	const QUrl& sourceUrl() const { return _sourceUrl; }

	/// \brief This reloads the input data from the external file.
	/// \param frame The animation frame to reload from the external file.
	Q_INVOKABLE void refreshFromSource(int frame = -1);

	/// \brief Returns the status returned by the file parser on its last invocation.
	virtual PipelineStatus status() const override { return _importStatus; }

	/// \brief Scans the input source for animation frames and updates the internal list of frames.
	Q_INVOKABLE bool updateFrames();

	/// \brief Returns the number of animation frames that can be loaded from the data source.
	int numberOfFrames() const { return _frames.size(); }

	/// \brief Returns the index of the animation frame loaded last from the input file.
	int loadedFrame() const { return _loadedFrame; }

	/// \brief Returns the list of animation frames in the input file(s).
	const QVector<LinkedFileImporter::FrameSourceInformation>& frames() const { return _frames; }

	/// \brief Given an animation time, computes the input frame index to be shown at that time.
	Q_INVOKABLE int animationTimeToInputFrame(TimePoint time) const;

	/// \brief Given an input frame index, returns the animation time at which it is shown.
	Q_INVOKABLE TimePoint inputFrameToAnimationTime(int frame) const;

	/// \brief Returns whether the scene's animation interval is being adjusted to the number of frames reported by the file parser.
	bool adjustAnimationIntervalEnabled() const { return _adjustAnimationIntervalEnabled; }

	/// \brief Controls whether the scene's animation interval should be adjusted to the number of frames reported by the file parser.
	void setAdjustAnimationIntervalEnabled(bool enabled) { _adjustAnimationIntervalEnabled = enabled; }

	/// \brief Adjusts the animation interval of the current data set to the number of frames in the data source.
	void adjustAnimationInterval(int gotoFrameIndex = -1);

	/// \brief Requests a frame of the input file sequence.
	PipelineFlowState requestFrame(int frame);

	/// \brief Asks the object for the result of the geometry pipeline at the given time.
	virtual PipelineFlowState evaluate(TimePoint time) override;

	/// \brief Returns the list of imported scene objects.
	const QVector<SceneObject*>& sceneObjects() const { return _sceneObjects; }

	/// \brief Inserts a new object into the list of scene objects held by this container object.
	void addSceneObject(SceneObject* obj) {
		if(!_sceneObjects.contains(obj)) {
			obj->setSaveWithScene(saveWithScene());
			_sceneObjects.push_back(obj);
		}
	}

	/// \brief Looks for an object of the given type in the list of scene objects and returns it.
	template<class T>
	T* findSceneObject() const {
		for(SceneObject* obj : sceneObjects()) {
			T* castObj = dynamic_object_cast<T>(obj);
			if(castObj) return castObj;
		}
		return nullptr;
	}

	/// \brief Removes all scene objects owned by this LinkedFileObject that are not
	///        listed in the given set of active objects.
	void removeInactiveObjects(const QSet<SceneObject*>& activeObjects) {
		for(int index = _sceneObjects.size() - 1; index >= 0; index--)
			if(!activeObjects.contains(_sceneObjects[index]))
				_sceneObjects.remove(index);
	}

	/// \brief Controls whether the imported data is saved along with the scene.
	/// \param on \c true if data should be stored in the scene file; \c false if the data resides only in the external file.
	/// \undoable
	void setSaveWithScene(bool on) override {
		SceneObject::setSaveWithScene(on);
		// Propagate flag to sub-objects.
		for(SceneObject* sceneObj : sceneObjects())
			sceneObj->setSaveWithScene(on);
	}

	/// Returns the attributes set or loaded by the file importer which are fed into the modification pipeline
	/// along with the scene objects.
	const QVariantMap& attributes() const { return _attributes; }

	/// Sets the attributes that will be fed into the modification pipeline
	/// along with the scene objects.
	void setAttributes(const QVariantMap& attributes) { _attributes = attributes; }

	/// Resets the attributes that will be fed into the modification pipeline
	/// along with the scene objects.
	void clearAttributes() { _attributes.clear(); }

	/// Returns the title of this object.
	virtual QString objectTitle() override;

	/// Returns the number of sub-objects that should be displayed in the modifier stack.
	virtual int editableSubObjectCount() override;

	/// Returns a sub-object that should be listed in the modifier stack.
	virtual RefTarget* editableSubObject(int index) override;

public Q_SLOTS:

	/// \brief Displays the file selection dialog and lets the user select a new input file.
	void showFileSelectionDialog(QWidget* parent = nullptr);

	/// \brief Displays the remote file selection dialog and lets the user select a new source URL.
	void showURLSelectionDialog(QWidget* parent = nullptr);

public:

	Q_PROPERTY(QUrl sourceUrl READ sourceUrl WRITE setSource);
	Q_PROPERTY(LinkedFileImporter* importer READ importer);
	Q_PROPERTY(int numberOfFrames READ numberOfFrames);
	Q_PROPERTY(bool adjustAnimationIntervalEnabled READ adjustAnimationIntervalEnabled WRITE setAdjustAnimationIntervalEnabled);

protected Q_SLOTS:

	/// \brief This is called when the background loading operation has finished.
	void loadOperationFinished();

protected:

	/// \brief Saves the status returned by the parser object and generates a ReferenceEvent::ObjectStatusChanged event.
	void setStatus(const PipelineStatus& status);

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// \brief Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// \brief Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// \brief Cancels the current load operation if there is any in progress.
	void cancelLoadOperation();

private:

	/// The associated importer object that is responsible for parsing the input file.
	ReferenceField<LinkedFileImporter> _importer;

	/// Stores the imported scene objects.
	VectorReferenceField<SceneObject> _sceneObjects;

	/// Controls whether the scene's animation interval is adjusted to the number of frames found in the input file.
	PropertyField<bool> _adjustAnimationIntervalEnabled;

	/// The source file (may include a wild-card pattern).
	PropertyField<QUrl, QUrl, ReferenceEvent::TitleChanged> _sourceUrl;

	/// Controls the mapping of input file frames to animation frames (i.e. the numerator of the playback rate for the file sequence).
	PropertyField<int> _playbackSpeedNumerator;

	/// Controls the mapping of input file frames to animation frames (i.e. the denominator of the playback rate for the file sequence).
	PropertyField<int> _playbackSpeedDenominator;

	/// Specifies the starting animation frame to which the first frame of the file sequence is mapped.
	PropertyField<int> _playbackStartTime;

	/// Stores the list of animation frames in the input file(s).
	QVector<LinkedFileImporter::FrameSourceInformation> _frames;

	/// The index of the animation frame loaded last from the input file.
	int _loadedFrame;

	/// The index of the animation frame currently being loaded.
	int _frameBeingLoaded;

	/// The background file loading task started by evaluate().
	Future<LinkedFileImporter::ImportTaskPtr> _loadFrameOperation;

	/// The watcher object that is used to monitor the background operation.
	FutureWatcher _loadFrameOperationWatcher;

	/// The status returned by the parser during its last call.
	PipelineStatus _importStatus;

	/// Attributes set or loaded by the file importer which will be fed into the modification pipeline
	/// along with the scene objects.
	QVariantMap _attributes;

private:

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_REFERENCE_FIELD(_importer);
	DECLARE_VECTOR_REFERENCE_FIELD(_sceneObjects);
	DECLARE_PROPERTY_FIELD(_adjustAnimationIntervalEnabled);
	DECLARE_PROPERTY_FIELD(_sourceUrl);
	DECLARE_PROPERTY_FIELD(_playbackSpeedNumerator);
	DECLARE_PROPERTY_FIELD(_playbackSpeedDenominator);
	DECLARE_PROPERTY_FIELD(_playbackStartTime);
};

};

#endif // __OVITO_LINKED_FILE_OBJECT_H
