#pragma once

#include <QList>
#include <QString>

#include "include/ui/widget/json/JsonTree.h"

namespace JsonEdit {
    enum class Severity { Error, Warning };

    struct Issue {
        Severity severity = Severity::Error;
        QString message;
        QString pointer;
        Span span;
        // set when a discriminated union rejected the value outright, so a caller weighing
        // branches can tell "this is the wrong variant" from "this variant has a bad field"
        bool variantMismatch = false;
    };

    // Type checking is injected into the editor: a null validator leaves syntax checking only.
    class Validator {
    public:
        virtual ~Validator() = default;
        virtual QList<Issue> Validate(const Value& root) const = 0;
    };
}
