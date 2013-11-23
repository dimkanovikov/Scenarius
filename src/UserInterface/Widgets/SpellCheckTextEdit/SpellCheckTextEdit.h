#ifndef SPELLCHECKTEXTEDIT_H
#define SPELLCHECKTEXTEDIT_H

#include <QTextEdit>

#include "SpellChecker.h"

class SpellCheckHighlighter;


/**
 * @brief Класс текстового редактора с проверкой орфографии
 */
class SpellCheckTextEdit : public QTextEdit
{
	Q_OBJECT

public:
	explicit SpellCheckTextEdit(QWidget *parent = 0);

	/**
	 * @brief Установить язык для проверки орфографии
	 * @param Язык
	 */
	void setSpellCheckLanguage(SpellChecker::Language _language);

protected:
	/**
	 * @brief Получить путь к файлу с пользовательским словарём
	 * @return Путь к файлу со словарём
	 *
	 * @note В текущей реализации возвращается пустая строка.
	 */
	virtual QString userDictionaryfile() const;

	/**
	 * @brief Переопределено для добавления действий связанных с проверкой орфографии
	 */
	void contextMenuEvent(QContextMenuEvent* _event);

private slots:
	/**
	 * @brief Игнорировать слово под курсором
	 */
	void aboutIgnoreWord() const;

	/**
	 * @brief Добавить слово под курсором в пользовательский словарь
	 */
	void aboutAddWordToUserDictionary() const;

	/**
	 * @brief Заменить слово под курсором на выбранный вариант из предложенных
	 */
	void aboutReplaceWordOnSuggestion() const;

private:
	/**
	 * @brief Найти слово в позиции
	 * @param Позиция в тексте
	 * @return Слово, находящееся в данной позиции
	 */
	QString wordOnPosition(const QPoint& _position) const;

private:
	/**
	 * @brief Проверяющий орфографию
	 */
	SpellChecker* m_spellChecker;

	/**
	 * @brief Подсвечивающий орфографические ошибки
	 */
	SpellCheckHighlighter* m_spellCheckHighlighter;

	/**
	 * @brief Действия для слова не прошедшего проверку орфографии
	 */
	/** @{ */
	QAction* m_ignoreWordAction;
	QAction* m_addWordToUserDictionaryAction;
	QList<QAction*> m_suggestionsActions;
	/** @} */

	/**
	 * @brief Последняя позиция курсора, при открытии контекстного меню
	 */
	QPoint m_lastCursorPosition;
};

#endif // SPELLCHECKTEXTEDIT_H
