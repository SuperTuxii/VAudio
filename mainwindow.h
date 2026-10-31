//
// Created by SuperTuxi on 11.06.25.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSplitter>
#include <QPushButton>
#include <QSystemTrayIcon>

#include "pipewireapi.h"
#include "verticalscrollarea.h"

class InputPanel;
class AddSinkPanel;
class SinkPanel;

class MainWindow : public QSplitter {
    Q_OBJECT
public:
    explicit MainWindow(PipewireAPI* pipewire_api);

    bool addSinkPanel(AddSinkPanel* addSinkPanel, const QString& name);
    void setSinkPanelChecked(SinkPanel* sinkPanel, bool checked);
    void setSinkPanelSink(struct global* global);
    void setSinkPanelSource(struct global* global);
    void removeSinkPanel(SinkPanel* sinkPanel);
    void addInputPanel(InputPanel* inputPanel);
    void inputOptionInserted(struct proxy_data* proxyData);
    void inputOptionRemoved(uint32_t id);
    void removeInputPanel(InputPanel* inputPanel);
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent* event) override;
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void quit();

    void show_debug(const QString& text);
    void hide_debug();

    [[nodiscard]] PipewireAPI* pipewire() const {
        return pipewire_api;
    }
private:
    PipewireAPI* pipewire_api;
    SinkPanel* checkedSinkPanel;
    std::map<std::string, SinkPanel*> sinks;

    VerticalScrollArea sinkArea;
    QWidget sinksWidget;
    QVBoxLayout sinksLayout;
    QHBoxLayout sinkAddOptions;
    QPushButton addSinkButton;
    QPushButton openSinkButton;
    VerticalScrollArea inputArea;
    QWidget inputWidget;
    QVBoxLayout inputsLayout;
    QLabel noSinkSelectedLabel;
    QHBoxLayout inputAddOptions;
    QPushButton addInputButton;

    QSystemTrayIcon trayIcon;
    QMenu trayIconMenu;

    QLabel debugWindow;

    void addSinkButtonClicked();
    void openSinkButtonClicked();
};

#endif //MAINWINDOW_H
