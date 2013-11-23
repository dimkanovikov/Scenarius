#include "SpellCheckTextEdit.h"
#include "SpellChecker.h"
#include "SpellCheckHighlighter.h"

#include <QContextMenuEvent>
#include <QMenu>


namespace {
	const int SUGGESTIONS_ACTIONS_MAX_COUNT = 5;
}

SpellCheckTextEdit::SpellCheckTextEdit(QWidget *parent) :
	QTextEdit(parent)
{
	//
	// Настроим проверяющего
	//
	m_spellChecker = new SpellChecker(userDictionaryfile());

	//
	// Настраиваем подсветку слов не прошедших проверку орфографии
	//
	m_spellCheckHighlighter = new SpellCheckHighlighter(document(), m_spellChecker);

	//
	// Настраиваем действия контекстного меню для слов не прошедших проверку орфографии
	//
	// ... игнорировать слово
	m_ignoreWordAction = new QAction(tr("Ignore"), this);
	connect(m_ignoreWordAction, SIGNAL(triggered()), this, SLOT(aboutIgnoreWord()));
	// ... добавить слово в словарь
	m_addWordToUserDictionaryAction = new QAction(tr("Add to dictionary"), this);
	connect(m_addWordToUserDictionaryAction, SIGNAL(triggered()),
			this, SLOT(aboutAddWordToUserDictionary()));
	// ... добавляем несколько пустых пунктов, для последующего помещения в них вариантов
	for (int actionIndex = 0; actionIndex < SUGGESTIONS_ACTIONS_MAX_COUNT; ++actionIndex) {
		m_suggestionsActions.append(new QAction(QString(), this));
		connect(m_suggestionsActions.at(actionIndex), SIGNAL(triggered()),
				this, SLOT(aboutReplaceWordOnSuggestion()));
	}
}

void SpellCheckTextEdit::setSpellCheckLanguage(SpellChecker::Language _language)
{
	//
	// Установим язык проверяющего
	//
	m_spellChecker->setSpellingLanguage(_language);

	//
	// Заново выделим слова не проходящие проверку орфографии вновь заданного языка
	//
	m_spellCheckHighlighter->rehighlight();
}

QString SpellCheckTextEdit::userDictionaryfile() const
{
	return QString::null;
}

void SpellCheckTextEdit::contextMenuEvent(QContextMenuEvent* _event)
{
	//
	// Запомним позицию курсора
	//
	m_lastCursorPosition = _event->pos();

	//
	// Определим слово под курсором
	//
	QString wordUnderCursor = wordOnPosition(m_lastCursorPosition);

	//
	// Сформируем стандартное контекстное меню
	//
	QMenu* menu = createStandardContextMenu();

	//
	// Если слово не проходит проверку орфографии добавим дополнительные действия в контекстное меню
	//
	if (!m_spellChecker->spellCheckWord(wordUnderCursor)) {
		// ... действие, перед которым вставляем дополнительные пункты
		QStringList suggestions = m_spellChecker->suggestionsForWord(wordUnderCursor);
		// ... вставляем варианты
		QAction* actionInsertBefore = menu->actions().first();
		int addedSuggestionsCount = 0;
		foreach (const QString& suggestion, suggestions) {
			if (addedSuggestionsCount < SUGGESTIONS_ACTIONS_MAX_COUNT) {
				m_suggestionsActions.at(addedSuggestionsCount)->setText(suggestion);
				menu->insertAction(actionInsertBefore, m_suggestionsActions.at(addedSuggestionsCount));
				++addedSuggestionsCount;
			} else {
				break;
			}
		}
		if (addedSuggestionsCount > 0) {
			menu->insertSeparator(actionInsertBefore);
		}
		// ... вставляем дополнительные действия
		menu->insertAction(actionInsertBefore, m_ignoreWordAction);
		menu->insertAction(actionInsertBefore, m_addWordToUserDictionaryAction);
		menu->insertSeparator(actionInsertBefore);
	}

	//
	// Покажем меню, а после очистим от него память
	//
	menu->exec(_event->globalPos());
	delete menu;
}

void SpellCheckTextEdit::aboutIgnoreWord() const
{
	//
	// Определим слово под курсором
	//
	QString wordUnderCursor = wordOnPosition(m_lastCursorPosition);

	//
	// Объявляем проверяющему о том, что это слово нужно игнорировать
	//
	m_spellChecker->ignoreWord(wordUnderCursor);

	//
	// Уберём выделение с игнорируемых слов
	//
	// TODO: может стать причиной тормозов в программе на длинных текстах
	//
	m_spellCheckHighlighter->rehighlight();
}

void SpellCheckTextEdit::aboutAddWordToUserDictionary() const
{
	//
	// Определим слово под курсором
	//
	QString wordUnderCursor = wordOnPosition(m_lastCursorPosition);

	//
	// Объявляем проверяющему о том, что это слово нужно добавить в пользовательский словарь
	//
	m_spellChecker->addWordToDictionary(wordUnderCursor);

	//
	// Уберём выделение со слов добавленных в словарь
	//
	// TODO: может стать причиной тормозов в программе на длинных текстах
	//
	m_spellCheckHighlighter->rehighlight();
}

void SpellCheckTextEdit::aboutReplaceWordOnSuggestion() const
{
	if (QAction* suggestAction = qobject_cast<QAction*>(sender())) {
		QTextCursor tc = cursorForPosition(m_lastCursorPosition);
		tc.select(QTextCursor::WordUnderCursor);
		tc.removeSelectedText();
		tc.insertText(suggestAction->text());
	}
}

QString SpellCheckTextEdit::wordOnPosition(const QPoint& _position) const
{
	QTextCursor tc = cursorForPosition(_position);
	tc.select(QTextCursor::WordUnderCursor);
	return tc.selectedText();
}
