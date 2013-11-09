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

#ifndef __OVITO_SELECT_EXPRESSION_MODIFIER_H
#define __OVITO_SELECT_EXPRESSION_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <core/gui/widgets/general/AutocompleteLineEdit.h>
#include "../ParticleModifier.h"

class FunctionParser;	// From muParser library.

namespace Particles {

using namespace Ovito;

/******************************************************************************
* Selects particles based on a user-defined Boolean expression.
******************************************************************************/
class OVITO_PARTICLES_EXPORT SelectExpressionModifier : public ParticleModifier
{
public:

	/// Default constructor.
	Q_INVOKABLE SelectExpressionModifier() {
		INIT_PROPERTY_FIELD(SelectExpressionModifier::_expression);
	}

	//////////////////////////// from base classes ////////////////////////////

	/// \brief This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp) override;

	/////////////////////////// specific methods ///////////////////////////////

	/// Sets the expression that is used to select particles.
	void setExpression(const QString& expression) { _expression = expression; }

	/// Returns the expression that is used to select particles.
	const QString& expression() const { return _expression; }

	/// \brief Returns the list of input variables discovered during the last modifier evaluation.
	const QStringList& lastVariableNames() const { return _variableNames; }

public:

	Q_PROPERTY(QString expression READ expression WRITE setExpression)

protected:

	/// Modifies the particle object.
	virtual ObjectStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// Compiles the stored expression string. Throws an exception on error.
	void compileExpression(FunctionParser& parser, int& numVariables);

	/// Determines the available variable names.
	QStringList getVariableNames(const PipelineFlowState& inputState);

	/// The expression that is used to select atoms.
	PropertyField<QString> _expression;

	/// The list of variables during the last evaluation.
	QStringList _variableNames;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Expression select");
	Q_CLASSINFO("ModifierCategory", "Selection");

	DECLARE_PROPERTY_FIELD(_expression);
};

/******************************************************************************
* A properties editor for the SelectExpressionModifier class.
******************************************************************************/
class SelectExpressionModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE SelectExpressionModifierEditor() {}

	/// Sets the object being edited in this editor.
	virtual void setEditObject(RefTarget* newObject) override {
		ParticleModifierEditor::setEditObject(newObject);
		updateEditorFields();
	}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

private:

	/// Updates the enabled/disabled status of the editor's controls.
	void updateEditorFields();

	QLabel* variableNamesList;
	AutocompleteLineEdit* expressionLineEdit;

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_SELECT_EXPRESSION_MODIFIER_H
