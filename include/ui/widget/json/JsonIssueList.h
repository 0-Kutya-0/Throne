#pragma once

#include <QListWidget>

namespace JsonEdit {
    class JsonCodeEdit;

    // Problem list for a JsonCodeEdit: follows its issues and jumps to them when clicked.
    class JsonIssueList : public QListWidget {
        Q_OBJECT

    public:
        explicit JsonIssueList(QWidget* parent = nullptr);

        void attach(JsonCodeEdit* editor);

    private:
        void refresh();

        JsonCodeEdit* m_editor = nullptr;
    };
}
