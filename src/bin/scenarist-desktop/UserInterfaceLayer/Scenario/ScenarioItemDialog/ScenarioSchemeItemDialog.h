#ifndef SCENARIOSCHEMEITEMDIALOG_H
#define SCENARIOSCHEMEITEMDIALOG_H

#include <3rd_party/Widgets/QLightBoxWidget/qlightboxdialog.h>

#include <BusinessLayer/ScenarioDocument/ScenarioModelItem.h>

namespace Ui {
    class ScenarioSchemeItemDialog;
}


namespace UserInterface
{
    /**
     * @brief Диалог редактирования элементов схемы
     */
    class ScenarioSchemeItemDialog : public QLightBoxDialog
    {
        Q_OBJECT

    public:
        explicit ScenarioSchemeItemDialog(QWidget* _parent = 0);
        ~ScenarioSchemeItemDialog();

        /**
         * @brief Восстановить значения по умолчанию
         */
        void clear();

        /**
         * @brief Получить тип карточки
         */
        BusinessLogic::ScenarioModelItem::Type cardType() const;

        /**
         * @brief Установить тип карточки
         */
        void setCardType(BusinessLogic::ScenarioModelItem::Type _type);

        /**
         * @brief Получить заголовок карточки
         */
        QString cardTitle() const;

        /**
         * @brief Установить заголовок карточки
         */
        void setCardTitle(const QString& _title);

        /**
         * @brief Получить описание карточки
         */
        QString cardDescription() const;

        /**
         * @brief Установить описание карточки
         */
        void setCardDescription(const QString& _description);

        /**
         * @brief Получить цвет карточки
         */
        QString cardColor() const;

        /**
         * @brief Установить цвет карточки
         */
        void setCardColor(const QString& _color);

    protected:
        /**
         * @brief Виджет на который нужно установить фокус при отображении
         */
        QWidget* focusedOnExec() const override;

    private:
        /**
         * @brief Настроить представление
         */
        void initView() override;

        /**
         * @brief Настроить соединения
         */
        void initConnections() override;

        /**
         * @brief Настроить внешний вид диалога
         */
        void initStyleSheet();

    private:
        Ui::ScenarioSchemeItemDialog* m_ui;
    };
}

#endif // SCENARIOSCHEMEITEMDIALOG_H
