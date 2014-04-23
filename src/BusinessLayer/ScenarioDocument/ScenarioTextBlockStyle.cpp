#include "ScenarioTextBlockStyle.h"
#include "ScenarioTextBlockStylePrivate.h"

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/SettingsStorage.h>

#include <QTextBlock>

using namespace BusinessLogic;


ScenarioTextBlockStyle::ScenarioTextBlockStyle(ScenarioTextBlockStyle::Type _blockType) :
	m_pimpl(new ScenarioTextBlockStylePrivate(ScenarioTextBlockStyle::Undefined))
{
	setType(_blockType);
}

ScenarioTextBlockStyle::~ScenarioTextBlockStyle()
{
	delete m_pimpl;
}

ScenarioTextBlockStyle::Type ScenarioTextBlockStyle::forBlock(const QTextBlock& _block)
{
	ScenarioTextBlockStyle::Type blockType = ScenarioTextBlockStyle::Undefined;
	if (_block.blockFormat().hasProperty(ScenarioTextBlockStyle::PropertyType)) {
		blockType = (ScenarioTextBlockStyle::Type)_block.blockFormat().intProperty(ScenarioTextBlockStyle::PropertyType);
	}
	return blockType;
}

void ScenarioTextBlockStyle::setType(ScenarioTextBlockStyle::Type _type)
{
	m_pimpl->blockType = _type;

	//
	// Запомним в стиле его настройки
	//
	m_pimpl->blockFormat.setProperty(ScenarioTextBlockStyle::PropertyType, blockType());
	m_pimpl->blockFormat.setProperty(ScenarioTextBlockStyle::PropertyHeaderType,
									 ScenarioTextBlockStyle::Undefined);
	m_pimpl->charFormat.setProperty(ScenarioTextBlockStyle::PropertyIsFirstUppercase, true);
	m_pimpl->charFormat.setProperty(ScenarioTextBlockStyle::PropertyIsCanModify, true);
	m_pimpl->charFormat.setForeground(
				QColor(
					DataStorageLayer::StorageFacade::settingsStorage()->value(
						"scenario-editor/text-color",
						DataStorageLayer::SettingsStorage::ApplicationSettings)
					));

	//
	// Настроим остальные характеристики
	//
	switch (blockType()) {
		case TimeAndPlace: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->charFormat.setFontCapitalization(QFont::AllUppercase);
			break;
		}

		case Action: {
			m_pimpl->blockFormat.setTopMargin(15);
			break;
		}

		case Character: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->blockFormat.setLeftMargin(200);
			m_pimpl->charFormat.setFontCapitalization(QFont::AllUppercase);
			break;
		}

		case Parenthetical: {
			m_pimpl->blockFormat.setLeftMargin(150);
			m_pimpl->blockFormat.setRightMargin(150);
			m_pimpl->charFormat.setProperty(ScenarioTextBlockStyle::PropertyIsFirstUppercase, false);
			m_pimpl->charFormat.setProperty(ScenarioTextBlockStyle::PropertyPrefix, "(");
			m_pimpl->charFormat.setProperty(ScenarioTextBlockStyle::PropertyPostfix, ")");
			break;
		}

		case Dialog: {
			m_pimpl->blockFormat.setLeftMargin(100);
			m_pimpl->blockFormat.setRightMargin(100);
			break;
		}

		case Transition: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->blockFormat.setAlignment(Qt::AlignRight);
			m_pimpl->charFormat.setFontCapitalization(QFont::AllUppercase);
			break;
		}

		case Note: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->charFormat.setFontCapitalization(QFont::AllUppercase);
			break;
		}

		case TitleHeader: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->charFormat.setFontCapitalization(QFont::AllUppercase);
			m_pimpl->charFormat.setProperty(ScenarioTextBlockStyle::PropertyIsCanModify, false);
			break;
		}

		case Title: {
			m_pimpl->blockFormat.setLeftMargin(150);
			m_pimpl->blockFormat.setRightMargin(150);
			m_pimpl->blockFormat.setProperty(ScenarioTextBlockStyle::PropertyHeaderType,
											 ScenarioTextBlockStyle::TitleHeader);
			m_pimpl->blockFormat.setProperty(ScenarioTextBlockStyle::PropertyHeader,
											 QObject::tr("Title:", "ScenarioTextBlockStyle"));
			break;
		}

		case NoprintableText: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->blockFormat.setLeftMargin(200);
			m_pimpl->charFormat.setForeground(
						QColor(
							DataStorageLayer::StorageFacade::settingsStorage()->value(
								"scenario-editor/nonprintable-text-color",
								DataStorageLayer::SettingsStorage::ApplicationSettings)
							));
			break;
		}

		case SceneGroupHeader:
		case SceneGroupFooter: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->charFormat.setFontCapitalization(QFont::AllUppercase);
			break;
		}

		case FolderHeader:
		case FolderFooter: {
			m_pimpl->blockFormat.setTopMargin(15);
			m_pimpl->blockFormat.setBackground(
						QColor(
							DataStorageLayer::StorageFacade::settingsStorage()->value(
								"scenario-editor/folder-background-color",
								DataStorageLayer::SettingsStorage::ApplicationSettings)
							));
			m_pimpl->charFormat.setForeground(
						QColor(
							DataStorageLayer::StorageFacade::settingsStorage()->value(
								"scenario-editor/folder-text-color",
								DataStorageLayer::SettingsStorage::ApplicationSettings)
							));
			m_pimpl->charFormat.setFontCapitalization(QFont::AllUppercase);
			break;
		}

		default: {
			break;
		}
	}
}

ScenarioTextBlockStyle::Type ScenarioTextBlockStyle::blockType() const
{
	return m_pimpl->blockType;
}

QTextBlockFormat ScenarioTextBlockStyle::blockFormat() const
{
	return m_pimpl->blockFormat;
}

QTextCharFormat ScenarioTextBlockStyle::charFormat() const
{
	return m_pimpl->charFormat;
}

bool ScenarioTextBlockStyle::isFirstUppercase() const
{
	return m_pimpl->charFormat.boolProperty(ScenarioTextBlockStyle::PropertyIsFirstUppercase);
}

bool ScenarioTextBlockStyle::isCanModify() const
{
	return m_pimpl->charFormat.boolProperty(ScenarioTextBlockStyle::PropertyIsCanModify);
}

bool ScenarioTextBlockStyle::hasDecoration() const
{
	return !prefix().isEmpty()
			|| !postfix().isEmpty();
}

QString ScenarioTextBlockStyle::prefix() const
{
	return m_pimpl->charFormat.stringProperty(ScenarioTextBlockStyle::PropertyPrefix);
}

QString ScenarioTextBlockStyle::postfix() const
{
	return m_pimpl->charFormat.stringProperty(ScenarioTextBlockStyle::PropertyPostfix);
}

bool ScenarioTextBlockStyle::hasHeader() const
{
	return !header().isEmpty();
}

ScenarioTextBlockStyle::Type ScenarioTextBlockStyle::headerType() const
{
	return (ScenarioTextBlockStyle::Type)m_pimpl->blockFormat.intProperty(ScenarioTextBlockStyle::PropertyHeaderType);
}

QString ScenarioTextBlockStyle::header() const
{
	return m_pimpl->blockFormat.stringProperty(ScenarioTextBlockStyle::PropertyHeader);
}

bool ScenarioTextBlockStyle::isHeader() const
{
	return blockType() == ScenarioTextBlockStyle::TitleHeader;
}

bool ScenarioTextBlockStyle::isEmbeddable() const
{
	return
			blockType() == SceneGroupHeader
			|| blockType() == SceneGroupFooter
			|| blockType() == FolderHeader
			|| blockType() == FolderFooter;
}

bool ScenarioTextBlockStyle::isEmbeddableHeader() const
{
	return
			blockType() == SceneGroupHeader
			|| blockType() == FolderHeader;
}

ScenarioTextBlockStyle::Type ScenarioTextBlockStyle::embeddableFooter() const
{
	ScenarioTextBlockStyle::Type footer = Undefined;

	if (blockType() == SceneGroupHeader) {
		footer = SceneGroupFooter;
	} else if (blockType() == FolderHeader) {
		footer = FolderFooter;
	}

	return footer;
}
