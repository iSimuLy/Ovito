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

#include <plugins/scripting/Scripting.h>
#include "ScriptingApplet.h"

#include <QString>
//#include <QJSEngine>
#include <QtScript>

//#include <plugins/particles/modifier/slice/SliceModifier.h>
#include "../bindings/ScriptBindings.h"

namespace Scripting {

IMPLEMENT_OVITO_OBJECT(Scripting, ScriptingApplet, UtilityApplet);

/******************************************************************************
* Initializes the utility applet.
******************************************************************************/
ScriptingApplet::ScriptingApplet() : _panel(nullptr)
{
}

/******************************************************************************
* Shows the UI of the utility in the given RolloutContainer.
******************************************************************************/
void ScriptingApplet::openUtility(RolloutContainer* container, const RolloutInsertionParameters& rolloutParams)
{
	// Create main panel widget.
	_panel = new QWidget();
	QVBoxLayout* layout = new QVBoxLayout(_panel);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	// Create code editor widget.
	_editor = new CodeEdit(_panel);
	_editor->setPlainText(QStringLiteral("1+2"));
	connect(_editor, &CodeEdit::ctrlEnterPressed,
			this, &ScriptingApplet::runScript);
	layout->addWidget(_editor, 1);

	// Create output widget.
	_output = new QLabel(_panel);
	_output->setTextFormat(Qt::PlainText);
	_output->setText("<output goes here>");
	layout->addWidget(_output, 1);

	// Create run button.
	QPushButton* runScriptBtn = new QPushButton(tr("Run"), _panel);
	connect(runScriptBtn, &QPushButton::clicked, this,
			&ScriptingApplet::runScript);
	layout->addWidget(runScriptBtn);

	// Create rollout around panel widget.
	container->addRollout(_panel, tr("Scripting"),
						  rolloutParams.useAvailableSpace());
}

/******************************************************************************
* Removes the UI of the utility from the rollout container.
******************************************************************************/
void ScriptingApplet::closeUtility(RolloutContainer* container)
{
	delete _panel;
}

/******************************************************************************
* Runs the current script in the editor.
******************************************************************************/
void ScriptingApplet::runScript()
{
	// Set up engine.
	QObject parent; // <- for memory management.
	QScriptEngine* engine = prepareEngine(&parent);

	// Evaluate.
	QScriptValue result = engine->evaluate(_editor->toPlainText());
	if(result.isError()) {
		_output->setStyleSheet("QLabel { color: red; }");
		_output->setText(result.toString());
	} else {
		_output->setStyleSheet("QLabel { }");
		_output->setText(result.toString());
		if (result.isArray()) {
			QString s;
			s += "ARRAY: [";
			int length = result.property("length").toInteger();
			for (int i = 0; i != length; ++i)
				s += result.property(i).toString() + ",\n";
			s += "]";
			_output->setText(s);
		}
	}
}

};


// Local variables:
// indent-tabs-mode: t
// tab-width: 4
// c-basic-offset: 4
// End:
