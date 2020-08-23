#ifndef JsonCommandParser_h
#define JsonCommandParser_h

#include <Arduino.h>
#include "../lib/ArduinoJson-6.x/src/ArduinoJson.h"
#include "../DynamicArray/DynamicArray.h"

class JsonCommandParser
{
private:
    bool gotLine;
    String str;

public:
    JsonCommandParser() {
        gotLine = false;
        str = "";
    }
    bool isGotLine() {
        return this->gotLine;
    }
    void setGotLine(bool gotLine) {
        this->gotLine = gotLine;
    }
    bool handleCommand(String str, DynamicArray *dispenserSequenceCommand) {
        if (str != "")
        {
            this->str = str;
            gotLine = parsePositions(dispenserSequenceCommand);
        }
        return gotLine;
    }
    bool parsePositions(DynamicArray *dispenserSequenceCommand) {
        bool flag = true;
        int N = 0;
        dispenserSequenceCommand->clearArray();
        const size_t capacity = JSON_ARRAY_SIZE(10) + JSON_OBJECT_SIZE(2) + 50;
        DynamicJsonDocument doc(capacity);
        DeserializationError error = deserializeJson(doc, str);
        if (error)
        {
            flag = false;
        }
        else
        {
            N = doc["N"];
            JsonArray positions = doc["sequence"];
            for (int i = 0; i < N; i++)
            {
                int n = positions[i];
                dispenserSequenceCommand->addValue(n);
            }
        }
        return flag;
    }
};

#endif