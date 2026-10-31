//
// Created by SuperTuxi on 11.06.25.
//

#ifndef INPUTPANEL_H
#define INPUTPANEL_H

#include <QBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QMenu>

class QStandardItem;
class QLabel;
class SinkPanel;

class InputPanel : public QWidget {
    Q_OBJECT
public:
    InputPanel(SinkPanel* sinkPanel, struct input_data* input_data);

    void setInputOption(const QStandardItem* item);

    [[nodiscard]] SinkPanel* sink_panel() const {
        return sinkPanel;
    }
    [[nodiscard]] struct input_data * input_data() const {
        return inputData;
    }
private:
    SinkPanel* sinkPanel;
    struct input_data* inputData;

    QVBoxLayout layout;
    QHBoxLayout inputLayout;
    QComboBox inputOptions;
    QPushButton inputInformation;
    QHBoxLayout actionLayout;
    QPushButton muteButton;
    QPushButton reconnectionOptions;
    QMenu reconnectionOptionsMenu;
    QPushButton removeButton;

    void hideInputPanel();
    void inputLockChanged(bool locked);
    void inputOptionIndexChanged(int index);
    void inputOptionAboutToBeRemoved(const QModelIndex& parent, int start, int end);
    void inputInformationToggled(bool checked);
    void muteButtonToggled(bool checked);
    void loadReconnectionOption(const char* key, const char* value);
    void loadReconnectionOptions(const struct pw_node_info* nodeInfo);
    void reconnectionOptionTriggered(QAction* action);
    void removeButtonClicked();
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif //INPUTPANEL_H
