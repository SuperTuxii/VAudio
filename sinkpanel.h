//
// Created by SuperTuxi on 11.06.25.
//

#ifndef SINKPANEL_H
#define SINKPANEL_H

#include <QSlider>
#include <QSpinBox>
#include <QFileDialog>

#include "mainwindow.h"
#include "pipewireapi.h"

class InputPanel;
class MainWindow;

struct input_data {
    InputPanel* panel = nullptr;
    struct global* global = nullptr;
    QString name;
    bool muted;
    std::map<std::string, std::string> reconnectionOptionMap;
};

class SinkPanel : public QPushButton {
    Q_OBJECT
public:
    SinkPanel(MainWindow* mainWindow, const QString& name);

    void addInputPanel();
    void removeInputPanel(InputPanel* inputPanel);
    bool setInputPanelOption(struct input_data* inputData, uint32_t oldId);
    void inputPanelMute(struct input_data* inputData);

    void setInputPanelInactive(struct input_data* inputData);
    void inputOptionInserted(struct proxy_data* proxyData);
    void inputOptionRemoved(uint32_t id);
    void load();
    void unload();
    QJsonObject toJson() const;
    void readJson(const QJsonObject &json);

    static void updateReconnectionOptionMap(struct input_data* inputData, struct pw_node_info* nodeInfo);

    [[nodiscard]] MainWindow* main_window() const {
        return mainWindow;
    }
    [[nodiscard]] QString name() const {
        return label.text();
    }
    [[nodiscard]] bool isLocked() const {
        return lockButton.isChecked();
    }
    [[nodiscard]] bool isSuspended() const {
        return suspendButton.isChecked();
    }

    void set_sink(struct global* sink) {
        this->sink = sink;
    }
    void set_source(struct global* source) {
        this->source = source;
        if (!this->isSuspended() && source)
            this->mainWindow->pipewire()->linkNodes(sink->id, source->id);
    }
signals:
    void hideInputPanels();
    void removeInputPanels();
    void inputLockChanged(bool locked);
private:
    MainWindow* mainWindow;
    struct global* sink;
    struct global* source;
    std::list<struct input_data*> inputs;
    std::map<uint32_t, struct input_data*> activeInputs;
    std::list<struct input_data*> reconnectionInputs;

    QVBoxLayout layout;
    QLabel label;
    QHBoxLayout volumeLayout;
    QSlider volumeSlider;
    QSpinBox volumeValueBox;
    QHBoxLayout actionLayout;
    QPushButton microphoneButton;
    QHBoxLayout sinkActionLayout;
    QPushButton lockButton;
    QPushButton suspendButton;
    QPushButton unloadButton;

    void sinkPanelToggled(bool checked);
    void volumeChanged(int value);
    void microphoneButtonToggled(bool checked);
    void lockButtonToggled(bool checked);
    void suspendButtonToggled(bool checked);
    void unloadButtonClicked();
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif //SINKPANEL_H
