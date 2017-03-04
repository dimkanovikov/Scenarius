#ifndef FOUNTAINIMPORTER_H
#define FOUNTAINIMPORTER_H

#include "AbstractImporter.h"
#include <BusinessLayer/ScenarioDocument/ScenarioTemplate.h>

class QXmlStreamWriter;

namespace BusinessLogic
{
    /**
     * @brief Импортер Fountain-документов
     */
    class FountainImporter : public AbstractImporter
    {
    public:
        FountainImporter();

        /**
         * @brief Импорт сценария из документа
         */
        QString importScenario(const ImportParameters &_importParameters) const;

    private:
        void writeBlock(QXmlStreamWriter &writer, QString paragraphText,
                        ScenarioBlockStyle::Type blockStyle) const;
    };
}

#endif // FOUNTAINIMPORTER_H
